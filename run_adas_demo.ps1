# One-click Vision Pilot ADAS component demo (virtual camera + virtual ECU).
# Usage: .\run_adas_demo.bat   OR   .\run_adas_demo.ps1 [-SkipBuild] [-Frames 50]
param(
    [switch]$SkipBuild,
    [switch]$Rebuild,
    [int]$Frames = 50
)

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$DockerDir = Join-Path $Root "VisionPilot\docker"
$OutDir = Join-Path $Root "platforms\virtual_hw"
$RawLog = Join-Path $OutDir "last_demo_output.txt"
$Report = Join-Path $OutDir "adas_demo_report.txt"

function Assert-Ok([string]$What) {
    if ($LASTEXITCODE -ne 0) { throw "$What failed (exit $LASTEXITCODE)" }
}

function Write-Banner([string]$Text) {
    Write-Host ""
    Write-Host ("=" * 72) -ForegroundColor Cyan
    Write-Host " $Text" -ForegroundColor Cyan
    Write-Host ("=" * 72) -ForegroundColor Cyan
}

function Ensure-Docker {
    Write-Banner "1/5  Docker engine"
    docker info 2>$null | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host ("Docker ready: " + (docker version --format '{{.Server.Version}}'))
        return
    }
    $exe = @(
        "$env:ProgramFiles\Docker\Docker\Docker Desktop.exe",
        "${env:ProgramFiles(x86)}\Docker\Docker\Docker Desktop.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $exe) { throw "Docker Desktop not found. Install Docker Desktop first." }
    Write-Host "Starting Docker Desktop..."
    Start-Process $exe | Out-Null
    $deadline = (Get-Date).AddMinutes(3)
    do {
        Start-Sleep -Seconds 4
        docker info 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Docker is ready"
            return
        }
        Write-Host "  waiting for daemon..."
    } while ((Get-Date) -lt $deadline)
    throw "Docker did not become ready within 3 minutes"
}

function Ensure-Image {
    Write-Banner "2/5  VisionPilot CPU image"
    Push-Location $DockerDir
    try {
        docker image inspect visionpilot:cpu 2>$null | Out-Null
        $missing = ($LASTEXITCODE -ne 0)
        if ($SkipBuild -and -not $missing -and -not $Rebuild) {
            Write-Host "Using existing visionpilot:cpu (-SkipBuild)"
            return
        }
        Write-Host "Building visionpilot:cpu..."
        docker build -t visionpilot:cpu -f Dockerfile.cpu --build-arg ENABLE_ROS2=OFF ..
        Assert-Ok "Docker build"
        Write-Host "Image ready: visionpilot:cpu"
    } finally {
        Pop-Location
    }
}

function Invoke-Demo {
    Write-Banner "3/5  Synthetic camera + speed assets"
    Push-Location $DockerDir
    try {
        docker compose -f docker-compose.demo.yml down --remove-orphans 2>&1 | Out-Null

        $genCmd = "pip install --no-cache-dir opencv-python-headless numpy && python generate_demo_assets.py --out-dir /data/demo --frames $Frames --fps 10"
        docker compose -f docker-compose.demo.yml run --rm --no-deps --entrypoint bash demo-assets -lc $genCmd
        Assert-Ok "Demo asset generation"

        Write-Banner "4/5  Running ADAS stack (VisionPilot + virtual ECU)"
        if (Test-Path $RawLog) { Remove-Item $RawLog -Force }

        & docker compose -f docker-compose.demo.yml up --abort-on-container-exit --exit-code-from visionpilot-demo visionpilot-demo virtual-ecu 2>&1 |
            Tee-Object -FilePath $RawLog | ForEach-Object {
                $line = "$_"
                if ($line -match "ADAS ACC=|Ready|Headless|actuator CMD|provider=cpu|exited with code") {
                    Write-Host $line
                }
            }
        $code = $LASTEXITCODE
        if ($code -ne 0) {
            Write-Host "Compose finished with code $code (report will still be generated if logs exist)" -ForegroundColor Yellow
        }
    } finally {
        Pop-Location
    }
}

function Write-AdasReport {
    Write-Banner "5/5  ADAS component report"
    if (-not (Test-Path $RawLog)) { throw "No demo log at $RawLog" }

    $lines = Get-Content $RawLog
    $adas = @($lines | Where-Object { $_ -match "ADAS ACC=" })
    $ecu = @($lines | Where-Object { $_ -match "actuator CMD" })
    $models = @($lines | Where-Object { $_ -match "Ready|provider=cpu|Headless" })

    $accOn = @($adas | Where-Object { $_ -match "ACC=ON" }).Count
    $fcwOn = @($adas | Where-Object { $_ -match "FCW=ON" }).Count
    $aebOn = @($adas | Where-Object { $_ -match "AEB=ON" }).Count
    $lkasOn = @($adas | Where-Object { $_ -match "LKAS=ON" }).Count
    $ldwOn = @($adas | Where-Object { $_ -match "LDW=(LLDW|RLDW)" }).Count
    $isaOver = @($adas | Where-Object { $_ -match "ISA=OVER" }).Count
    $apOn = @($adas | Where-Object { $_ -match "AUTOPILOT=ENGAGED" }).Count
    $cipo = @($adas | Where-Object { $_ -match "cipo=true" }).Count

    $out = @()
    $out += "Vision Pilot - ADAS Virtual Hardware Demo Report"
    $out += "Generated: $(Get-Date -Format o)"
    $out += "Frames requested: $Frames"
    $out += "ADAS log frames: $($adas.Count)"
    $out += "Virtual ECU CMD messages: $($ecu.Count)"
    $out += ""
    $out += "Component activation counts (frames where feature was ON/ENGAGED):"
    $out += "  ACC        (adaptive cruise / longitudinal) : $accOn"
    $out += "  FCW        (forward collision warning)      : $fcwOn"
    $out += "  AEB        (autonomous emergency braking)   : $aebOn"
    $out += "  LKAS       (lane keep assist / steering)    : $lkasOn"
    $out += "  LDW        (lane departure LLDW/RLDW)       : $ldwOn"
    $out += "  ISA OVER   (speed above limit)              : $isaOver"
    $out += "  AUTOPILOT  (ACC+LKAS engaged)               : $apOn"
    $out += "  CIPO track (closest in-path object)         : $cipo"
    $out += ""
    $out += "--- Model / runtime ---"
    $out += ($models | Select-Object -First 8)
    $out += ""
    $out += "--- Sample ADAS frames (first 10) ---"
    $out += ($adas | Select-Object -First 10)
    $out += ""
    $out += "--- Sample ADAS frames (last 5) ---"
    $out += ($adas | Select-Object -Last 5)
    $out += ""
    $out += "--- Sample virtual ECU actuator CMDs ---"
    $out += ($ecu | Select-Object -First 8)

    $text = ($out -join "`r`n")
    Set-Content -Path $Report -Value $text -Encoding UTF8
    Write-Host $text
    Write-Host ""
    Write-Host "Raw log : $RawLog" -ForegroundColor Green
    Write-Host "Report  : $Report" -ForegroundColor Green
}

Write-Banner "Vision Pilot ADAS one-click demo"
Write-Host "Repo: $Root"
Ensure-Docker
Ensure-Image
Invoke-Demo
Write-AdasReport
Write-Banner "DONE"
exit 0
