# Terraform — Vision Pilot cloud foundations

Provision artifact storage + container registries (and optional CI VMs) on:

| Cloud | Path | Defaults |
|-------|------|----------|
| AWS | `aws/` | S3 + ECR (+ optional EC2) |
| GCP | `gcp/` | GCS + Artifact Registry (+ optional GCE) |
| Azure | `azure/` | Storage + ACR (+ optional VM) |

## Quick start

```bash
# AWS
cd infra/terraform/aws
terraform init
terraform plan
# terraform apply -var='create_ci_instance=true'

# GCP
cd infra/terraform/gcp
terraform init
terraform plan -var='project_id=YOUR_GCP_PROJECT'
# terraform apply -var='project_id=...' -var='create_ci_vm=true'

# Azure
cd infra/terraform/azure
terraform init
terraform plan
# terraform apply -var='create_ci_vm=true' -var='admin_ssh_public_key=ssh-rsa AAAA...'
```

CI VMs are **off by default** (`create_ci_* = false`) so `plan` is safe without launching compute.
