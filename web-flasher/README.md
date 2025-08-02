# ESP32 Device Manager

A modern, web-based tool for serial communication and device configuration with ESP32 devices through the browser. No software installation required!

## Features

- **Web-Based Interface**: Works entirely in the browser using Web Serial API
- **Serial Communication**: Real-time bidirectional communication with ESP32 devices
- **Device Configuration**: Dynamic configuration form generation and management
- **Serial Monitor**: Advanced console with pause, auto-scroll, and export features
- **Device Information**: Display device details and status
- **Configuration Export**: Save and export device configurations as JSON
- **Responsive Design**: Works on desktop and mobile devices

## Requirements

### Browser Support
- **Chrome/Chromium** (version 89+)
- **Edge** (version 89+)
- **Opera** (version 76+)

**Note**: Web Serial API is not supported in Firefox or Safari.

### Hardware
- ESP32 development board
- USB cable for connection
- Computer with USB ports

## Quick Start

### 1. Setup for Local Development

1. Clone or download this repository
2. Open `index.html` in a supported browser
3. For HTTPS (required for Web Serial API):
   - Use a local HTTPS server like `python -m http.server 8000` with SSL
   - Or deploy to GitHub Pages (recommended)

### 2. Deploy to GitHub Pages

1. Create a new GitHub repository
2. Upload all files from the `web-flasher` directory
3. Go to repository Settings → Pages
4. Select "Deploy from a branch" → "main" branch
5. Your device manager will be available at `https://yourusername.github.io/your-repo-name/`

### 3. Using the Device Manager

1. **Connect Device**:
   - Click "Connect Serial"
   - Select your ESP32 from the port list
   - Choose the appropriate baud rate
   - The status indicator will turn green when connected

2. **Serial Communication**:
   - Use the serial console to send commands
   - View real-time device output
   - Control console features (pause, auto-scroll, timestamps)
   - Export logs for debugging

3. **Device Configuration**:
   - Click "Load Configuration" to request device schema
   - Fill out the dynamic configuration form
   - Save configuration to device
   - Export configuration as JSON backup

4. **Device Management**:
   - View device information and status
   - Restart device remotely
   - Monitor device responses

## ESP32 Firmware Requirements

Your ESP32 firmware should support these features for full compatibility:

### Serial Commands
The device manager sends these commands to the ESP32:

| Command | Description | Expected Response |
|---------|-------------|-------------------|
| `CONFIG_SCHEMA` | Request configuration schema | JSON schema definition |
| `CONFIG_SAVE <json>` | Save configuration | Success/error message |
| `RESTART` | Restart device | Confirmation message |

### Configuration Schema Format
The ESP32 should respond to `CONFIG_SCHEMA` with a JSON object like:

```json
{
  "wifi": {
    "ssid": { "type": "string", "label": "WiFi SSID", "default": "", "required": true },
    "password": { "type": "password", "label": "WiFi Password", "default": "", "required": true },
    "auto_connect": { "type": "boolean", "label": "Auto Connect", "default": true }
  },
  "mqtt": {
    "broker": { "type": "string", "label": "MQTT Broker", "default": "localhost", "required": true },
    "port": { "type": "number", "label": "MQTT Port", "default": 1883, "min": 1, "max": 65535 }
  }
}
```

### Field Types Supported
- `string`: Text input
- `password`: Password input (masked)
- `number`: Numeric input with min/max validation
- `boolean`: Checkbox input
- `select`: Dropdown with predefined options

## File Structure

```
web-flasher/
├── index.html          # Main HTML interface
├── css/
│   ├── style.css       # Custom styling
│   └── xterm.css       # Terminal styling
├── js/
│   ├── app.js          # Main application logic
│   ├── crypto-js.js    # Cryptographic functions
│   └── xterm.min.js    # Terminal library
└── README.md           # This file
```

## Serial Console Features

### Console Controls
- **Pause/Resume**: Temporarily stop log updates
- **Auto-scroll**: Automatically scroll to newest messages
- **Timestamps**: Show/hide message timestamps
- **Flush Buffer**: Clear the serial buffer
- **Clear Log**: Remove all console messages
- **Export Log**: Download console output as text file

### Message Types
- **Info**: General information messages
- **Success**: Successful operations
- **Warning**: Warning messages
- **Error**: Error messages
- **Data**: Device output data
- **Command**: Sent commands

## Configuration Management

### Dynamic Form Generation
The device manager automatically generates configuration forms based on the device's schema response. This allows for:

- **Flexible Configuration**: Support any configuration structure
- **Type Validation**: Input validation based on field types
- **Default Values**: Pre-populate fields with device defaults
- **Required Fields**: Highlight mandatory configuration items

### Configuration Actions
- **Load Configuration**: Request and display current device settings
- **Save Configuration**: Send updated settings to device
- **Export Configuration**: Download configuration as JSON backup
- **Restart Device**: Remotely restart the ESP32

## Troubleshooting

### Connection Issues

1. **"Web Serial API not supported"**
   - Use Chrome, Edge, or Opera
   - Ensure you're using HTTPS (required for Web Serial API)

2. **"No ports available"**
   - Check USB cable connection
   - Install ESP32 USB drivers if needed
   - Try a different USB port

3. **"Connection failed"**
   - Check baud rate settings
   - Verify USB cable supports data transfer
   - Ensure ESP32 is powered and responsive

### Communication Issues

1. **"No response from device"**
   - Verify ESP32 firmware is running
   - Check baud rate compatibility
   - Ensure device is not in bootloader mode

2. **"Configuration not loaded"**
   - Verify ESP32 firmware supports `CONFIG_SCHEMA` command
   - Check JSON response format
   - Monitor serial output for error messages

3. **"Configuration not saved"**
   - Check JSON format sent to device
   - Verify ESP32 firmware supports `CONFIG_SAVE` command
   - Monitor serial output for error messages

### Console Issues

1. **"Console not updating"**
   - Check if console is paused
   - Verify auto-scroll is enabled
   - Clear console and reconnect

2. **"Export not working"**
   - Check browser download settings
   - Ensure console has content to export
   - Try different browser

## Security Considerations

- **HTTPS Required**: Web Serial API only works over HTTPS
- **Local Access**: Serial port access requires user permission
- **No Data Storage**: Configuration is not stored on the web server
- **Secure Communication**: All serial communication is local

## Browser Compatibility

| Browser | Version | Status |
|---------|---------|--------|
| Chrome | 89+ | ✅ Supported |
| Edge | 89+ | ✅ Supported |
| Opera | 76+ | ✅ Supported |
| Firefox | Any | ❌ Not Supported |
| Safari | Any | ❌ Not Supported |

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is open source. Feel free to modify and distribute according to your needs.

## Support

For issues and questions:
1. Check the troubleshooting section
2. Review browser compatibility
3. Verify hardware connections
4. Check ESP32 firmware requirements

## Changelog

### Version 2.0.0
- Removed firmware flashing functionality
- Focused on serial communication and device configuration
- Added dynamic configuration form generation
- Enhanced serial console with advanced controls
- Added device information display
- Improved configuration export functionality
- Streamlined user interface

### Version 1.0.0
- Initial release with flashing capabilities
- Web Serial API integration
- Basic device configuration interface
- Serial monitor functionality 