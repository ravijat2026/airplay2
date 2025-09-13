#!/bin/bash

# AirPlay 2 Lite Installation Script for OpenWRT

set -e

# Default values
PACKAGE_FILE=""
FORCE_INSTALL=0
BACKUP_CONFIG=1

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --package)
            PACKAGE_FILE="$2"
            shift 2
            ;;
        --force)
            FORCE_INSTALL=1
            shift
            ;;
        --no-backup)
            BACKUP_CONFIG=0
            shift
            ;;
        --help)
            echo "AirPlay 2 Lite Installation Script"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --package FILE            Package file to install"
            echo "  --force                   Force installation (overwrite existing)"
            echo "  --no-backup               Don't backup existing configuration"
            echo "  --help                    Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 --package airplay2-lite_1.0.0-1_mips_24kc.ipk"
            echo "  $0 --package airplay2-lite_1.0.0-1_mips_24kc.ipk --force"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Check if running on OpenWRT
if [ ! -f /etc/openwrt_release ]; then
    echo "Error: This script must be run on an OpenWRT system"
    exit 1
fi

# Check if package file is provided
if [ -z "$PACKAGE_FILE" ]; then
    echo "Error: Package file must be specified"
    echo "Use --help for usage information"
    exit 1
fi

# Check if package file exists
if [ ! -f "$PACKAGE_FILE" ]; then
    echo "Error: Package file '$PACKAGE_FILE' not found"
    exit 1
fi

# Check if already installed
if opkg list-installed | grep -q "airplay2-lite" && [ $FORCE_INSTALL -eq 0 ]; then
    echo "AirPlay 2 Lite is already installed. Use --force to reinstall."
    exit 1
fi

# Backup existing configuration
if [ $BACKUP_CONFIG -eq 1 ] && [ -f /etc/config/airplay2-lite ]; then
    echo "Backing up existing configuration..."
    cp /etc/config/airplay2-lite /etc/config/airplay2-lite.backup.$(date +%Y%m%d_%H%M%S)
fi

# Stop existing service
if [ -f /etc/init.d/airplay2-lite ]; then
    echo "Stopping existing service..."
    /etc/init.d/airplay2-lite stop || true
fi

# Remove existing package if force install
if [ $FORCE_INSTALL -eq 1 ]; then
    echo "Removing existing package..."
    opkg remove airplay2-lite || true
fi

# Install package
echo "Installing package: $PACKAGE_FILE"
opkg install "$PACKAGE_FILE"

# Check installation
if ! opkg list-installed | grep -q "airplay2-lite"; then
    echo "Error: Installation failed"
    exit 1
fi

# Configure service
echo "Configuring service..."
/etc/init.d/airplay2-lite enable

# Start service
echo "Starting service..."
/etc/init.d/airplay2-lite start

# Check service status
if /etc/init.d/airplay2-lite status > /dev/null 2>&1; then
    echo "Installation completed successfully!"
    echo ""
    echo "Service status:"
    /etc/init.d/airplay2-lite status
    echo ""
    echo "Configuration file: /etc/config/airplay2-lite"
    echo "Logs: logread | grep airplay2-lite"
    echo ""
    echo "To configure:"
    echo "  uci set airplay2-lite.main.device_name='Your Device Name'"
    echo "  uci commit airplay2-lite"
    echo "  /etc/init.d/airplay2-lite restart"
else
    echo "Warning: Service failed to start. Check logs for details."
    echo "Run: logread | grep airplay2-lite"
    exit 1
fi
