# ESP-NOW Tree Network Web Interface Setup

## Overview
This firmware has been enhanced with serial command handling to support the ESP32 Device Manager web interface. The web interface can now configure and monitor ESP-NOW Tree Network devices through serial communication.

## Required Dependencies

### Arduino Libraries
Install these libraries through the Arduino Library Manager:

1. **ArduinoJson** (version 6.x)
   - Required for JSON command/response handling
   - Install via: Tools → Manage Libraries → Search "ArduinoJson" → Install

2. **WiFi** (built-in with ESP32 board package)
   - Required for MAC address and RSSI functionality

3. **Preferences** (built-in with ESP32 board package)
   - Required for NVS storage of configuration

## New Features Added

### Serial Command Handler
- **File**: `SerialCommandHandler.h` and `SerialCommandHandler.cpp`
- **Purpose**: Handles serial commands from the web interface
- **Integration**: Automatically initialized in `setup()` and updated in `loop()`

### Supported Commands

#### Configuration Commands
- `CONFIG_SCHEMA` - Returns the configuration schema for the web interface
- `CONFIG_SAVE <json>` - Saves configuration from web interface
- `CONFIG_LOAD` - Returns current configuration values

#### System Commands
- `RESTART` - Restarts the ESP32 device
- `STATUS` - Returns device status information

#### Monitoring Commands
- `NETWORK_STATUS` - Returns network topology and configuration status
- `NETWORK_STATS` - Returns network performance statistics
- `IO_STATUS` - Returns current I/O states and shared data
- `DEVICE_DATA` - Returns device-specific data values

### Response Format
All responses are prefixed with either:
- `RESPONSE: <message>` - For simple text responses
- `JSON_RESPONSE: <json>` - For structured JSON data

## Configuration Parameters

### Network Identity (Editable)
- **Hierarchical ID (HID)**: 1-999 range, device position in tree structure
- **Bit Index**: 0-31 range, assigned bit position in shared 32-bit data
- **Device Name**: Human-readable device identifier

### System Behavior (Editable)
- **Debug Logging Level**: None/Basic/Detailed/Verbose
- **Status Update Interval**: 100-5000ms
- **Auto Report on Input Change**: Boolean
- **Test Mode**: Boolean

## Monitoring Data

### Network Status
- HID, Bit Index, Parent HID
- Is Root Device, Configuration Status
- Tree Depth, Child Device Count

### Network Statistics
- Messages Sent/Received/Forwarded/Ignored
- Security Violations, Last Message Time
- Last Sender MAC, Signal Strength (RSSI)

### I/O Status
- Input/Output States (binary display)
- My Bit State, Input Change Count
- Visual I/O pin status (3 inputs, 3 outputs)
- 32-bit Shared Data Display with highlighted device bit

### Device Data
- Memory States, Analog Values (2x)
- Integer Values (2x), Sequence Counter
- Uptime (formatted)

## Usage Instructions

1. **Upload Firmware**: Compile and upload the firmware to your ESP32 device
2. **Connect Web Interface**: Open the web interface and connect to the ESP32 via serial
3. **Load Configuration**: Click "Load Configuration" to get the device's current settings
4. **Configure Device**: Modify settings in the Network Identity and System Behavior tabs
5. **Save Configuration**: Click "Save Configuration" to apply changes
6. **Monitor Device**: View real-time data in the monitoring tabs

## Debug Level Control

The firmware supports dynamic debug level control through the web interface:
- **None (0)**: Minimal output
- **Basic (1)**: Standard operational messages
- **Detailed (2)**: Extended debugging information
- **Verbose (3)**: Maximum debugging output

## Troubleshooting

### Common Issues
1. **Serial Connection Failed**: Ensure correct baud rate (115200) and port selection
2. **JSON Parse Errors**: Check that ArduinoJson library is installed
3. **Configuration Not Saved**: Verify NVS storage is working properly
4. **Monitoring Data Not Updating**: Check that device is properly configured

### Debug Commands
The firmware responds to all commands with detailed logging. Check the serial monitor for:
- Command processing status
- JSON parsing results
- Configuration save/load operations
- Error messages and troubleshooting tips

## Integration Notes

- The serial command handler runs at high priority in the main loop
- All commands are processed asynchronously without blocking the main system
- Configuration changes are immediately applied and persisted to NVS
- Monitoring data is real-time and reflects current device state
- The web interface automatically handles JSON parsing and form generation

## Future Enhancements

Potential improvements for future versions:
- Real-time data streaming for monitoring panels
- Advanced configuration validation
- Bulk configuration operations
- Firmware update support
- Network topology visualization 