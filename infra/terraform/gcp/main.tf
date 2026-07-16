terraform {
  required_version = ">= 1.5.0"
  required_providers {
    google = {
      source  = "hashicorp/google"
      version = "~> 5.0"
    }
  }
}

provider "google" {
  project = var.project_id
  region  = var.region
}

variable "project_id" {
  type = string
}

variable "region" {
  type    = string
  default = "us-central1"
}

variable "name_prefix" {
  type    = string
  default = "vision-pilot"
}

variable "create_ci_vm" {
  type    = bool
  default = false
}

resource "google_storage_bucket" "artifacts" {
  name                        = "${var.name_prefix}-artifacts-${var.project_id}"
  location                    = var.region
  uniform_bucket_level_access = true
  versioning {
    enabled = true
  }
  force_destroy = false
}

resource "google_artifact_registry_repository" "docker" {
  location      = var.region
  repository_id = "${var.name_prefix}-docker"
  description   = "Vision Pilot container images"
  format        = "DOCKER"
}

resource "google_compute_firewall" "ci" {
  name    = "${var.name_prefix}-ci"
  network = "default"
  allow {
    protocol = "tcp"
    ports    = ["22", "8080", "50000"]
  }
  source_ranges = var.allowed_cidr
  target_tags   = ["${var.name_prefix}-ci"]
}

resource "google_compute_instance" "jenkins_agent" {
  count        = var.create_ci_vm ? 1 : 0
  name         = "${var.name_prefix}-jenkins-agent"
  machine_type = var.machine_type
  zone         = "${var.region}-a"
  tags         = ["${var.name_prefix}-ci"]

  boot_disk {
    initialize_params {
      image = "ubuntu-os-cloud/ubuntu-2404-lts-amd64"
      size  = 100
    }
  }

  network_interface {
    network = "default"
    access_config {}
  }

  metadata_startup_script = file("${path.module}/../modules/cloud_init_ci.sh")
}

variable "allowed_cidr" {
  type    = list(string)
  default = ["0.0.0.0/0"]
}

variable "machine_type" {
  type    = string
  default = "e2-standard-4"
}

output "artifact_registry" {
  value = "${var.region}-docker.pkg.dev/${var.project_id}/${google_artifact_registry_repository.docker.repository_id}"
}

output "artifacts_bucket" {
  value = google_storage_bucket.artifacts.name
}

output "jenkins_agent_ip" {
  value = try(google_compute_instance.jenkins_agent[0].network_interface[0].access_config[0].nat_ip, null)
}
