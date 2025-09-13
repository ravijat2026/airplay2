# Test CMakeLists.txt for AirPlay 2 Lite

Write-Host "Testing CMakeLists.txt..." -ForegroundColor Green

# Check if CMakeLists.txt exists
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "❌ CMakeLists.txt not found in root directory" -ForegroundColor Red
    exit 1
}

Write-Host "✅ CMakeLists.txt found in root directory" -ForegroundColor Green

# Check if source directory exists
if (-not (Test-Path "src")) {
    Write-Host "❌ src directory not found" -ForegroundColor Red
    exit 1
}

Write-Host "✅ src directory found" -ForegroundColor Green

# Check if all source files exist
$SOURCE_FILES = @(
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
    "src\network_utils.h"
)

$allFilesPresent = $true
foreach ($file in $SOURCE_FILES) {
    if (Test-Path $file) {
        Write-Host "✅ $file" -ForegroundColor Green
    } else {
        Write-Host "❌ $file missing" -ForegroundColor Red
        $allFilesPresent = $false
    }
}

if (-not $allFilesPresent) {
    Write-Host "❌ Some source files are missing" -ForegroundColor Red
    exit 1
}

# Check CMakeLists.txt content
$cmakeContent = Get-Content "CMakeLists.txt" -Raw
if ($cmakeContent -match "src/main.c") {
    Write-Host "✅ CMakeLists.txt references src/main.c" -ForegroundColor Green
} else {
    Write-Host "❌ CMakeLists.txt does not reference src/main.c" -ForegroundColor Red
    exit 1
}

if ($cmakeContent -match "include_directories\(src\)") {
    Write-Host "✅ CMakeLists.txt includes src directory" -ForegroundColor Green
} else {
    Write-Host "❌ CMakeLists.txt does not include src directory" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "🎉 CMakeLists.txt test passed!" -ForegroundColor Green
Write-Host "The package structure is correct for OpenWRT build." -ForegroundColor Cyan
