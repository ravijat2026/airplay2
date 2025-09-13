# Verify OpenWRT SDK URLs

Write-Host "Verifying OpenWRT SDK URLs..." -ForegroundColor Green

$SDK_URLS = @{
    "mips_24kc" = "https://downloads.openwrt.org/releases/23.05.3/targets/ramips/mt7628/openwrt-sdk-23.05.3-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
    "mipsel_24kc" = "https://downloads.openwrt.org/releases/23.05.3/targets/ramips/mt7628/openwrt-sdk-23.05.3-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
    "arm_cortex-a7_neon-vfpv4" = "https://downloads.openwrt.org/releases/23.05.3/targets/armvirt/64/openwrt-sdk-23.05.3-armvirt-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
    "aarch64_cortex-a72" = "https://downloads.openwrt.org/releases/23.05.3/targets/armvirt/64/openwrt-sdk-23.05.3-armvirt-64_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
}

$allUrlsValid = $true

foreach ($target in $SDK_URLS.Keys) {
    $url = $SDK_URLS[$target]
    Write-Host "Testing $target..." -ForegroundColor Yellow
    Write-Host "URL: $url" -ForegroundColor Gray
    
    try {
        $response = Invoke-WebRequest -Uri $url -Method Head -TimeoutSec 10 -ErrorAction Stop
        if ($response.StatusCode -eq 200) {
            Write-Host "✅ $target - URL is accessible" -ForegroundColor Green
        } else {
            Write-Host "❌ $target - HTTP $($response.StatusCode)" -ForegroundColor Red
            $allUrlsValid = $false
        }
    } catch {
        Write-Host "❌ $target - Error: $($_.Exception.Message)" -ForegroundColor Red
        $allUrlsValid = $false
    }
    Write-Host ""
}

if ($allUrlsValid) {
    Write-Host "🎉 All SDK URLs are accessible!" -ForegroundColor Green
} else {
    Write-Host "⚠️ Some SDK URLs are not accessible" -ForegroundColor Yellow
    Write-Host "This may cause build failures in GitHub Actions" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Alternative approach:" -ForegroundColor Cyan
Write-Host "If URLs are not accessible, you can:" -ForegroundColor White
Write-Host "1. Use a different OpenWRT release version" -ForegroundColor White
Write-Host "2. Build locally with your own SDK" -ForegroundColor White
Write-Host "3. Use a different SDK source" -ForegroundColor White
