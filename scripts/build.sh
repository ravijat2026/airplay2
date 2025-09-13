#!/bin/bash

# AirPlay 2 Lite Build Script for OpenWRT

set -e

# Default values
TARGET="mips_24kc"
SUBTARGET="mt7628"
SDK_URL=""
SDK_DIR=""
BUILD_DIR="build"
VERBOSE=0
CLEAN=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --target)
            TARGET="$2"
            shift 2
            ;;
        --subtarget)
            SUBTARGET="$2"
            shift 2
            ;;
        --sdk-url)
            SDK_URL="$2"
            shift 2
            ;;
        --sdk-dir)
            SDK_DIR="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=1
            shift
            ;;
        --clean)
            CLEAN=1
            shift
            ;;
        --help)
            echo "AirPlay 2 Lite Build Script"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --target TARGET           Target architecture (default: mips_24kc)"
            echo "  --subtarget SUBTARGET     Target subtarget (default: mt7628)"
            echo "  --sdk-url URL             OpenWRT SDK download URL"
            echo "  --sdk-dir DIR             OpenWRT SDK directory"
            echo "  --build-dir DIR           Build directory (default: build)"
            echo "  --verbose                 Enable verbose output"
            echo "  --clean                   Clean build directory"
            echo "  --help                    Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0 --target mips_24kc --subtarget mt7628"
            echo "  $0 --target arm_cortex-a7_neon-vfpv4"
            echo "  $0 --sdk-dir /path/to/openwrt-sdk"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Function to download SDK
download_sdk() {
    if [ -z "$SDK_URL" ]; then
        case "$TARGET" in
            mips_24kc)
                SDK_URL="https://downloads.openwrt.org/releases/23.05.5/targets/ramips/mt7628/openwrt-sdk-23.05.5-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
                SDK_DIR="openwrt-sdk-23.05.5-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64"
                ;;
            mipsel_24kc)
                SDK_URL="https://downloads.openwrt.org/releases/23.05.5/targets/ramips/mt7628/openwrt-sdk-23.05.5-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
                SDK_DIR="openwrt-sdk-23.05.5-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64"
                ;;
            arm_cortex-a7_neon-vfpv4)
                SDK_URL="https://downloads.openwrt.org/releases/23.05.5/targets/armvirt/64/openwrt-sdk-23.05.5-armvirt-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
                SDK_DIR="openwrt-sdk-23.05.5-armvirt-64_gcc-12.3.0_musl.Linux-x86_64"
                ;;
            aarch64_cortex-a72)
                SDK_URL="https://downloads.openwrt.org/releases/23.05.5/targets/armvirt/64/openwrt-sdk-23.05.5-armvirt-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
                SDK_DIR="openwrt-sdk-23.05.5-armvirt-64_gcc-12.3.0_musl.Linux-x86_64"
                ;;
            *)
                echo "Unknown target: $TARGET"
                exit 1
                ;;
        esac
    fi
    
    echo "Downloading OpenWRT SDK..."
    wget -q "$SDK_URL" -O "$(basename "$SDK_URL")"
    
    echo "Extracting SDK..."
    tar -xf "$(basename "$SDK_URL")"
    
    if [ -z "$SDK_DIR" ]; then
        SDK_DIR="$(basename "$SDK_URL" .tar.xz)"
    fi
    
    mv "$SDK_DIR" "$BUILD_DIR/openwrt-sdk"
}

# Clean build directory
if [ $CLEAN -eq 1 ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Download SDK if not provided
if [ -z "$SDK_DIR" ]; then
    download_sdk
    SDK_DIR="$BUILD_DIR/openwrt-sdk"
fi

# Copy source to SDK
echo "Copying source to SDK..."
cp -r . "$SDK_DIR/package/airplay2-lite"

# Setup feeds
echo "Setting up feeds..."
cd "$SDK_DIR"
echo "src-link airplay2-lite package/airplay2-lite" >> feeds.conf.default
./scripts/feeds update -a
./scripts/feeds install -a

# Configure build
echo "Configuring build..."
make defconfig
echo "CONFIG_PACKAGE_airplay2-lite=y" >> .config
make defconfig

# Build package
echo "Building package..."
if [ $VERBOSE -eq 1 ]; then
    make package/airplay2-lite/compile V=s
else
    make package/airplay2-lite/compile
fi

# Copy built package
echo "Copying built package..."
cp bin/packages/*/base/airplay2-lite_*.ipk "$BUILD_DIR/"

echo "Build completed successfully!"
echo "Package location: $BUILD_DIR/airplay2-lite_*.ipk"
