#!/bin/bash

# AirPlay 2 Lite Configuration Script for OpenWRT

set -e

# Default configuration
DEVICE_NAME="OpenWRT AirPlay"
MODEL_NAME="OpenWRT"
DEVICE_ID="OpenWRT-AirPlay-001"
PORT=7000
ENABLE_MULTIROOM=0
MULTIROOM_GROUP="default-group"
AUDIO_DEVICE="default"
SAMPLE_RATE=44100
CHANNELS=2
BITS_PER_SAMPLE=16
BUFFER_SIZE=4096
USE_HW_VOLUME=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --device-name)
            DEVICE_NAME="$2"
            shift 2
            ;;
        --model-name)
            MODEL_NAME="$2"
            shift 2
            ;;
        --device-id)
            DEVICE_ID="$2"
            shift 2
            ;;
        --port)
            PORT="$2"
            shift 2
            ;;
        --enable-multiroom)
            ENABLE_MULTIROOM=1
            shift
            ;;
        --multiroom-group)
            MULTIROOM_GROUP="$2"
            shift 2
            ;;
        --audio-device)
            AUDIO_DEVICE="$2"
            shift 2
            ;;
        --sample-rate)
            SAMPLE_RATE="$2"
            shift 2
            ;;
        --channels)
            CHANNELS="$2"
            shift 2
            ;;
        --bits-per-sample)
            BITS_PER_SAMPLE="$2"
            shift 2
            ;;
        --buffer-size)
            BUFFER_SIZE="$2"
            shift 2
            ;;
        --use-hw-volume)
            USE_HW_VOLUME=1
            shift
            ;;
        --help)
            echo "AirPlay 2 Lite Configuration Script"
            echo ""
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --device-name NAME        Set device name (default: OpenWRT AirPlay)"
            echo "  --model-name NAME         Set model name (default: OpenWRT)"
            echo "  --device-id ID            Set device ID (default: OpenWRT-AirPlay-001)"
            echo "  --port PORT               Set server port (default: 7000)"
            echo "  --enable-multiroom        Enable multi-room support"
            echo "  --multiroom-group GROUP   Set multi-room group (default: default-group)"
            echo "  --audio-device DEVICE     Set audio device (default: default)"
            echo "  --sample-rate RATE        Set sample rate (default: 44100)"
            echo "  --channels CHANNELS       Set audio channels (default: 2)"
            echo "  --bits-per-sample BITS    Set bit depth (default: 16)"
            echo "  --buffer-size SIZE        Set buffer size (default: 4096)"
            echo "  --use-hw-volume           Enable hardware volume control"
            echo "  --help                    Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Create configuration file
cat > files/airplay2-lite.config << EOF
config airplay2_lite 'main'
    option device_name '$DEVICE_NAME'
    option model_name '$MODEL_NAME'
    option device_id '$DEVICE_ID'
    option port '$PORT'
    option enable_multiroom '$ENABLE_MULTIROOM'
    option multiroom_group '$MULTIROOM_GROUP'
    option audio_device '$AUDIO_DEVICE'
    option sample_rate '$SAMPLE_RATE'
    option channels '$CHANNELS'
    option bits_per_sample '$BITS_PER_SAMPLE'
    option buffer_size '$BUFFER_SIZE'
    option use_hw_volume '$USE_HW_VOLUME'
EOF

echo "Configuration updated:"
echo "  Device Name: $DEVICE_NAME"
echo "  Model Name: $MODEL_NAME"
echo "  Device ID: $DEVICE_ID"
echo "  Port: $PORT"
echo "  Multi-room: $([ $ENABLE_MULTIROOM -eq 1 ] && echo "Enabled" || echo "Disabled")"
echo "  Multi-room Group: $MULTIROOM_GROUP"
echo "  Audio Device: $AUDIO_DEVICE"
echo "  Sample Rate: $SAMPLE_RATE Hz"
echo "  Channels: $CHANNELS"
echo "  Bit Depth: $BITS_PER_SAMPLE bits"
echo "  Buffer Size: $BUFFER_SIZE bytes"
echo "  Hardware Volume: $([ $USE_HW_VOLUME -eq 1 ] && echo "Enabled" || echo "Disabled")"
