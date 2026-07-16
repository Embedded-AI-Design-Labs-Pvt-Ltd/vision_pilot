# Jenkins for Vision Pilot

## Local (Docker Compose)

```bash
cd VisionPilot/docker
docker compose --profile jenkins up -d jenkins
# UI: http://localhost:8081
```

Point a Pipeline job at `infra/jenkins/Jenkinsfile` (SCM: this repo).

## Agents

Cloud CI hosts can be created with Terraform (`infra/terraform/*/`) using
`create_ci_instance` / `create_ci_vm`. Those VMs install Docker + Terraform + JDK via
`infra/terraform/modules/cloud_init_ci.sh`.

## Required agent tools

- Docker Engine + buildx
- Git, bash
- Optional: Terraform CLI, pre-commit
