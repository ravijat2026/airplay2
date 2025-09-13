# AirPlay 2 Lite - Troubleshooting Guide

## Build Issues

### GitHub Actions Build Failures

#### Common Issues and Solutions

**1. SDK Download Failure**
```
Error: Failed to download OpenWRT SDK
```
**Solution:**
- Check if the SDK URL is correct and accessible
- Verify the OpenWRT version (23.05.5) is still available
- Update the SDK URL in `.github/workflows/build.yml` if needed

**2. Package Dependencies Missing**
```
Error: Package libopenssl not found
```
**Solution:**
- Ensure all required packages are enabled in the build configuration
- Check that the SDK includes the necessary libraries
- Verify the package names in the Makefile dependencies

**3. Compilation Errors**
```
Error: undefined reference to 'avahi_client_new'
```
**Solution:**
- Check that Avahi libraries are properly linked
- Verify the pkg-config configuration
- Ensure all required headers are included

**4. Architecture-Specific Issues**
```
Error: mips_24kc compilation failed
```
**Solution:**
- Check compiler flags for MIPS architecture
- Verify the target-specific optimizations
- Ensure the SDK supports the target architecture

### Local Build Issues

**1. Missing OpenWRT SDK**
```bash
Error: OpenWRT SDK not found
```
**Solution:**
```bash
# Download the appropriate SDK for your target
wget https://downloads.openwrt.org/releases/23.05.5/targets/ramips/mt7628/openwrt-sdk-23.05.5-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64.tar.xz
tar -xf openwrt-sdk-23.05.5-ramips-mt7628_gcc-12.3.0_musl.Linux-x86_64.tar.xz
```

**2. Build Environment Issues**
```bash
Error: make: command not found
```
**Solution:**
```bash
# Install required build tools
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config
```

**3. Cross-compilation Issues**
```bash
Error: cannot find -lavahi-client
```
**Solution:**
- Ensure the OpenWRT SDK is properly set up
- Check that the cross-compilation environment is configured
- Verify library paths in the SDK

## Runtime Issues

### Service Won't Start

**1. Missing Dependencies**
```bash
Error: airplay2-lite: command not found
```
**Solution:**
```bash
# Install required packages
opkg update
opkg install libopenssl libavahi-client libavahi-common libdaemon alsa-lib
```

**2. Permission Issues**
```bash
Error: Permission denied
```
**Solution:**
```bash
# Check file permissions
ls -la /usr/bin/airplay2-lite
chmod +x /usr/bin/airplay2-lite

# Check audio device permissions
ls -la /dev/snd/
```

**3. Configuration Issues**
```bash
Error: Invalid configuration
```
**Solution:**
```bash
# Check configuration file
cat /etc/config/airplay2-lite

# Validate configuration
uci show airplay2-lite
```

### Audio Issues

**1. No Audio Output**
```bash
# Check ALSA configuration
aplay -l
speaker-test -c 2 -t wav

# Check audio device
cat /proc/asound/cards
```

**2. Audio Distortion**
```bash
# Check buffer settings
uci get airplay2-lite.main.buffer_size

# Adjust buffer size
uci set airplay2-lite.main.buffer_size=8192
uci commit airplay2-lite
/etc/init.d/airplay2-lite restart
```

**3. Volume Control Issues**
```bash
# Check hardware volume control
uci get airplay2-lite.main.use_hw_volume

# Enable hardware volume
uci set airplay2-lite.main.use_hw_volume=1
uci commit airplay2-lite
```

### Network Issues

**1. AirPlay Not Discoverable**
```bash
# Check mDNS/Bonjour
avahi-browse -a

# Check firewall
iptables -L | grep 7000

# Check port binding
netstat -ln | grep 7000
```

**2. Connection Issues**
```bash
# Check service status
/etc/init.d/airplay2-lite status

# Check logs
logread | grep airplay2-lite

# Test port connectivity
telnet localhost 7000
```

**3. Multi-room Issues**
```bash
# Check multi-room configuration
uci get airplay2-lite.main.enable_multiroom
uci get airplay2-lite.main.multiroom_group

# Enable multi-room
uci set airplay2-lite.main.enable_multiroom=1
uci set airplay2-lite.main.multiroom_group=my-room
uci commit airplay2-lite
```

## Performance Issues

### High CPU Usage

**1. Optimize Buffer Settings**
```bash
# Reduce buffer size for lower latency
uci set airplay2-lite.main.buffer_size=2048

# Adjust sample rate
uci set airplay2-lite.main.sample_rate=44100
```

**2. Check System Resources**
```bash
# Monitor CPU usage
top | grep airplay2-lite

# Check memory usage
free -m

# Check disk usage
df -h
```

### High Memory Usage

**1. Reduce Buffer Sizes**
```bash
# Minimize audio buffer
uci set airplay2-lite.main.buffer_size=1024

# Use mono audio
uci set airplay2-lite.main.channels=1
```

**2. Optimize Configuration**
```bash
# Disable multi-room if not needed
uci set airplay2-lite.main.enable_multiroom=0

# Use lower bit depth
uci set airplay2-lite.main.bits_per_sample=16
```

## Debug Commands

### System Information
```bash
# Check OpenWRT version
cat /etc/openwrt_release

# Check architecture
uname -m

# Check available memory
free -m

# Check CPU info
cat /proc/cpuinfo
```

### Service Debugging
```bash
# Run in foreground for debugging
airplay2-lite -d

# Check service logs
logread -f | grep airplay2-lite

# Check system logs
dmesg | grep airplay
```

### Network Debugging
```bash
# Check network interfaces
ip addr show

# Check routing table
ip route show

# Check DNS resolution
nslookup _airplay._tcp.local
```

### Audio Debugging
```bash
# List audio devices
aplay -l

# Test audio output
speaker-test -c 2 -t wav -l 1

# Check ALSA configuration
cat /etc/asound.conf
```

## Common Solutions

### Reset Configuration
```bash
# Reset to defaults
rm /etc/config/airplay2-lite
cp /etc/config/airplay2-lite.default /etc/config/airplay2-lite
/etc/init.d/airplay2-lite restart
```

### Reinstall Package
```bash
# Remove package
opkg remove airplay2-lite

# Clean up
rm -rf /etc/config/airplay2-lite
rm -rf /etc/init.d/airplay2-lite

# Reinstall
opkg install airplay2-lite_*.ipk
```

### Update Package
```bash
# Download latest version
wget https://github.com/yourusername/airplay2-lite/releases/latest/download/airplay2-lite_*.ipk

# Update package
opkg install airplay2-lite_*.ipk --force-reinstall
```

## Getting Help

### Log Collection
```bash
# Collect system information
cat /etc/openwrt_release > debug.log
uname -a >> debug.log
free -m >> debug.log

# Collect service logs
logread >> debug.log
dmesg >> debug.log

# Collect configuration
uci show airplay2-lite >> debug.log
```

### Reporting Issues
When reporting issues, please include:
1. OpenWRT version and architecture
2. Hardware specifications
3. Configuration file contents
4. Service logs
5. Steps to reproduce the issue

### Community Support
- GitHub Issues: [Repository Issues](https://github.com/yourusername/airplay2-lite/issues)
- OpenWRT Forum: [OpenWRT Community](https://forum.openwrt.org/)
- Documentation: [Project README](README.md)
