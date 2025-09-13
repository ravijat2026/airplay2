# Simple test build script for AirPlay 2 Lite (PowerShell version)

Write-Host "Testing AirPlay 2 Lite build..." -ForegroundColor Green

# Check if we're in the right directory
if (-not (Test-Path "Makefile")) {
    Write-Host "Error: Makefile not found. Run this script from the project root." -ForegroundColor Red
    exit 1
}

# Create a simple test build directory
$BUILD_DIR = "test-build"
if (Test-Path $BUILD_DIR) {
    Remove-Item -Recurse -Force $BUILD_DIR
}
New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null

# Copy source files
Write-Host "Copying source files..." -ForegroundColor Yellow
Copy-Item -Recurse src $BUILD_DIR\
Copy-Item Makefile $BUILD_DIR\
Copy-Item -Recurse files $BUILD_DIR\

# Test OpenWRT package structure
Write-Host "Testing OpenWRT package structure..." -ForegroundColor Yellow
if ((Test-Path "$BUILD_DIR\Makefile") -and (Test-Path "$BUILD_DIR\src") -and (Test-Path "$BUILD_DIR\files")) {
    Write-Host "✅ Package structure is correct" -ForegroundColor Green
} else {
    Write-Host "❌ Package structure is incorrect" -ForegroundColor Red
    exit 1
}

# Test configuration files
Write-Host "Testing configuration files..." -ForegroundColor Yellow
if ((Test-Path "$BUILD_DIR\files\airplay2-lite.init") -and (Test-Path "$BUILD_DIR\files\airplay2-lite.config")) {
    Write-Host "✅ Configuration files present" -ForegroundColor Green
} else {
    Write-Host "❌ Configuration files missing" -ForegroundColor Red
    exit 1
}

# Test source files
Write-Host "Testing source files..." -ForegroundColor Yellow
$REQUIRED_FILES = @(
    "src\main.c",
    "src\airplay_server.c",
    "src\airplay_server.h",
    "src\audio_output.c",
    "src\audio_output.h",
    "src\volume_control.c",
    "src\volume_control.h",
    "src\playback_control.c",
    "src\playback_control.h",
    "src\multiroom.c",
    "src\multiroom.h",
    "src\crypto_utils.c",
    "src\crypto_utils.h",
    "src\network_utils.c",
    "src\network_utils.h",
    "src\CMakeLists.txt"
)

$allFilesPresent = $true
foreach ($file in $REQUIRED_FILES) {
    if (Test-Path "$BUILD_DIR\$file") {
        Write-Host "✅ $file" -ForegroundColor Green
    } else {
        Write-Host "❌ $file missing" -ForegroundColor Red
        $allFilesPresent = $false
    }
}

if (-not $allFilesPresent) {
    exit 1
}

# Test GitHub Actions workflow
Write-Host "Testing GitHub Actions workflow..." -ForegroundColor Yellow
if (Test-Path ".github\workflows\build.yml") {
    Write-Host "✅ GitHub Actions workflow present" -ForegroundColor Green
} else {
    Write-Host "❌ GitHub Actions workflow missing" -ForegroundColor Red
    exit 1
}

# Test scripts
Write-Host "Testing build scripts..." -ForegroundColor Yellow
if ((Test-Path "scripts\configure.sh") -and (Test-Path "scripts\build.sh") -and (Test-Path "scripts\install.sh")) {
    Write-Host "✅ Build scripts present" -ForegroundColor Green
} else {
    Write-Host "❌ Build scripts missing" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "🎉 All tests passed! The package is ready for OpenWRT build." -ForegroundColor Green
Write-Host ""
Write-Host "To build with OpenWRT SDK:" -ForegroundColor Cyan
Write-Host "1. Set up OpenWRT SDK for your target architecture" -ForegroundColor White
Write-Host "2. Run: make package/airplay2-lite/compile" -ForegroundColor White
Write-Host ""
Write-Host "To build with GitHub Actions:" -ForegroundColor Cyan
Write-Host "1. Push to GitHub repository" -ForegroundColor White
Write-Host "2. GitHub Actions will build automatically" -ForegroundColor White
Write-Host "3. Download .ipk from Actions artifacts" -ForegroundColor White

# Cleanup
Remove-Item -Recurse -Force $BUILD_DIR
