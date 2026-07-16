terraform {
  required_version = ">= 1.5.0"
  required_providers {
    azurerm = {
      source  = "hashicorp/azurerm"
      version = "~> 4.0"
    }
  }
}

provider "azurerm" {
  features {}
}

variable "location" {
  type    = string
  default = "eastus"
}

variable "name_prefix" {
  type    = string
  default = "visionpilot"
}

variable "create_ci_vm" {
  type    = bool
  default = false
}

resource "azurerm_resource_group" "rg" {
  name     = "${var.name_prefix}-rg"
  location = var.location
}

resource "azurerm_storage_account" "artifacts" {
  name                     = substr(replace("${var.name_prefix}artifacts", "-", ""), 0, 24)
  resource_group_name      = azurerm_resource_group.rg.name
  location                 = azurerm_resource_group.rg.location
  account_tier             = "Standard"
  account_replication_type = "LRS"
  min_tls_version          = "TLS1_2"
}

resource "azurerm_storage_container" "artifacts" {
  name                  = "vision-pilot"
  storage_account_id    = azurerm_storage_account.artifacts.id
  container_access_type = "private"
}

resource "azurerm_container_registry" "acr" {
  name                = substr(replace("${var.name_prefix}acr", "-", ""), 0, 50)
  resource_group_name = azurerm_resource_group.rg.name
  location            = azurerm_resource_group.rg.location
  sku                 = "Basic"
  admin_enabled       = false
}

resource "azurerm_virtual_network" "vnet" {
  count               = var.create_ci_vm ? 1 : 0
  name                = "${var.name_prefix}-vnet"
  address_space       = ["10.40.0.0/16"]
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
}

resource "azurerm_subnet" "ci" {
  count                = var.create_ci_vm ? 1 : 0
  name                 = "ci"
  resource_group_name  = azurerm_resource_group.rg.name
  virtual_network_name = azurerm_virtual_network.vnet[0].name
  address_prefixes     = ["10.40.1.0/24"]
}

resource "azurerm_network_security_group" "ci" {
  count               = var.create_ci_vm ? 1 : 0
  name                = "${var.name_prefix}-ci-nsg"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name

  security_rule {
    name                       = "ssh"
    priority                   = 100
    direction                  = "Inbound"
    access                     = "Allow"
    protocol                   = "Tcp"
    source_port_range          = "*"
    destination_port_range     = "22"
    source_address_prefix      = "*"
    destination_address_prefix = "*"
  }

  security_rule {
    name                       = "jenkins"
    priority                   = 110
    direction                  = "Inbound"
    access                     = "Allow"
    protocol                   = "Tcp"
    source_port_range          = "*"
    destination_port_range     = "8080"
    source_address_prefix      = "*"
    destination_address_prefix = "*"
  }
}

resource "azurerm_public_ip" "ci" {
  count               = var.create_ci_vm ? 1 : 0
  name                = "${var.name_prefix}-ci-pip"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name
  allocation_method   = "Static"
  sku                 = "Standard"
}

resource "azurerm_network_interface" "ci" {
  count               = var.create_ci_vm ? 1 : 0
  name                = "${var.name_prefix}-ci-nic"
  location            = azurerm_resource_group.rg.location
  resource_group_name = azurerm_resource_group.rg.name

  ip_configuration {
    name                          = "internal"
    subnet_id                     = azurerm_subnet.ci[0].id
    private_ip_address_allocation = "Dynamic"
    public_ip_address_id          = azurerm_public_ip.ci[0].id
  }
}

resource "azurerm_network_interface_security_group_association" "ci" {
  count                     = var.create_ci_vm ? 1 : 0
  network_interface_id      = azurerm_network_interface.ci[0].id
  network_security_group_id = azurerm_network_security_group.ci[0].id
}

resource "azurerm_linux_virtual_machine" "jenkins_agent" {
  count                 = var.create_ci_vm ? 1 : 0
  name                  = "${var.name_prefix}-jenkins-agent"
  resource_group_name   = azurerm_resource_group.rg.name
  location              = azurerm_resource_group.rg.location
  size                  = var.vm_size
  admin_username        = "azureuser"
  network_interface_ids = [azurerm_network_interface.ci[0].id]
  custom_data           = base64encode(file("${path.module}/../modules/cloud_init_ci.sh"))

  admin_ssh_key {
    username   = "azureuser"
    public_key = var.admin_ssh_public_key
  }

  os_disk {
    caching              = "ReadWrite"
    storage_account_type = "Premium_LRS"
  }

  source_image_reference {
    publisher = "Canonical"
    offer     = "ubuntu-24_04-lts"
    sku       = "server"
    version   = "latest"
  }
}

variable "vm_size" {
  type    = string
  default = "Standard_D4s_v5"
}

variable "admin_ssh_public_key" {
  type    = string
  default = ""
}

output "acr_login_server" {
  value = azurerm_container_registry.acr.login_server
}

output "storage_account" {
  value = azurerm_storage_account.artifacts.name
}

output "jenkins_agent_public_ip" {
  value = try(azurerm_public_ip.ci[0].ip_address, null)
}
