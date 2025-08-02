# ESP32 Device Manager

A modern web-based device management interface for ESP32 devices, specifically designed for ESP-NOW Tree Network devices. This application provides real-time monitoring, configuration management, and serial communication capabilities through a clean, responsive web interface.

## 🌟 Features

### **Device Management**
- **Serial Communication**: Connect to ESP32 devices via Web Serial API
- **Real-time Monitoring**: Live status updates for network, I/O, and device data
- **Configuration Management**: Dynamic form generation for device parameters
- **Visual I/O Display**: Intuitive pin state visualization with input/output grids

### **Network Monitoring**
- **Network Status**: Hierarchical ID, Bit Index, Parent HID, Tree Depth
- **Network Statistics**: Message counts, signal strength, last activity
- **ESP-NOW Tree Support**: Optimized for distributed I/O networks

### **Advanced Features**
- **Multi-Input Shared Data**: Visual display of 32-bit × 3-input shared data
- **Dynamic Bit Index Highlighting**: Automatic highlighting of device's assigned bit
- **Configuration Persistence**: Save/load device configurations
- **Debug Console**: Separate debug and serial consoles with export capabilities

## 🚀 Quick Start

### **Web Application**
1. **Open the Web App**: Navigate to the GitHub Pages URL (see below)
2. **Connect Device**: Click "Connect Serial" and select your ESP32 device
3. **Load Configuration**: Click "Load Configuration" to get current device settings
4. **Monitor & Configure**: Use the tabs to view and modify device parameters

### **ESP32 Firmware Setup**
1. **Upload Firmware**: Flash the `HELTEC_ESPNOW_TREE_BCAST` firmware to your ESP32
2. **Configure Device**: Set Hierarchical ID and Bit Index via the web interface
3. **Network Setup**: Configure ESP-NOW tree network parameters as needed

## 📁 Project Structure

```
WEB_UPLOAD/
├── web-flasher/                 # Web application
│   ├── index.html              # Main interface
│   ├── js/
│   │   └── app.js              # Main application logic
│   ├── css/
│   │   └── style.css           # Styling and layout
│   └── README.md               # Web app documentation
├── HELTEC_ESPNOW_TREE_BCAST/   # ESP32 firmware
│   ├── HELTEC_ESPNOW_TREE_BCAST.ino
│   ├── DataManager.h/.cpp      # Data management
│   ├── IoDevice.h/.cpp         # I/O handling
│   ├── TreeNetwork.h/.cpp      # ESP-NOW tree network
│   ├── SerialCommandHandler.h/.cpp  # Web interface communication
│   └── MenuSystem.h/.cpp       # OLED display management
└── README.md                   # This file
```

## 🔧 Technical Details

### **Web Technologies**
- **HTML5**: Modern semantic markup
- **CSS3**: Responsive design with Bootstrap 5
- **JavaScript ES6+**: Modern async/await patterns
- **Web Serial API**: Direct device communication
- **Web Workers**: Background processing

### **ESP32 Features**
- **ESP-NOW**: Wireless mesh networking
- **NVS**: Non-volatile storage for configuration
- **ArduinoJson**: JSON communication protocol
- **OLED Display**: Status visualization
- **Multi-Input Support**: 3-input × 32-bit shared data

### **Communication Protocol**
- **Serial Commands**: `CONFIG_SCHEMA`, `CONFIG_SAVE`, `IO_STATUS`, etc.
- **JSON Responses**: Structured data exchange
- **Real-time Updates**: 2-second monitoring intervals

## 🌐 GitHub Pages

This application is designed to run on GitHub Pages. The web interface is completely client-side and can be accessed directly from the repository.

### **Access the Web App**
Once deployed to GitHub Pages, you can access the application at:
```
https://[your-username].github.io/[repository-name]/
```

### **Local Development**
For local development, you can run a simple HTTP server:
```bash
cd web-flasher
python3 -m http.server 8080
# Then visit http://localhost:8080
```

## 📋 Requirements

### **Browser Requirements**
- **Chrome/Edge**: Web Serial API support
- **Firefox**: Limited Web Serial API support
- **Safari**: No Web Serial API support

### **ESP32 Requirements**
- **Arduino IDE**: For firmware compilation
- **Required Libraries**:
  - `ArduinoJson` (v6.x)
  - `WiFi` (built-in)
  - `ESP32` board package

## 🔄 Recent Updates

### **v2.0 - Multi-Input Shared Data**
- **Enhanced Display**: Horizontal layout for 32-bit × 3-input shared data
- **Dynamic Highlighting**: Automatic bit index highlighting based on device configuration
- **Improved UX**: Larger cells, better spacing, and clearer visual hierarchy
- **Real-time Updates**: Immediate configuration reflection without reload

### **v1.0 - Core Features**
- **Device Management**: Serial connection and configuration
- **Network Monitoring**: ESP-NOW tree network status
- **I/O Visualization**: Input/output pin state display
- **Configuration Persistence**: Save/load device settings

## 🤝 Contributing

1. **Fork the repository**
2. **Create a feature branch**: `git checkout -b feature/amazing-feature`
3. **Commit your changes**: `git commit -m 'Add amazing feature'`
4. **Push to the branch**: `git push origin feature/amazing-feature`
5. **Open a Pull Request**

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **ESP32 Community**: For excellent documentation and examples
- **Web Serial API**: For enabling direct browser-to-device communication
- **Bootstrap**: For responsive UI components
- **ArduinoJson**: For efficient JSON handling on embedded systems

---

**Note**: This application requires a modern browser with Web Serial API support. For best experience, use Chrome or Edge on desktop platforms. 