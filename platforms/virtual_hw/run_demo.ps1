# One-shot virtual-hardware demonstration (Windows PowerShell).
$ErrorActionPreference = "Stop"
Set-Location "$PSScriptRoot\..\..\VisionPilot\docker"

Write-Host "==> Generating synthetic camera + speed assets"
docker compose -f docker-compose.demo.yml run --rm --no-deps demo-assets

Write-Host "==> Starting VisionPilot + virtual ECU"
docker compose -f docker-compose.demo.yml up --abort-on-container-exit --exit-code-from visionpilot-demo visionpilot-demo virtual-ecu
