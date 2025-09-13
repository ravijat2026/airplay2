# AirPlay 2 Lite for OpenWRT

A lightweight AirPlay 2 server implementation optimized for OpenWRT routers with limited resources. This package provides full AirPlay 2 support with minimal memory and CPU usage.

## Features

- ✅ **Full AirPlay 2 Support** - Compatible with iOS, macOS, and other AirPlay clients
- ✅ **Volume Control** - Hardware and software volume control
- ✅ **Multi-room Support** - Synchronized audio across multiple devices
- ✅ **Playback Controls** - Play, pause, stop, next, previous
- ✅ **Optimized for Low Resources** - Designed for 128MB RAM, 580MHz CPU
- ✅ **OpenWRT Integration** - Native OpenWRT package with init scripts

## System Requirements

- **CPU**: MIPS (MediaTek MT7628AN) or ARM
- **RAM**: 128MB DDR2 (minimum)
- **Storage**: 16MB NOR flash (32MB recommended)
- **OpenWRT**: Version 23.05.5 or later
- **Audio**: ALSA-compatible audio device

## Installation

### Method 1: Download Pre-built Package

1. Download the appropriate `.ipk` file for your architecture from the [Releases](https://github.com/yourusername/airplay2-lite/releases) page
2. Upload to your OpenWRT router
3. Install using opkg:

```bash
opkg install airplay2-lite_1.0.0-1_mips_24kc.ipk
```

### Method 2: Build from Source

1. Clone this repository
2. Set up OpenWRT SDK for your target architecture
3. Build the package:

```bash
make package/airplay2-lite/compile
```

## Configuration

The package includes a UCI configuration file at `/etc/config/airplay2-lite`:

```bash
config airplay2_lite 'main'
    option device_name 'OpenWRT AirPlay'
    option model_name 'OpenWRT'
    option device_id 'OpenWRT-AirPlay-001'
    option port '7000'
    option enable_multiroom '0'
    option multiroom_group 'default-group'
    option audio_device 'default'
    option sample_rate '44100'
    option channels '2'
    option bits_per_sample '16'
    option buffer_size '4096'
    option use_hw_volume '0'
```

### Configuration Options

- `device_name`: Name shown in AirPlay client
- `model_name`: Device model identifier
- `device_id`: Unique device identifier
- `port`: AirPlay server port (default: 7000)
- `enable_multiroom`: Enable multi-room audio (0/1)
- `multiroom_group`: Multi-room group identifier
- `audio_device`: ALSA audio device name
- `sample_rate`: Audio sample rate (44100/48000)
- `channels`: Audio channels (1/2)
- `bits_per_sample`: Audio bit depth (16/24/32)
- `buffer_size`: Audio buffer size in bytes
- `use_hw_volume`: Use hardware volume control (0/1)

## Usage

### Start/Stop Service

```bash
# Start service
/etc/init.d/airplay2-lite start

# Stop service
/etc/init.d/airplay2-lite stop

# Restart service
/etc/init.d/airplay2-lite restart

# Enable auto-start
/etc/init.d/airplay2-lite enable
```

### Manual Execution

```bash
# Run in foreground (for debugging)
airplay2-lite -d

# Run as daemon
airplay2-lite -f
```

### Logs

View service logs:

```bash
logread | grep airplay2-lite
```

## Multi-room Setup

1. Enable multiroom in configuration:
```bash
uci set airplay2-lite.main.enable_multiroom=1
uci commit airplay2-lite
```

2. Configure multiple devices with the same group:
```bash
uci set airplay2-lite.main.multiroom_group=my-room
uci commit airplay2-lite
```

3. Restart the service on all devices

## Troubleshooting

### Audio Issues

1. Check ALSA configuration:
```bash
aplay -l
```

2. Test audio output:
```bash
speaker-test -c 2 -t wav
```

3. Check audio permissions:
```bash
ls -la /dev/snd/
```

### Network Issues

1. Check firewall rules:
```bash
iptables -L | grep 7000
```

2. Verify mDNS/Bonjour:
```bash
avahi-browse -a
```

3. Test port availability:
```bash
netstat -ln | grep 7000
```

### Performance Issues

1. Monitor resource usage:
```bash
top | grep airplay2-lite
```

2. Check memory usage:
```bash
free -m
```

3. Adjust buffer size in configuration

## Development

### Building

1. Install OpenWRT SDK for your target
2. Clone this repository
3. Build:

```bash
make package/airplay2-lite/compile V=s
```

### Dependencies

- libopenssl
- libavahi-client
- libavahi-common
- libdaemon
- alsa-lib

### Architecture Support

- MIPS (mips_24kc) - MediaTek MT7628AN
- MIPSEL (mipsel_24kc) - Various MIPSEL devices
- ARM (arm_cortex-a7_neon-vfpv4) - ARM Cortex-A7
- AArch64 (aarch64_cortex-a72) - ARM Cortex-A72

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on target hardware
5. Submit a pull request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Based on Shairport-Sync architecture
- Uses OpenWRT build system
- Compatible with AirPlay 2 protocol

## Support

For issues and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review OpenWRT documentation

## Changelog

### Version 1.0.0
- Initial release
- Full AirPlay 2 support
- Multi-room audio
- Volume control
- Playback controls
- OpenWRT integration
