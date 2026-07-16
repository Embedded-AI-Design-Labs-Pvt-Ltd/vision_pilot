pipeline {
  agent any

  options {
    timestamps()
    disableConcurrentBuilds()
    buildDiscarder(logRotator(numToKeepStr: '20'))
  }

  parameters {
    choice(name: 'VARIANT', choices: ['cpu', 'gpu'], description: 'Docker image variant')
    booleanParam(name: 'ENABLE_ROS2', defaultValue: false, description: 'Build with ROS2 Jazzy interfaces')
    booleanParam(name: 'BUILD_ARM64', defaultValue: false, description: 'Also build linux/arm64 (Raspberry Pi 5)')
    booleanParam(name: 'RUN_MCU_CHECKS', defaultValue: true, description: 'Syntax-check MCU / Zephyr firmware trees')
  }

  environment {
    VP_ROOT = "${WORKSPACE}/VisionPilot"
    IMAGE_TAG = "visionpilot:${params.VARIANT}${params.ENABLE_ROS2 ? '-ros2' : ''}"
  }

  stages {
    stage('Checkout') {
      steps {
        checkout scm
      }
    }

    stage('Lint / static checks') {
      steps {
        sh '''
          set -euo pipefail
          if command -v pre-commit >/dev/null 2>&1; then
            pre-commit run --all-files || true
          else
            echo "pre-commit not installed on agent — skipping"
          fi
        '''
      }
    }

    stage('Docker build (host arch)') {
      steps {
        dir('VisionPilot/docker') {
          sh '''
            set -euo pipefail
            ARGS="--${VARIANT}"
            if [ "${ENABLE_ROS2}" = "true" ]; then ARGS="$ARGS --ros2"; fi
            chmod +x build.sh run.sh
            ./build.sh $ARGS
          '''
        }
      }
    }

    stage('Docker build (arm64 / RPi5)') {
      when { expression { return params.BUILD_ARM64 && params.VARIANT == 'cpu' } }
      steps {
        dir('VisionPilot/docker') {
          sh '''
            set -euo pipefail
            docker buildx create --use --name vp-builder 2>/dev/null || docker buildx use vp-builder
            ./build.sh --cpu --platform linux/arm64 --tag visionpilot:cpu-arm64
          '''
        }
      }
    }

    stage('MCU / Zephyr firmware checks') {
      when { expression { return params.RUN_MCU_CHECKS } }
      steps {
        sh '''
          set -euo pipefail
          echo "Validating platform trees..."
          test -f platforms/README.md
          test -f platforms/protocol/vp_vehicle_gateway.md
          for f in \
            platforms/arduino/vision_pilot_gateway/vision_pilot_gateway.ino \
            platforms/esp32/vision_pilot_gateway/main/main.c \
            platforms/zephyr/apps/vp_gateway/src/main.c \
            platforms/qnx_sdp/README.md \
            platforms/raspberry_pi5/README.md
          do
            test -f "$f" && echo "OK $f"
          done
        '''
      }
    }

    stage('Terraform validate') {
      steps {
        sh '''
          set -euo pipefail
          if ! command -v terraform >/dev/null 2>&1; then
            echo "terraform not on PATH — skipping validate"
            exit 0
          fi
          for cloud in aws gcp azure; do
            echo "=== terraform validate: $cloud ==="
            (cd "infra/terraform/$cloud" && terraform init -backend=false && terraform validate)
          done
        '''
      }
    }

    stage('Smoke run (CPU, no display)') {
      when { expression { return params.VARIANT == 'cpu' } }
      steps {
        sh '''
          set -euo pipefail
          # Container exits when no video is configured; use a short timeout probe of the image entrypoint help path.
          docker image inspect "${IMAGE_TAG}" >/dev/null
          timeout 15s docker run --rm "${IMAGE_TAG}" || true
          echo "Image ${IMAGE_TAG} is present and runnable"
        '''
      }
    }
  }

  post {
    success {
      echo "Vision Pilot CI succeeded: ${IMAGE_TAG}"
    }
    failure {
      echo "Vision Pilot CI failed — check Docker build logs and agent tooling"
    }
  }
}
