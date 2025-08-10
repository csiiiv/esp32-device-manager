/**
 * ESP32 Device Manager - Serial Communication and Configuration Tool
 * Focused on device communication and configuration management
 */

class ESP32DeviceManager {
    constructor() {
        // Serial connection
        this.serialDevice = null;
        this.serialTransport = null;
        this.isSerialConnected = false;
        this.serialReader = null;
        this.serialWriter = null;
        
        // Console states
        this.serialLogPaused = false;
        this.serialBuffer = '';
        this.serialAutoscroll = true;
        this.serialShowTimestamps = true;
        
        // Console line limits
        this.maxConsoleLines = 500;
        this.serialLineCount = 0;
        this.debugLineCount = 0;
        
        // Configuration
        this.configSchema = null;
        this.deviceInfo = null;
        this.currentConfig = {};
        
        // Debug and monitoring
        this.debugMode = true;
        this.connectionRetryCount = 0;
        this.maxRetryAttempts = 3;
        this.lastActivityTime = null;
        this.heartbeatInterval = null;
        
        this.initializeElements();
        this.bindEvents();
        this.initializeMonitoringPanels(); // Initialize monitoring panels with simulated data
        this.startSimulatedUpdates(); // Start simulated updates for preview
        console.log('🚀 ESP32 Device Manager loaded successfully!');
        this.log('ESP32 Device Manager initialized', 'info', 'serial');
        this.log('Debug mode: ' + (this.debugMode ? 'ENABLED' : 'DISABLED'), 'info', 'serial');
        this.logSystemInfo();
    }

    initializeElements() {
        // Serial connection elements
        this.serialConnectBtn = document.getElementById('serialConnectBtn');
        this.serialDisconnectBtn = document.getElementById('serialDisconnectBtn');
        this.serialConnectionStatus = document.getElementById('serialConnectionStatus');
        this.configBaudRate = document.getElementById('configBaudRate');

        // Serial console elements
        this.serialInput = document.getElementById('serialInput');
        this.sendBtn = document.getElementById('sendCommandBtn');
        this.serialOutput = document.getElementById('serialOutput');
        this.debugOutput = document.getElementById('debugOutput');
        this.debugConsole = document.getElementById('debugConsole');
        
        // Console controls
        this.debugToggleBtn = document.getElementById('debugToggleBtn');
        this.serialPauseBtn = document.getElementById('pauseSerialLogBtn');
        this.serialAutoscrollBtn = document.getElementById('autoscrollSerialBtn');
        this.serialTimestampBtn = document.getElementById('timestampSerialLogBtn');
        this.serialClearBtn = document.getElementById('clearSerialLogBtn');
        this.serialExportBtn = document.getElementById('exportSerialLogBtn');
        this.serialFlushBtn = document.getElementById('flushSerialBufferBtn');
        this.debugExportBtn = document.getElementById('exportDebugLogBtn');
        
        // Status panel elements
        this.connectionStatusPanel = document.getElementById('connectionStatusPanel');
        this.lastActivityTimeElement = document.getElementById('lastActivityTime');
        this.retryCountElement = document.getElementById('retryCount');
        
        // Configuration elements
        this.loadConfigBtn = document.getElementById('loadConfigBtn');
        this.saveConfigBtn = document.getElementById('saveConfigBtn');
        this.exportConfigBtn = document.getElementById('exportConfigBtn');
        this.restartDeviceBtn = document.getElementById('restartDeviceBtn');
        this.deviceInfoContainer = document.getElementById('deviceInfo');
        this.configFormContainer = document.getElementById('configFormContainer');
        
        // Monitoring panel elements
        this.networkStatusPanel = document.getElementById('networkStatusPanel');
        this.networkStatsPanel = document.getElementById('networkStatsPanel');
        this.ioStatusPanel = document.getElementById('ioStatusPanel');
        this.deviceDataPanel = document.getElementById('deviceDataPanel');
    }

    bindEvents() {
        // Serial connection events
        if (this.serialConnectBtn) {
            this.serialConnectBtn.addEventListener('click', () => this.toggleSerialConnection());
        }
        if (this.serialDisconnectBtn) {
            this.serialDisconnectBtn.addEventListener('click', () => this.disconnectSerial());
        }
        
        // Serial console events
        if (this.sendBtn) {
            this.sendBtn.addEventListener('click', () => this.sendCommand());
        }
        if (this.serialInput) {
        this.serialInput.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') this.sendCommand();
        });
        }
        
        // Debug controls
        if (this.debugToggleBtn) {
            this.debugToggleBtn.addEventListener('click', () => this.toggleDebugMode());
        }
        
        // Configuration events
        if (this.loadConfigBtn) {
            this.loadConfigBtn.addEventListener('click', () => this.loadConfiguration());
        }
        if (this.saveConfigBtn) {
            this.saveConfigBtn.addEventListener('click', () => this.saveConfiguration());
        }
        if (this.exportConfigBtn) {
            this.exportConfigBtn.addEventListener('click', () => this.exportConfiguration());
        }
        if (this.restartDeviceBtn) {
            this.restartDeviceBtn.addEventListener('click', () => this.restartDevice());
        }
        
        // Console control events
        if (this.serialPauseBtn) {
            this.serialPauseBtn.addEventListener('click', () => this.toggleSerialLogPause());
        }
        if (this.serialAutoscrollBtn) {
            this.serialAutoscrollBtn.addEventListener('click', () => this.toggleSerialAutoscroll());
        }
        if (this.serialTimestampBtn) {
            this.serialTimestampBtn.addEventListener('click', () => this.toggleSerialTimestamps());
        }
        if (this.serialClearBtn) {
            this.serialClearBtn.addEventListener('click', () => this.clearSerialLog());
        }
        if (this.serialExportBtn) {
            this.serialExportBtn.addEventListener('click', () => this.exportSerialLog());
        }
        if (this.debugExportBtn) {
            this.debugExportBtn.addEventListener('click', () => this.exportDebugLog());
        }
        if (this.serialFlushBtn) {
            this.serialFlushBtn.addEventListener('click', () => this.flushSerialBuffer());
        }
        
        // Add visibility change listener for better connection monitoring
        document.addEventListener('visibilitychange', () => {
            if (document.hidden && this.isSerialConnected) {
                this.log('Page hidden - monitoring connection status', 'warning', 'serial');
            } else if (!document.hidden && this.isSerialConnected) {
                this.log('Page visible - checking connection status', 'info', 'serial');
                this.checkConnectionStatus();
            }
        });
    }

    async toggleSerialConnection() {
        if (this.isSerialConnected) {
            await this.disconnectSerial();
        } else {
            await this.connectSerial();
        }
    }

    async connectSerial() {
        try {
            // Request port
            this.serialDevice = await navigator.serial.requestPort();
            
            // Open port
            await this.serialDevice.open({ baudRate: 115200 });
            
            // Get reader and writer
            this.serialReader = this.serialDevice.readable.getReader();
            this.serialWriter = this.serialDevice.writable.getWriter();
            
            // Update connection status FIRST
            this.isSerialConnected = true;
            this.updateSerialConnectionStatus('Connected');
            
            // Enable serial-dependent buttons
            this.serialInput.disabled = false;
            this.sendBtn.disabled = false;
            this.loadConfigBtn.disabled = false;
            
            // Start serial reader in background (don't await it)
            this.startSerialReader().catch(error => {
                // Don't disconnect immediately for reader errors
            });
            
            // Start connection monitoring
            this.startConnectionMonitoring();
            
            // Start monitoring updates
            this.startMonitoringUpdates();
            
            this.log('Serial connection established successfully', 'success', 'serial');
            
            // Auto-load configuration after connection with proper delay
            setTimeout(() => {
                this.loadConfiguration();
            }, 2000);

        } catch (error) {
            this.handleConnectionError(error);
        }
    }

    handleConnectionError(error) {
        this.log('=== CONNECTION ERROR ===', 'error', 'serial');
        this.log('Error type: ' + error.constructor.name, 'error', 'serial');
        this.log('Error message: ' + error.message, 'error', 'serial');
        this.log('Error stack: ' + error.stack, 'debug', 'debug');
        
        if (error.name === 'NotFoundError') {
            this.log('Device not found - check USB connection', 'error', 'serial');
        } else if (error.name === 'SecurityError') {
            this.log('Permission denied - user cancelled port selection', 'error', 'serial');
        } else if (error.name === 'InvalidStateError') {
            this.log('Port already in use - try disconnecting first', 'error', 'serial');
        } else if (error.name === 'NetworkError') {
            this.log('Network error - check device connection', 'error', 'serial');
        } else {
            this.log('Unknown connection error - check device and cable', 'error', 'serial');
        }
        
        this.connectionRetryCount++;
        this.updateStatusPanel();
        if (this.connectionRetryCount < this.maxRetryAttempts) {
            this.log(`Retry attempt ${this.connectionRetryCount}/${this.maxRetryAttempts} in 2 seconds...`, 'warning', 'serial');
            setTimeout(() => this.connectSerial(), 2000);
        } else {
            this.log('Max retry attempts reached. Please check device connection.', 'error', 'serial');
            this.connectionRetryCount = 0;
            this.updateStatusPanel();
        }
    }

    async disconnectSerial() {
        this.log('=== DISCONNECTING SERIAL ===', 'info', 'serial');
        
        try {
            // Stop connection monitoring
            this.stopConnectionMonitoring();
            this.stopMonitoringUpdates(); // Stop monitoring updates on disconnect
            
            // Stop serial reader
            if (this.serialReader) {
                this.log('Stopping serial reader...', 'debug', 'debug');
                await this.serialReader.cancel();
                this.serialReader = null;
            }
            
            // Close serial writer
            if (this.serialWriter) {
                this.log('Closing serial writer...', 'debug', 'debug');
                this.serialWriter.releaseLock();
                this.serialWriter = null;
            }
            
            // Close device
        if (this.serialDevice) {
                this.log('Closing serial device...', 'debug', 'debug');
            await this.serialDevice.close();
            this.serialDevice = null;
            }
            
            this.isSerialConnected = false;
            this.updateSerialConnectionStatus('Disconnected');
            this.log('Serial connection closed successfully', 'info', 'serial');
            
            // Disable serial input and configuration buttons
            this.serialInput.disabled = true;
            this.sendBtn.disabled = true;
            this.loadConfigBtn.disabled = true;
            this.saveConfigBtn.disabled = true;
            this.exportConfigBtn.disabled = true;
            this.restartDeviceBtn.disabled = true;
            
            // Clear device info and config form
            this.clearDeviceInfo();
            this.clearConfigForm();
            
        } catch (error) {
            this.log('Error during disconnect: ' + error.message, 'error', 'serial');
            // Force reset state even if error occurred
            this.isSerialConnected = false;
            this.serialDevice = null;
            this.serialReader = null;
            this.serialWriter = null;
            this.updateSerialConnectionStatus('Disconnected');
        }
    }

    updateSerialConnectionStatus(status) {
        const statusText = this.serialConnectionStatus.querySelector('.status-text');
        const statusDot = this.serialConnectionStatus.querySelector('.status-dot');
        
        if (statusText) statusText.textContent = status === 'Connected' ? 'Serial Connected' : 'Serial Disconnected';
        
        if (statusDot) {
            statusDot.className = 'status-dot ' + (status === 'Connected' ? 'connected' : 'disconnected');
        }
        
        if (status === 'Connected') {
            this.serialConnectBtn.style.display = 'none';
            this.serialDisconnectBtn.style.display = 'block';
        } else {
            this.serialConnectBtn.style.display = 'block';
            this.serialDisconnectBtn.style.display = 'none';
        }
    }

    async startSerialReader() {
        if (!this.serialDevice) {
            this.log('No serial device available for reading', 'error', 'serial');
            return;
        }
        
        if (!this.serialReader) {
            this.log('No serial reader available', 'error', 'serial');
            return;
        }
        
        this.log('Starting serial reader...', 'debug', 'debug');
        
        try {
            const decoder = new TextDecoder();
            
            this.log('Serial reader started successfully', 'debug', 'debug');
            
            while (this.isSerialConnected) {
                try {
                    const { value, done } = await this.serialReader.read();
                    
                    if (done) {
                        this.log('Serial reader stream ended', 'warning', 'serial');
                        break;
                    }
                    
                    // Update last activity time
                    this.lastActivityTime = Date.now();
                    this.updateStatusPanel();
                    
                    const text = decoder.decode(value, { stream: true });
                    this.serialBuffer += text;
                    
                    if (this.debugMode) {
                        this.log(`Raw data received: ${value.length} bytes`, 'debug', 'debug');
                    }
                    
                    // Process complete lines
                    const lines = this.serialBuffer.split('\n');
                    this.serialBuffer = lines.pop(); // Keep incomplete line in buffer
                    
                    for (const line of lines) {
                        if (line.trim()) {
                            this.handleSerialData(line.trim());
                        }
                    }
                    
                } catch (readError) {
                    // Don't immediately disconnect on read errors
                    // Just log the error and continue
                    this.log('Serial read error (non-fatal): ' + readError.message, 'warning', 'serial');
                    
                    // Only disconnect if it's a serious error
                    if (readError.name === 'NetworkError' || readError.name === 'InvalidStateError') {
                        this.handleSerialReadError(readError);
                        break;
                    }
                    
                    // For other errors, wait a bit and continue
                    await new Promise(resolve => setTimeout(resolve, 100));
                }
            }
            
        } catch (error) {
            this.handleSerialReadError(error);
        } finally {
            if (this.serialReader) {
                this.serialReader.releaseLock();
                this.serialReader = null;
            }
        }
    }

    handleSerialReadError(error) {
        this.log('=== SERIAL READ ERROR ===', 'error', 'serial');
        this.log('Error type: ' + error.constructor.name, 'error', 'serial');
        this.log('Error message: ' + error.message, 'error', 'serial');
        
        if (this.debugMode) {
            this.log('Error stack: ' + error.stack, 'debug', 'debug');
            this.log('Error details:', 'debug', 'debug');
            this.log('- Error name: ' + error.name, 'debug', 'debug');
            this.log('- Error code: ' + (error.code || 'N/A'), 'debug', 'debug');
            this.log('- Error cause: ' + (error.cause || 'N/A'), 'debug', 'debug');
        }
        
        if (error.name === 'NetworkError') {
            this.log('Device connection lost - network error detected', 'error', 'serial');
            this.log('This usually indicates the device was disconnected', 'error', 'serial');
            this.log('Troubleshooting steps:', 'error', 'serial');
            this.log('1. Check USB cable connection', 'error', 'serial');
            this.log('2. Try a different USB port', 'error', 'serial');
            this.log('3. Restart the ESP32 device', 'error', 'serial');
            this.log('4. Check if device is powered properly', 'error', 'serial');
        } else if (error.name === 'InvalidStateError') {
            this.log('Serial port in invalid state - device may have been disconnected', 'error', 'serial');
            this.log('Troubleshooting steps:', 'error', 'serial');
            this.log('1. Disconnect and reconnect the device', 'error', 'serial');
            this.log('2. Close any other applications using the serial port', 'error', 'serial');
            this.log('3. Restart the browser', 'error', 'serial');
        } else if (error.name === 'NotReadableError') {
            this.log('Serial port not readable - device may be in use by another application', 'error', 'serial');
            this.log('Troubleshooting steps:', 'error', 'serial');
            this.log('1. Close Arduino IDE, PlatformIO, or other serial monitors', 'error', 'serial');
            this.log('2. Check Windows Device Manager for port conflicts', 'error', 'serial');
            this.log('3. Restart the computer if needed', 'error', 'serial');
        } else if (error.name === 'AbortError') {
            this.log('Serial operation aborted - connection was closed', 'error', 'serial');
        } else {
            this.log('Unknown serial read error - device may have been disconnected', 'error', 'serial');
            this.log('Error details: ' + JSON.stringify(error, Object.getOwnPropertyNames(error)), 'debug', 'debug');
        }
        
        // Attempt to gracefully handle the disconnection
        this.handleDeviceDisconnection();
    }

    handleDeviceDisconnection() {
        console.log('🚨 handleDeviceDisconnection: Device disconnection detected!');
        console.log('🚨 handleDeviceDisconnection: Stack trace:', new Error().stack);
        
        this.log('=== DEVICE DISCONNECTION DETECTED ===', 'warning', 'serial');
        this.log('Attempting to handle device disconnection gracefully...', 'info', 'serial');
        
        // Stop connection monitoring
        console.log('🛑 handleDeviceDisconnection: Stopping connection monitoring');
        this.stopConnectionMonitoring();
        this.stopMonitoringUpdates(); // Stop monitoring updates on disconnection
        
        // Update UI to reflect disconnection
        console.log('🔄 handleDeviceDisconnection: Setting isSerialConnected to false');
        this.isSerialConnected = false;
        this.updateSerialConnectionStatus('Disconnected');
        
        // Disable all serial-dependent buttons
        console.log('🔒 handleDeviceDisconnection: Disabling buttons');
        this.serialInput.disabled = true;
        this.sendBtn.disabled = true;
        this.loadConfigBtn.disabled = true;
        this.saveConfigBtn.disabled = true;
        this.exportConfigBtn.disabled = true;
        this.restartDeviceBtn.disabled = true;
        
        // Clear device info and config form
        console.log('🧹 handleDeviceDisconnection: Clearing device info and form');
        this.clearDeviceInfo();
        this.clearConfigForm();
        
        this.log('Device disconnection handled. Please reconnect when ready.', 'info', 'serial');
        
        // Clean up resources
        console.log('🧹 handleDeviceDisconnection: Cleaning up resources');
        this.serialDevice = null;
        this.serialReader = null;
        this.serialWriter = null;
        
        console.log('✅ handleDeviceDisconnection: Disconnection handling complete');
    }

    startConnectionMonitoring() {
        this.log('Starting connection monitoring...', 'debug', 'debug');
        
        // Monitor connection every 5 seconds
        this.heartbeatInterval = setInterval(() => {
            this.checkConnectionStatus();
        }, 5000);
        
        // Update status panel every second when debug mode is enabled
        this.statusUpdateInterval = setInterval(() => {
            this.updateStatusPanel();
        }, 1000);
    }

    stopConnectionMonitoring() {
        if (this.heartbeatInterval) {
            this.log('Stopping connection monitoring...', 'debug', 'debug');
            clearInterval(this.heartbeatInterval);
            this.heartbeatInterval = null;
        }
        
        if (this.statusUpdateInterval) {
            clearInterval(this.statusUpdateInterval);
            this.statusUpdateInterval = null;
        }
    }

    async checkConnectionStatus() {
        console.log('🔍 checkConnectionStatus: Checking connection...');
        console.log('  - isSerialConnected:', this.isSerialConnected);
        console.log('  - serialDevice:', !!this.serialDevice);
        
        if (!this.isSerialConnected || !this.serialDevice) {
            console.log('❌ checkConnectionStatus: Not connected or no device');
            return;
        }
        
        try {
            // Check if device is still accessible
            console.log('🔍 checkConnectionStatus: Getting device signals...');
            const info = await this.serialDevice.getSignals();
            this.lastActivityTime = Date.now();
            this.updateStatusPanel();
            
            console.log('✅ checkConnectionStatus: Device signals OK');
            
            if (this.debugMode) {
                this.log('Connection check: Device signals OK', 'debug', 'debug');
            }
            
        } catch (error) {
            console.log('💥 checkConnectionStatus: Error checking connection:', error);
            this.log('Connection check failed: ' + error.message, 'warning', 'serial');
            this.handleDeviceDisconnection();
        }
    }

    async sendCommand() {
        if (!this.isSerialConnected) {
            this.log('Please connect to device first', 'error', 'serial');
            return;
        }

        const command = this.serialInput.value.trim();
        if (!command) return;

        try {
            await this.sendSerialData(command);
            this.serialInput.value = '';
        } catch (error) {
            this.log(`Failed to send command: ${error.message}`, 'error', 'serial');
        }
    }

    async sendSerialData(data) {
        if (!this.isSerialConnected) {
            return;
        }

        if (!this.serialWriter) {
            return;
        }

        try {
            const encoder = new TextEncoder();
            const encodedData = encoder.encode(data + '\n');
            await this.serialWriter.write(encodedData);
        } catch (error) {
            this.log('Error sending data: ' + error.message, 'error', 'serial');
        }
    }

    async loadConfiguration() {
        if (!this.isSerialConnected) {
            this.log('Please connect to device first', 'error', 'serial');
            return;
        }

        if (!this.serialWriter) {
            this.log('Serial writer not available', 'error', 'serial');
            return;
        }

        try {
            this.log('Loading configuration from device...', 'info', 'serial');
            
            // Clear existing form
            this.clearConfigForm();
            
            // Request configuration schema from device
            await this.sendSerialData('CONFIG_SCHEMA');
            
            // Wait a bit for the response
            await new Promise(resolve => setTimeout(resolve, 1000));
            
            if (this.configSchema) {
                this.generateConfigForm();
                this.saveConfigBtn.disabled = false;
                this.exportConfigBtn.disabled = false;
                this.log('Configuration loaded successfully', 'success', 'serial');
            } else {
                this.log('Failed to load configuration - no schema received', 'error', 'serial');
            }
            
        } catch (error) {
            this.log('Error loading configuration: ' + error.message, 'error', 'serial');
        }
    }

    simulateConfigResponse() {
        // Simulate device information
        this.deviceInfo = {
            chip: 'ESP32',
            version: '1.0.0',
            mac: 'AA:BB:CC:DD:EE:FF',
            flash: '4MB',
            sdk: 'ESP-IDF v4.4.0'
        };
        
        // Simulate configuration schema for ESP-NOW Tree Network
        this.configSchema = {
            network_identity: {
                hierarchical_id: { 
                    type: 'number', 
                    label: 'Hierarchical ID (HID)', 
                    default: 0, 
                    min: 1, 
                    max: 999, 
                    required: true,
                    description: 'Device position in tree structure (1-999)'
                },
                bit_index: { 
                    type: 'number', 
                    label: 'Bit Index', 
                    default: 1, 
                    min: 0, 
                    max: 31, 
                    required: true,
                    description: 'Assigned bit position in shared 32-bit data (B0-B31)'
                },
                device_name: { 
                    type: 'string', 
                    label: 'Device Name', 
                    default: 'ESP32_Device', 
                    required: false,
                    description: 'Human-readable device identifier'
                }
            },
            system_behavior: {
                debug_level: { 
                    type: 'select', 
                    label: 'Debug Logging Level', 
                    options: ['None', 'Basic', 'Detailed', 'Verbose'], 
                    default: 'Basic',
                    description: 'Level of debug output (None=0, Basic=1, Detailed=2, Verbose=3)'
                },
                status_interval: { 
                    type: 'number', 
                    label: 'Status Update Interval (ms)', 
                    default: 200, 
                    min: 100, 
                    max: 5000,
                    description: 'How often to update status display'
                },
                auto_report: { 
                    type: 'boolean', 
                    label: 'Auto Report on Input Change', 
                    default: true,
                    description: 'Automatically report when inputs change'
                },
                test_mode: { 
                    type: 'boolean', 
                    label: 'Test Mode', 
                    default: false,
                    description: 'Enable test mode for debugging'
                }
            }
        };
        
        this.updateDeviceInfo();
        this.generateConfigForm();
        
        this.log('Configuration schema loaded successfully', 'success', 'serial');
        this.saveConfigBtn.disabled = false;
        this.exportConfigBtn.disabled = false;
        
        // Initialize monitoring panels
        this.initializeMonitoringPanels();
        
        // Start monitoring updates
        this.startMonitoringUpdates();
    }

    initializeMonitoringPanels() {
        // Initialize panels with placeholder data until real data arrives
        this.updateNetworkStatusPanel();
        this.updateNetworkStatsPanel();
        this.updateIOStatusPanel();
        this.updateDeviceDataPanel();
    }

    startMonitoringUpdates() {
        // Update monitoring panels every 2 seconds when connected
        this.monitoringInterval = setInterval(() => {
            if (this.isSerialConnected) {
                // Send actual commands to device instead of using simulated data
                this.requestNetworkStatus();
                this.requestNetworkStats();
                this.requestIOStatus();
                this.requestDeviceData();
            }
        }, 2000);
    }

    requestNetworkStatus() {
        if (this.isSerialConnected) {
            this.sendSerialData('NETWORK_STATUS\n');
        }
    }

    requestNetworkStats() {
        if (this.isSerialConnected) {
            this.sendSerialData('NETWORK_STATS\n');
        }
    }

    requestIOStatus() {
        if (this.isSerialConnected) {
            this.sendSerialData('IO_STATUS\n');
        }
    }

    requestDeviceData() {
        if (this.isSerialConnected) {
            this.sendSerialData('DEVICE_DATA\n');
        }
    }

    stopMonitoringUpdates() {
        if (this.monitoringInterval) {
            clearInterval(this.monitoringInterval);
            this.monitoringInterval = null;
        }
    }

    updateNetworkStatusPanel() {
        if (!this.networkStatusPanel) return;
        
        const content = this.networkStatusPanel.querySelector('#networkStatusContent');
        const indicator = this.networkStatusPanel.querySelector('#networkStatusIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'Disconnected';
            indicator.className = 'panel-status disconnected';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>Network Status:</strong> Connect to device to view network information.
                </div>
            `;
            return;
        }
        
        // Simulate network status data
        const networkData = {
            hid: 121,
            bitIndex: 5,
            parentHid: 12,
            isRoot: false,
            isConfigured: true,
            treeDepth: 2,
            childCount: 0,
            configurationStatus: 'Configured'
        };
        
        indicator.textContent = 'Connected';
        indicator.className = 'panel-status connected';
        
        content.innerHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Hierarchical ID</div>
                    <div class="value number">${networkData.hid}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Bit Index</div>
                    <div class="value number">${networkData.bitIndex}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Parent HID</div>
                    <div class="value number">${networkData.parentHid}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Is Root Device</div>
                    <div class="value boolean ${networkData.isRoot ? '' : 'false'}">${networkData.isRoot ? 'Yes' : 'No'}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Configuration Status</div>
                    <div class="value string">${networkData.configurationStatus}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Tree Depth</div>
                    <div class="value number">${networkData.treeDepth}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Child Devices</div>
                    <div class="value number">${networkData.childCount}</div>
                </div>
            </div>
        `;
    }

    updateNetworkStatsPanel() {
        if (!this.networkStatsPanel) return;
        
        const content = this.networkStatsPanel.querySelector('#networkStatsContent');
        const indicator = this.networkStatsPanel.querySelector('#networkStatsIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'No Data';
            indicator.className = 'panel-status no-data';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>Network Statistics:</strong> Connect to device to view performance metrics.
                </div>
            `;
            return;
        }
        
        // Simulate network statistics data
        const statsData = {
            messagesSent: Math.floor(Math.random() * 1000) + 500,
            messagesReceived: Math.floor(Math.random() * 800) + 400,
            messagesForwarded: Math.floor(Math.random() * 200) + 100,
            messagesIgnored: Math.floor(Math.random() * 50) + 10,
            securityViolations: Math.floor(Math.random() * 5),
            lastMessageTime: new Date().toLocaleTimeString(),
            lastSenderMAC: 'AA:BB:CC:DD:EE:FF',
            signalStrength: (Math.random() * 30 - 60).toFixed(1)
        };
        
        indicator.textContent = 'Active';
        indicator.className = 'panel-status active';
        
        content.innerHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Messages Sent</div>
                    <div class="value number">${statsData.messagesSent}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Messages Received</div>
                    <div class="value number">${statsData.messagesReceived}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Messages Forwarded</div>
                    <div class="value number">${statsData.messagesForwarded}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Messages Ignored</div>
                    <div class="value number">${statsData.messagesIgnored}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Security Violations</div>
                    <div class="value number">${statsData.securityViolations}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Last Message Time</div>
                    <div class="value timestamp">${statsData.lastMessageTime}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Last Sender MAC</div>
                    <div class="value mac">${statsData.lastSenderMAC}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Signal Strength (RSSI)</div>
                    <div class="value signal ${parseFloat(statsData.signalStrength) > -50 ? '' : parseFloat(statsData.signalStrength) > -70 ? 'weak' : 'poor'}">${statsData.signalStrength} dBm</div>
                </div>
            </div>
        `;
    }

    updateIOStatusPanel() {
        if (!this.ioStatusPanel) {
            return;
        }
        
        const content = this.ioStatusPanel.querySelector('#ioStatusContent');
        const indicator = this.ioStatusPanel.querySelector('#ioStatusIndicator');
        
        if (!content || !indicator) {
            return;
        }
        
        // Always show simulated data for preview, even when disconnected
        if (!this.isSerialConnected) {
            indicator.textContent = 'Simulated Data';
            indicator.className = 'panel-status no-data';
        } else {
            indicator.textContent = 'Connected';
            indicator.className = 'panel-status connected';
        }
        
        // Simulate I/O data with interesting patterns (always show for preview)
        const ioData = {
            inputStates: 5, // 0b101 = Input 1 ON, Input 2 OFF, Input 3 ON
            outputStates: 2, // 0b010 = Output 1 OFF, Output 2 ON, Output 3 OFF
            sharedData: 0x12345678, // Sample shared data for input 1
            myBitState: true,
            sharedDataArray: [
                0x12345678, // Input 1 shared data - some bits set
                0x87654321, // Input 2 shared data - different pattern
                0xAAAAAAAA  // Input 3 shared data - alternating pattern
            ],
            sharedOutputArray: [
                0x11111111, // Output 1 shared data - sample
                0x22222222, // Output 2 shared data - sample
                0x33333333  // Output 3 shared data - sample
            ],
            myBitStatesArray: [
                true,   // Input 1 my bit state
                false,  // Input 2 my bit state
                true    // Input 3 my bit state
            ],
            myOutputStatesArray: [
                true,
                false,
                true
            ],
            inputChangeCount: 42,
            lastInputChange: this.formatUptime(30000)
        };
        
        // Create input pin display
        const inputPins = [];
        for (let i = 0; i < 3; i++) {
            const inputActive = (ioData.inputStates & (1 << i)) !== 0;
            inputPins.push(`
                <div class="io-pin ${inputActive ? 'active' : 'inactive'}">
                    <div class="pin-label">Input ${i + 1}</div>
                    <div class="pin-state">${inputActive ? 'ON' : 'OFF'}</div>
                </div>
            `);
        }
        
        // Create output pin display
        const outputPins = [];
        for (let i = 0; i < 3; i++) {
            const outputActive = (ioData.outputStates & (1 << i)) !== 0;
            outputPins.push(`
                <div class="io-pin ${outputActive ? 'active' : 'inactive'}">
                    <div class="pin-label">Output ${i + 1}</div>
                    <div class="pin-state">${outputActive ? 'ON' : 'OFF'}</div>
                </div>
            `);
        }
        
        // Create shared data bit display for all inputs
        let sharedDataHTML = '';
        
        // Get the actual device bit index
        const deviceBitIndex = this.currentConfig?.network_identity?.bit_index ?? 5;
        
        // Check if we have the new multi-input format
        if (Array.isArray(ioData.sharedDataArray) && ioData.sharedDataArray.length >= 3) {
            // New multi-input format - Horizontal inputs+outputs per bit, grouped and tiled
            sharedDataHTML += `<h6>Shared Data Overview (32 bits × 3 inputs × 3 outputs)</h6>`;
            sharedDataHTML += `<div class="shared-data-display">`;
            sharedDataHTML += `<div class="shared-data-bits horizontal-overview">`;
            
            for (let bitIndex = 0; bitIndex < 32; bitIndex++) {
                const isMyBit = bitIndex === deviceBitIndex; // Use actual device bit index
                sharedDataHTML += `<div class="bit-group ${isMyBit ? 'my-bit-group' : ''}">`;
                sharedDataHTML += `<div class="bit-group-label">B${bitIndex}</div>`;
                // Inputs row
                sharedDataHTML += `<div class="input-group">`;
                for (let inputIndex = 0; inputIndex < 3; inputIndex++) {
                    const inputSharedData = ioData.sharedDataArray[inputIndex] || 0;
                    const bitActive = (inputSharedData & (1 << bitIndex)) !== 0;
                    sharedDataHTML += `<div class="bit-item ${bitActive ? 'active' : 'inactive'} ${isMyBit ? 'my-bit' : ''}">I${inputIndex + 1}</div>`;
                }
                sharedDataHTML += `</div>`; // Close inputs
                // Outputs row
                sharedDataHTML += `<div class="input-group">`;
                for (let outIndex = 0; outIndex < 3; outIndex++) {
                    const outputSharedData = (ioData.sharedOutputArray && ioData.sharedOutputArray[outIndex]) || 0;
                    const bitActiveQ = (outputSharedData & (1 << bitIndex)) !== 0;
                    sharedDataHTML += `<div class="bit-item ${bitActiveQ ? 'active' : 'inactive'} ${isMyBit ? 'my-bit' : ''}">Q${outIndex + 1}</div>`;
                }
                sharedDataHTML += `</div>`; // Close outputs
                sharedDataHTML += `</div>`; // Close bit-group
            }
            
            sharedDataHTML += `</div>`;
            sharedDataHTML += `<small class="text-muted">Format per bit: I:[1..3] then Q:[1..3]. Highlighted group (B${deviceBitIndex}) is this device's assigned bit.${!this.isSerialConnected ? ' (Simulated Data)' : ''}</small>`;
            sharedDataHTML += `</div>`;
        } else {
            // Backward compatibility - single input format
            sharedDataHTML += `<h6>Shared Data (32-bit) - Input 1</h6>`;
            sharedDataHTML += `<div class="shared-data-display">`;
            sharedDataHTML += `<div class="shared-data-bits">`;
            
            for (let i = 0; i < 32; i++) {
                const bitActive = (ioData.sharedData & (1 << i)) !== 0;
                const isMyBit = i === 5; // This device's bit index
                sharedDataHTML += `
                    <div class="bit-item ${bitActive ? 'active' : 'inactive'} ${isMyBit ? 'my-bit' : ''}">
                        B${i}-1
                    </div>
                `;
            }
            
            sharedDataHTML += `</div>`;
            sharedDataHTML += `<small class="text-muted">Highlighted bit (B5-1) is this device's assigned bit for Input 1</small>`;
            sharedDataHTML += `</div>`;
        }
        
        sharedDataHTML += `<small class="text-muted">Format: [Input1][Input2][Input3] for each bit. Highlighted group (B${deviceBitIndex}) is this device's assigned bit.${!this.isSerialConnected ? ' (Simulated Data)' : ''}</small>`;
        sharedDataHTML += `</div>`;
        
        console.log('📄 Generated shared data HTML length:', sharedDataHTML.length);
        console.log('📄 Shared data HTML preview:', sharedDataHTML.substring(0, 200) + '...');
        
        const finalHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Input States</div>
                    <div class="value boolean ${ioData.inputStates > 0 ? 'true' : 'false'}">
                        <i class="fas fa-${ioData.inputStates > 0 ? 'check' : 'times'}"></i>
                        ${ioData.inputStates} (0b${ioData.inputStates.toString(2).padStart(3, '0')})
                    </div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Output States</div>
                    <div class="value boolean ${ioData.outputStates > 0 ? 'true' : 'false'}">
                        <i class="fas fa-${ioData.outputStates > 0 ? 'check' : 'times'}"></i>
                        ${ioData.outputStates} (0b${ioData.outputStates.toString(2).padStart(3, '0')})
                    </div>
                </div>
                <div class="monitoring-item">
                    <div class="label">My Bit State</div>
                    <div class="value boolean ${ioData.myBitState ? 'true' : 'false'}">
                        <i class="fas fa-${ioData.myBitState ? 'check' : 'times'}"></i>
                        ${ioData.myBitState ? 'Active' : 'Inactive'}
                    </div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Input Changes</div>
                    <div class="value number">${ioData.inputChangeCount}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Last Input Change</div>
                    <div class="value timestamp">${ioData.lastInputChange}</div>
                </div>
            </div>
            
            <h6>Input Pins</h6>
            <div class="io-grid">
                ${inputPins.join('')}
            </div>
            
            <h6>Output Pins</h6>
            <div class="io-grid">
                ${outputPins.join('')}
            </div>
            
            ${sharedDataHTML}
        `;
        
        console.log('📄 Final HTML length:', finalHTML.length);
        console.log('📄 Setting content.innerHTML...');
        
        content.innerHTML = finalHTML;
        
        console.log('✅ I/O Status Panel updated successfully');
    }

    updateDeviceDataPanel() {
        if (!this.deviceDataPanel) return;
        
        const content = this.deviceDataPanel.querySelector('#deviceDataContent');
        const indicator = this.deviceDataPanel.querySelector('#deviceDataIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'No Data';
            indicator.className = 'panel-status no-data';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>Device Data:</strong> Connect to device to view device-specific data.
                </div>
            `;
            return;
        }
        
        // Simulate device data
        const deviceData = {
            memoryStates: Math.floor(Math.random() * 0xFFFF),
            analogValue1: Math.floor(Math.random() * 4096),
            analogValue2: Math.floor(Math.random() * 4096),
            integerValue1: Math.floor(Math.random() * 0xFFFF),
            integerValue2: Math.floor(Math.random() * 0xFFFF),
            sequenceCounter: Math.floor(Math.random() * 256),
            uptime: Math.floor(Math.random() * 86400000) // Random uptime in ms
        };
        
        indicator.textContent = 'Active';
        indicator.className = 'panel-status active';
        
        content.innerHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Memory States</div>
                    <div class="value number">0x${deviceData.memoryStates.toString(16).toUpperCase().padStart(4, '0')}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Analog Value 1</div>
                    <div class="value number">${deviceData.analogValue1}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Analog Value 2</div>
                    <div class="value number">${deviceData.analogValue2}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Integer Value 1</div>
                    <div class="value number">${deviceData.integerValue1}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Integer Value 2</div>
                    <div class="value number">${deviceData.integerValue2}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Sequence Counter</div>
                    <div class="value number">${deviceData.sequenceCounter}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Uptime</div>
                    <div class="value timestamp">${this.formatUptime(deviceData.uptime)}</div>
                </div>
            </div>
        `;
    }

    formatUptime(ms) {
        const seconds = Math.floor(ms / 1000);
        const minutes = Math.floor(seconds / 60);
        const hours = Math.floor(minutes / 60);
        const days = Math.floor(hours / 24);
        
        if (days > 0) {
            return `${days}d ${hours % 24}h ${minutes % 60}m`;
        } else if (hours > 0) {
            return `${hours}h ${minutes % 60}m ${seconds % 60}s`;
        } else if (minutes > 0) {
            return `${minutes}m ${seconds % 60}s`;
        } else {
            return `${seconds}s`;
        }
    }

    updateDeviceInfo() {
        if (!this.deviceInfo) return;
        
        this.deviceInfoContainer.innerHTML = `
            <div class="row">
                <div class="col-md-6">
                    <strong>Chip:</strong> ${this.deviceInfo.chip}<br>
                    <strong>Version:</strong> ${this.deviceInfo.version}<br>
                    <strong>SDK:</strong> ${this.deviceInfo.sdk}
                </div>
                <div class="col-md-6">
                    <strong>MAC:</strong> ${this.deviceInfo.mac}<br>
                    <strong>Flash:</strong> ${this.deviceInfo.flash}<br>
                    <strong>Status:</strong> <span class="text-success">Connected</span>
                </div>
            </div>
        `;
    }

    generateConfigForm() {
        if (!this.configSchema) {
            return;
        }
        
        let formHTML = '<div class="dynamic-config-form">';
        
        // Create tabs for different configuration sections
        const sections = Object.keys(this.configSchema);
        formHTML += '<div class="config-tabs">';
        sections.forEach((section, index) => {
            formHTML += `<button class="tab-button ${index === 0 ? 'active' : ''}" onclick="window.deviceManager.showConfigTab('${section}')">${section.toUpperCase()}</button>`;
        });
        formHTML += '</div>';
        
        // Create tab panels
        sections.forEach((section, index) => {
            formHTML += `<div class="tab-panel ${index === 0 ? 'active' : ''}" id="tab-${section}">`;
            formHTML += this.generateConfigFields(section, this.configSchema[section]);
            formHTML += '</div>';
        });
        
        formHTML += '</div>';
        
        this.configFormContainer.innerHTML = formHTML;
        
        // Store reference to this instance for tab switching
        window.deviceManager = this;
    }

    generateConfigFields(section, fields) {
        let fieldsHTML = '';
        
        Object.keys(fields).forEach(fieldName => {
            const field = fields[fieldName];
            const fieldId = `${section}_${fieldName}`;
            
            fieldsHTML += `<div class="config-field">`;
            fieldsHTML += `<label class="form-label" for="${fieldId}">${field.label}</label>`;
            
            if (field.type === 'string' || field.type === 'password') {
                fieldsHTML += `<input type="${field.type}" class="form-control" id="${fieldId}" value="${field.default}" ${field.required ? 'required' : ''}>`;
            } else if (field.type === 'number') {
                fieldsHTML += `<input type="number" class="form-control" id="${fieldId}" value="${field.default}" min="${field.min || 0}" max="${field.max || 999999}" ${field.required ? 'required' : ''}>`;
            } else if (field.type === 'boolean') {
                fieldsHTML += `<div class="form-check"><input type="checkbox" class="form-check-input" id="${fieldId}" ${field.default ? 'checked' : ''}><label class="form-check-label" for="${fieldId}">Enable</label></label></div>`;
            } else if (field.type === 'select') {
                fieldsHTML += `<select class="form-select" id="${fieldId}">`;
                
                // Handle options that can be either string or array
                let optionsArray = [];
                if (Array.isArray(field.options)) {
                    optionsArray = field.options;
                } else if (typeof field.options === 'string') {
                    optionsArray = field.options.split(',').map(opt => opt.trim());
                }
                
                optionsArray.forEach(option => {
                    const isSelected = option === field.default;
                    fieldsHTML += `<option value="${option}" ${isSelected ? 'selected' : ''}>${option}</option>`;
                });
                fieldsHTML += `</select>`;
            }
            
            if (field.description) {
                fieldsHTML += `<div class="form-text">${field.description}</div>`;
            }
            
            fieldsHTML += `</div>`;
        });
        
        return fieldsHTML;
    }

    showConfigTab(sectionName) {
        // Hide all tab panels
        document.querySelectorAll('.tab-panel').forEach(panel => {
            panel.classList.remove('active');
        });
        
        // Remove active class from all tab buttons
        document.querySelectorAll('.tab-button').forEach(button => {
            button.classList.remove('active');
        });
        
        // Show selected tab panel
        const selectedPanel = document.getElementById(`tab-${sectionName}`);
        if (selectedPanel) {
            selectedPanel.classList.add('active');
        }
        
        // Add active class to clicked button
        event.target.classList.add('active');
    }

    updateConfigFormWithValues(config) {
        // Update form fields with current configuration values
        Object.keys(config).forEach(section => {
            Object.keys(config[section]).forEach(fieldName => {
                const fieldId = `${section}_${fieldName}`;
                const element = document.getElementById(fieldId);
                
                if (element) {
                    const value = config[section][fieldName];
                    
                    if (element.type === 'checkbox') {
                        element.checked = Boolean(value);
                    } else if (element.type === 'number') {
                        element.value = value;
                    } else {
                        element.value = value;
                    }
                }
            });
        });
        
        // Enable save/export buttons
        this.saveConfigBtn.disabled = false;
        this.exportConfigBtn.disabled = false;
        
        // Force update of monitoring panels to reflect new configuration
        this.updateIOStatusPanel();
    }

    async saveConfiguration() {
        if (!this.isSerialConnected) {
            this.log('Please connect to device first', 'error', 'serial');
            return;
        }

        try {
            this.log('Saving configuration to device...', 'info', 'serial');
            
            // Collect form data
            const config = this.collectConfigData();
            
            // Update currentConfig with new values immediately
            this.currentConfig = config;
            
            // Send configuration to device
            await this.sendSerialData(`CONFIG_SAVE ${JSON.stringify(config)}`);
            
            this.log('Configuration saved successfully', 'success', 'serial');
            
            // Force update of monitoring panels to reflect new configuration
            this.updateIOStatusPanel();
            
        } catch (error) {
            this.log('Error saving configuration: ' + error.message, 'error', 'serial');
        }
    }

    collectConfigData() {
        if (!this.configSchema) {
            return {};
        }

        const config = {};

        Object.keys(this.configSchema).forEach(section => {
            config[section] = {};
            
            Object.keys(this.configSchema[section]).forEach(fieldName => {
                const fieldId = `${section}_${fieldName}`;
                const element = document.getElementById(fieldId);
                
                if (element) {
                    let value;
                    
                    if (element.type === 'checkbox') {
                        value = element.checked;
                    } else if (element.type === 'number') {
                        value = parseInt(element.value) || 0;
                    } else {
                        value = element.value;
                    }
                    
                    config[section][fieldName] = value;
                } else {
                    config[section][fieldName] = this.configSchema[section][fieldName].default;
                }
            });
        });

        return config;
    }

    exportConfiguration() {
        const config = this.collectConfigData();
        const configData = {
            device: this.deviceInfo,
            configuration: config,
            timestamp: new Date().toISOString()
        };
        
        const blob = new Blob([JSON.stringify(configData, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `esp32_config_${new Date().toISOString().split('T')[0]}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
        
        this.log('Configuration exported successfully', 'success', 'serial');
    }

    async restartDevice() {
        if (!this.isSerialConnected) {
            this.log('Please connect to device first', 'error', 'serial');
            return;
        }

        try {
            this.log('Restarting device...', 'info', 'serial');
            await this.sendSerialData('RESTART');
            this.log('Restart command sent', 'success', 'serial');
        } catch (error) {
            this.log(`Failed to restart device: ${error.message}`, 'error', 'serial');
        }
    }

    clearDeviceInfo() {
        this.deviceInfoContainer.innerHTML = `
            <div class="alert alert-secondary">
                <i class="fas fa-microchip"></i>
                <strong>Device Info:</strong> Connect to a device to view information.
            </div>
        `;
    }

    clearConfigForm() {
        this.configFormContainer.innerHTML = `
            <div class="alert alert-warning">
                <i class="fas fa-exclamation-triangle"></i>
                <strong>No Configuration Loaded:</strong> Click "Load Configuration" to generate the configuration form.
            </div>
        `;
    }

    log(message, type = 'info', target = 'serial') {
        if (target === 'serial' && this.serialLogPaused) return;
        if (target === 'debug' && !this.debugMode) return; // Only log debug messages if debugMode is enabled

        const timestamp = this.serialShowTimestamps ?
            `[${new Date().toLocaleTimeString()}]` : '';
        
        const output = target === 'serial' ? this.serialOutput : this.debugOutput;
        const lineCount = target === 'serial' ? this.serialLineCount : this.debugLineCount;

        if (!output) return;

        const logEntry = document.createElement('div');
        logEntry.className = `log-entry ${type}`;
        
        const timestampSpan = document.createElement('span');
        timestampSpan.className = 'timestamp';
        timestampSpan.textContent = timestamp;
        
        const messageSpan = document.createElement('span');
        messageSpan.className = 'message';
        messageSpan.textContent = message;
        
        logEntry.appendChild(timestampSpan);
        logEntry.appendChild(messageSpan);
        
        // Add to bottom for normal flow
            output.appendChild(logEntry);

        // Update line count
        if (target === 'serial') {
            this.serialLineCount++;
        } else {
            this.debugLineCount++;
        }

        // Check if we need to remove old lines
        this.trimConsoleLines(target);
        
        // Auto-scroll if enabled
        if ((target === 'serial' && this.serialAutoscroll) ||
            (target === 'debug' && this.debugMode)) { // Auto-scroll debug console if debug mode is on
            output.scrollTop = output.scrollHeight;
        }
    }

    trimConsoleLines(target) {
        const output = target === 'serial' ? this.serialOutput : this.debugOutput;
        const lineCount = target === 'serial' ? this.serialLineCount : this.debugLineCount;

        if (lineCount > this.maxConsoleLines) {
            // Remove oldest entries (they're at the top now)
            const entries = output.querySelectorAll('.log-entry');
            const entriesToRemove = entries.length - this.maxConsoleLines;

            for (let i = 0; i < entriesToRemove; i++) {
                if (entries[i]) {
                    entries[i].remove();
                }
            }

            // Update line count
            if (target === 'serial') {
                this.serialLineCount = this.maxConsoleLines;
            } else {
                this.debugLineCount = this.maxConsoleLines;
            }
        }
    }

    // Console control methods
    toggleSerialLogPause() {
        this.serialLogPaused = !this.serialLogPaused;
        if (this.serialPauseBtn) {
            this.serialPauseBtn.innerHTML = this.serialLogPaused ? 
                '<i class="fas fa-play"></i> Resume' : 
                '<i class="fas fa-pause"></i> Pause';
        }
    }

    toggleSerialAutoscroll() {
        this.serialAutoscroll = !this.serialAutoscroll;
        if (this.serialAutoscrollBtn) {
            this.serialAutoscrollBtn.innerHTML = this.serialAutoscroll ? 
                '<i class="fas fa-arrow-down"></i> Auto' : 
                '<i class="fas fa-arrow-up"></i> Manual';
        }
    }

    toggleSerialTimestamps() {
        this.serialShowTimestamps = !this.serialShowTimestamps;
        
        // Update button text
        if (this.serialTimestampBtn) {
            this.serialTimestampBtn.innerHTML = this.serialShowTimestamps ? 
                '<i class="fas fa-clock"></i> Time' : 
                '<i class="fas fa-clock"></i> No Time';
        }
        
        // Update existing log entries
        if (this.serialOutput) {
            const entries = this.serialOutput.querySelectorAll('.log-entry');
            entries.forEach(entry => {
                const timestampSpan = entry.querySelector('.timestamp');
                if (timestampSpan) {
                    if (this.serialShowTimestamps) {
                        // Add timestamp if it doesn't exist
                        if (!timestampSpan.textContent || timestampSpan.textContent === '') {
                            timestampSpan.textContent = `[${new Date().toLocaleTimeString()}]`;
                        }
                    } else {
                        // Remove timestamp
                        timestampSpan.textContent = '';
                    }
                }
            });
        }
        
        this.log('Timestamps ' + (this.serialShowTimestamps ? 'enabled' : 'disabled'), 'info', 'serial');
    }

    flushSerialBuffer() {
        this.serialBuffer = '';
        this.log('Serial buffer flushed', 'info', 'serial');
    }

    clearSerialLog() {
        if (this.serialOutput) {
        this.serialOutput.innerHTML = '';
            this.serialLineCount = 0;
        }
    }

    clearDebugLog() {
        if (this.debugOutput) {
            this.debugOutput.innerHTML = '';
            this.debugLineCount = 0;
        }
    }

    exportSerialLog() {
        if (this.serialOutput) {
            const content = this.serialOutput.innerText;
            this.downloadLog(content, 'serial_log.txt');
        }
    }

    exportDebugLog() {
        if (this.debugOutput) {
            const content = this.debugOutput.innerText;
            this.downloadLog(content, 'debug_log.txt');
        }
    }

    downloadLog(content, filename) {
        const blob = new Blob([content], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
    }
    
    toggleDebugMode() {
        this.debugMode = !this.debugMode;
        
        if (this.debugToggleBtn) {
            if (this.debugMode) {
                this.debugToggleBtn.innerHTML = '<i class="fas fa-bug"></i> Debug ON';
                this.debugToggleBtn.classList.add('btn-warning');
                this.debugToggleBtn.classList.remove('btn-outline-secondary');
                this.connectionStatusPanel.style.display = 'block';
                this.debugConsole.style.display = 'block';
                if (this.debugExportBtn) {
                    this.debugExportBtn.style.display = 'inline-block';
                }
            } else {
                this.debugToggleBtn.innerHTML = '<i class="fas fa-bug"></i> Debug';
                this.debugToggleBtn.classList.remove('btn-warning');
                this.debugToggleBtn.classList.add('btn-outline-secondary');
                this.connectionStatusPanel.style.display = 'none';
                this.debugConsole.style.display = 'none';
                if (this.debugExportBtn) {
                    this.debugExportBtn.style.display = 'none';
                }
            }
        }
        
        this.log('Debug mode: ' + (this.debugMode ? 'ENABLED' : 'DISABLED'), 'info', 'serial');
    }

    updateStatusPanel() {
        if (!this.debugMode || !this.connectionStatusPanel) return;
        
        // Update last activity time
        if (this.lastActivityTimeElement) {
            if (this.lastActivityTime) {
                const timeDiff = Date.now() - this.lastActivityTime;
                const seconds = Math.floor(timeDiff / 1000);
                const minutes = Math.floor(seconds / 60);
                
                if (minutes > 0) {
                    this.lastActivityTimeElement.textContent = `${minutes}m ${seconds % 60}s ago`;
                } else {
                    this.lastActivityTimeElement.textContent = `${seconds}s ago`;
                }
            } else {
                this.lastActivityTimeElement.textContent = 'Never';
            }
        }
        
        // Update retry count
        if (this.retryCountElement) {
            this.retryCountElement.textContent = this.connectionRetryCount;
        }
    }

    logSystemInfo() {
        this.log('=== SYSTEM INFORMATION ===', 'info', 'debug');
        this.log('Browser: ' + navigator.userAgent, 'debug', 'debug');
        this.log('Platform: ' + navigator.platform, 'debug', 'debug');
        this.log('Language: ' + navigator.language, 'debug', 'debug');
        this.log('Web Serial API Support: ' + ('serial' in navigator), 'debug', 'debug');
        this.log('Connection Type: ' + (navigator.connection ? navigator.connection.effectiveType : 'Unknown'), 'debug', 'debug');
        this.log('Online Status: ' + navigator.onLine, 'debug', 'debug');
        this.log('Memory Info: ' + (navigator.deviceMemory ? navigator.deviceMemory + 'GB' : 'Not available'), 'debug', 'debug');
        this.log('Hardware Concurrency: ' + navigator.hardwareConcurrency, 'debug', 'debug');
        this.log('================================', 'info', 'debug');
    }

    getConnectionDiagnostics() {
        const diagnostics = {
            timestamp: new Date().toISOString(),
            connectionStatus: this.isSerialConnected,
            deviceInfo: this.serialDevice ? {
                readable: !!this.serialDevice.readable,
                writable: !!this.serialDevice.writable,
                baudRate: this.configBaudRate ? this.configBaudRate.value : 'Unknown'
            } : null,
            readerStatus: !!this.serialReader,
            writerStatus: !!this.serialWriter,
            retryCount: this.connectionRetryCount,
            lastActivity: this.lastActivityTime ? new Date(this.lastActivityTime).toISOString() : null,
            bufferSize: this.serialBuffer.length,
            debugMode: this.debugMode
        };
        
        return diagnostics;
    }

    logConnectionDiagnostics() {
        const diagnostics = this.getConnectionDiagnostics();
        this.log('=== CONNECTION DIAGNOSTICS ===', 'info', 'debug');
        this.log('Connection Status: ' + diagnostics.connectionStatus, 'debug', 'debug');
        this.log('Device Available: ' + !!diagnostics.deviceInfo, 'debug', 'debug');
        this.log('Reader Active: ' + diagnostics.readerStatus, 'debug', 'debug');
        this.log('Writer Active: ' + diagnostics.writerStatus, 'debug', 'debug');
        this.log('Retry Count: ' + diagnostics.retryCount, 'debug', 'debug');
        this.log('Last Activity: ' + (diagnostics.lastActivity || 'Never'), 'debug', 'debug');
        this.log('Buffer Size: ' + diagnostics.bufferSize + ' characters', 'debug', 'debug');
        this.log('Debug Mode: ' + diagnostics.debugMode, 'debug', 'debug');
        this.log('================================', 'info', 'debug');
    }

    handleSerialData(data) {
        if (!this.isSerialConnected) return;
        
        // Update last activity time
        this.lastActivityTime = new Date();
        this.updateStatusPanel();
        
        // Log the received data
        this.log(data, 'data', 'serial');
        
        // Check for JSON responses from device commands
        if (data.startsWith('JSON_RESPONSE: ')) {
            try {
                const jsonStr = data.substring(14); // Remove 'JSON_RESPONSE: ' prefix
                const jsonData = JSON.parse(jsonStr);
                
                // Determine which command this response is for based on content
                if (jsonData.hasOwnProperty('network_identity') && jsonData.hasOwnProperty('system_behavior')) {
                    // Check if this is a schema response (has .default values) or current config response
                    const hasDefaults = jsonData.network_identity.hierarchical_id?.hasOwnProperty('default') || 
                                       jsonData.network_identity.bit_index?.hasOwnProperty('default');
                    
                    if (hasDefaults) {
                        // Configuration schema response
                        this.log('Received configuration schema from device', 'info', 'debug');
                        this.configSchema = jsonData;
                        
                        // Populate currentConfig with the actual current values from the device
                        this.currentConfig = {
                            network_identity: {
                                hierarchical_id: jsonData.network_identity.hierarchical_id?.default ?? 0,
                                bit_index: jsonData.network_identity.bit_index?.default ?? 5,
                                device_name: jsonData.network_identity.device_name?.default ?? 'ESP32_Device'
                            },
                            system_behavior: {
                                debug_level: jsonData.system_behavior.debug_level?.default ?? 'Basic',
                                status_interval: jsonData.system_behavior.status_interval?.default ?? 200,
                                auto_report: jsonData.system_behavior.auto_report?.default ?? true,
                                test_mode: jsonData.system_behavior.test_mode?.default ?? false
                            }
                        };
                        
                        this.generateConfigForm();
                        this.saveConfigBtn.disabled = false;
                        this.exportConfigBtn.disabled = false;
                    } else {
                        // Current configuration response (from CONFIG_LOAD)
                        this.log('Received current configuration from device', 'info', 'debug');
                        this.currentConfig = jsonData;
                        
                        // Update the form with current values
                        this.updateConfigFormWithValues(jsonData);
                    }
                } else if (jsonData.hasOwnProperty('hid') && jsonData.hasOwnProperty('bit_index')) {
                    // Network status response
                    this.updateNetworkStatusPanelWithRealData(jsonData);
                } else if (jsonData.hasOwnProperty('messages_sent') || jsonData.hasOwnProperty('signal_strength')) {
                    // Network stats response
                    this.updateNetworkStatsPanelWithRealData(jsonData);
                } else if (jsonData.hasOwnProperty('input_states') && jsonData.hasOwnProperty('output_states')) {
                    // I/O status response
                    this.updateIOStatusPanelWithRealData(jsonData);
                } else if (jsonData.hasOwnProperty('memory_states') || jsonData.hasOwnProperty('analog_value1')) {
                    // Device data response
                    this.updateDeviceDataPanelWithRealData(jsonData);
                }
            } catch (error) {
                this.log('Error parsing JSON response: ' + error.message, 'error', 'debug');
            }
        } else if (data.startsWith('RESPONSE: ')) {
            // Handle simple text responses
            const response = data.substring(10); // Remove 'RESPONSE: ' prefix
            this.log('Device response: ' + response, 'info', 'debug');
        }
    }

    updateNetworkStatusPanelWithRealData(jsonData) {
        if (!this.networkStatusPanel) return;
        
        const content = this.networkStatusPanel.querySelector('#networkStatusContent');
        const indicator = this.networkStatusPanel.querySelector('#networkStatusIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'Disconnected';
            indicator.className = 'panel-status disconnected';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>Network Status:</strong> Connect to device to view network information.
                </div>
            `;
            return;
        }
        
        const networkData = {
            hid: jsonData.hid ?? 'N/A',
            bitIndex: jsonData.bit_index ?? 'N/A',
            parentHid: jsonData.parent_hid ?? 'N/A',
            isRoot: jsonData.is_root ?? false,
            isConfigured: jsonData.is_configured ?? false,
            treeDepth: jsonData.tree_depth ?? 0,
            childCount: jsonData.child_count ?? 0,
            configurationStatus: jsonData.configuration_status ?? 'Unknown'
        };
        
        indicator.textContent = 'Connected';
        indicator.className = 'panel-status connected';
        
        content.innerHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Hierarchical ID</div>
                    <div class="value number">${networkData.hid}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Bit Index</div>
                    <div class="value number">${networkData.bitIndex}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Parent HID</div>
                    <div class="value number">${networkData.parentHid}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Is Root Device</div>
                    <div class="value boolean ${networkData.isRoot ? '' : 'false'}">${networkData.isRoot ? 'Yes' : 'No'}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Configuration Status</div>
                    <div class="value string">${networkData.configurationStatus}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Tree Depth</div>
                    <div class="value number">${networkData.treeDepth}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Child Devices</div>
                    <div class="value number">${networkData.childCount}</div>
                </div>
            </div>
        `;
    }

    updateNetworkStatsPanelWithRealData(jsonData) {
        if (!this.networkStatsPanel) return;
        
        const content = this.networkStatsPanel.querySelector('#networkStatsContent');
        const indicator = this.networkStatsPanel.querySelector('#networkStatsIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'No Data';
            indicator.className = 'panel-status no-data';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>Network Statistics:</strong> Connect to device to view performance metrics.
                </div>
            `;
            return;
        }
        
        const statsData = {
            messagesSent: jsonData.messages_sent || 'N/A',
            messagesReceived: jsonData.messages_received || 'N/A',
            messagesForwarded: jsonData.messages_forwarded || 'N/A',
            messagesIgnored: jsonData.messages_ignored || 'N/A',
            securityViolations: jsonData.security_violations || 'N/A',
            lastMessageTime: jsonData.last_message_time || 'N/A',
            lastSenderMAC: jsonData.last_sender_mac || 'N/A',
            signalStrength: jsonData.signal_strength || 'N/A'
        };
        
        indicator.textContent = 'Active';
        indicator.className = 'panel-status active';
        
        content.innerHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Messages Sent</div>
                    <div class="value number">${statsData.messagesSent}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Messages Received</div>
                    <div class="value number">${statsData.messagesReceived}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Messages Forwarded</div>
                    <div class="value number">${statsData.messagesForwarded}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Messages Ignored</div>
                    <div class="value number">${statsData.messagesIgnored}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Security Violations</div>
                    <div class="value number">${statsData.securityViolations}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Last Message Time</div>
                    <div class="value timestamp">${statsData.lastMessageTime}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Last Sender MAC</div>
                    <div class="value mac">${statsData.lastSenderMAC}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Signal Strength (RSSI)</div>
                    <div class="value signal ${parseFloat(statsData.signalStrength) > -50 ? '' : parseFloat(statsData.signalStrength) > -70 ? 'weak' : 'poor'}">${statsData.signalStrength} dBm</div>
                </div>
            </div>
        `;
    }

    updateIOStatusPanelWithRealData(jsonData) {
        if (!this.ioStatusPanel) return;
        
        const content = this.ioStatusPanel.querySelector('#ioStatusContent');
        const indicator = this.ioStatusPanel.querySelector('#ioStatusIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'Disconnected';
            indicator.className = 'panel-status disconnected';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>I/O Status:</strong> Connect to device to view I/O information.
                </div>
            `;
            return;
        }
        
        const ioData = {
            inputStates: jsonData.input_states || 0,
            outputStates: jsonData.output_states || 0,
            sharedData: jsonData.shared_data_single || 0, // Backward compatibility
            myBitState: jsonData.my_bit_state_single || false, // Backward compatibility
            sharedDataArray: jsonData.shared_data_array || [jsonData.shared_data_single || 0], // Inputs (I)
            myBitStatesArray: jsonData.my_bit_states_array || [jsonData.my_bit_state_single || false], // My Inputs bit
            sharedOutputArray: jsonData.shared_output_array || [jsonData.shared_output_single || 0], // Outputs (Q)
            myOutputStatesArray: jsonData.my_output_states_array || [jsonData.my_output_state_single || false],
            inputChangeCount: jsonData.input_change_count || 0,
            lastInputChange: jsonData.last_input_change || 'N/A'
        };
        
        indicator.textContent = 'Connected';
        indicator.className = 'panel-status connected';
        
        // Create input pin display
        const inputPins = [];
        for (let i = 0; i < 3; i++) {
            const inputActive = (ioData.inputStates & (1 << i)) !== 0;
            inputPins.push(`
                <div class="io-pin ${inputActive ? 'active' : 'inactive'}">
                    <div class="pin-label">Input ${i + 1}</div>
                    <div class="pin-state">${inputActive ? 'ON' : 'OFF'}</div>
                </div>
            `);
        }
        
        // Create output pin display
        const outputPins = [];
        for (let i = 0; i < 3; i++) {
            const outputActive = (ioData.outputStates & (1 << i)) !== 0;
            outputPins.push(`
                <div class="io-pin ${outputActive ? 'active' : 'inactive'}">
                    <div class="pin-label">Output ${i + 1}</div>
                    <div class="pin-state">${outputActive ? 'ON' : 'OFF'}</div>
                </div>
            `);
        }
        
        // Create shared data bit display for all inputs
        let sharedDataHTML = '';
        
        // Get the actual device bit index
        const deviceBitIndex = this.currentConfig?.network_identity?.bit_index ?? 5;
        
        // Check if we have the new multi-input format
        if (Array.isArray(ioData.sharedDataArray) && ioData.sharedDataArray.length >= 3) {
            // New multi-input format - Horizontal inputs+outputs per bit, grouped and tiled
            sharedDataHTML += `<h6>Shared Data Overview (32 bits × 3 inputs × 3 outputs)</h6>`;
            sharedDataHTML += `<div class="shared-data-display">`;
            sharedDataHTML += `<div class="shared-data-bits horizontal-overview">`;
            
            for (let bitIndex = 0; bitIndex < 32; bitIndex++) {
                const isMyBit = bitIndex === deviceBitIndex; // Use actual device bit index
                sharedDataHTML += `<div class="bit-group ${isMyBit ? 'my-bit-group' : ''}">`;
                sharedDataHTML += `<div class="bit-group-label">B${bitIndex}</div>`;
                // Inputs row
                sharedDataHTML += `<div class="input-group">`;
                for (let inputIndex = 0; inputIndex < 3; inputIndex++) {
                    const inputSharedData = ioData.sharedDataArray[inputIndex] || 0;
                    const bitActive = (inputSharedData & (1 << bitIndex)) !== 0;
                    sharedDataHTML += `<div class="bit-item ${bitActive ? 'active' : 'inactive'} ${isMyBit ? 'my-bit' : ''}">I${inputIndex + 1}</div>`;
                }
                sharedDataHTML += `</div>`; // Close inputs
                // Outputs row
                sharedDataHTML += `<div class="input-group">`;
                for (let outIndex = 0; outIndex < 3; outIndex++) {
                    const outputSharedData = (ioData.sharedOutputArray && ioData.sharedOutputArray[outIndex]) || 0;
                    const bitActiveQ = (outputSharedData & (1 << bitIndex)) !== 0;
                    sharedDataHTML += `<div class="bit-item ${bitActiveQ ? 'active' : 'inactive'} ${isMyBit ? 'my-bit' : ''}">Q${outIndex + 1}</div>`;
                }
                sharedDataHTML += `</div>`; // Close outputs
                sharedDataHTML += `</div>`; // Close bit-group
            }
            
            sharedDataHTML += `</div>`;
            sharedDataHTML += `<small class="text-muted">Format per bit: I:[1..3] then Q:[1..3]. Highlighted group (B${deviceBitIndex}) is this device's assigned bit.${!this.isSerialConnected ? ' (Simulated Data)' : ''}</small>`;
            sharedDataHTML += `</div>`;
        } else {
            // Backward compatibility - single input format
            sharedDataHTML += `<h6>Shared Data (32-bit) - Input 1</h6>`;
            sharedDataHTML += `<div class="shared-data-display">`;
            sharedDataHTML += `<div class="shared-data-bits">`;
            
            for (let i = 0; i < 32; i++) {
                const bitActive = (ioData.sharedData & (1 << i)) !== 0;
                const isMyBit = i === 5; // This device's bit index
                sharedDataHTML += `
                    <div class="bit-item ${bitActive ? 'active' : 'inactive'} ${isMyBit ? 'my-bit' : ''}">
                        B${i}-1
                    </div>
                `;
            }
            
            sharedDataHTML += `</div>`;
            sharedDataHTML += `<small class="text-muted">Highlighted bit (B5-1) is this device's assigned bit for Input 1</small>`;
            sharedDataHTML += `</div>`;
        }
        
        sharedDataHTML += `<small class="text-muted">Format: [Input1][Input2][Input3] for each bit. Highlighted group (B${deviceBitIndex}) is this device's assigned bit.${!this.isSerialConnected ? ' (Simulated Data)' : ''}</small>`;
        sharedDataHTML += `</div>`;
        
        console.log('📄 Generated shared data HTML length:', sharedDataHTML.length);
        console.log('📄 Shared data HTML preview:', sharedDataHTML.substring(0, 200) + '...');
        
        const finalHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Input States</div>
                    <div class="value boolean ${ioData.inputStates > 0 ? 'true' : 'false'}">
                        <i class="fas fa-${ioData.inputStates > 0 ? 'check' : 'times'}"></i>
                        ${ioData.inputStates} (0b${ioData.inputStates.toString(2).padStart(3, '0')})
                    </div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Output States</div>
                    <div class="value boolean ${ioData.outputStates > 0 ? 'true' : 'false'}">
                        <i class="fas fa-${ioData.outputStates > 0 ? 'check' : 'times'}"></i>
                        ${ioData.outputStates} (0b${ioData.outputStates.toString(2).padStart(3, '0')})
                    </div>
                </div>
                <div class="monitoring-item">
                    <div class="label">My Bit State</div>
                    <div class="value boolean ${ioData.myBitState ? 'true' : 'false'}">
                        <i class="fas fa-${ioData.myBitState ? 'check' : 'times'}"></i>
                        ${ioData.myBitState ? 'Active' : 'Inactive'}
                    </div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Input Changes</div>
                    <div class="value number">${ioData.inputChangeCount}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Last Input Change</div>
                    <div class="value timestamp">${ioData.lastInputChange}</div>
                </div>
            </div>
            
            <h6>Input Pins</h6>
            <div class="io-grid">
                ${inputPins.join('')}
            </div>
            
            <h6>Output Pins</h6>
            <div class="io-grid">
                ${outputPins.join('')}
            </div>
            
            ${sharedDataHTML}
        `;
        
        console.log('📄 Final HTML length:', finalHTML.length);
        console.log('📄 Setting content.innerHTML...');
        
        content.innerHTML = finalHTML;
        
        console.log('✅ I/O Status Panel updated successfully');
    }

    updateDeviceDataPanelWithRealData(jsonData) {
        if (!this.deviceDataPanel) return;
        
        const content = this.deviceDataPanel.querySelector('#deviceDataContent');
        const indicator = this.deviceDataPanel.querySelector('#deviceDataIndicator');
        
        if (!this.isSerialConnected) {
            indicator.textContent = 'No Data';
            indicator.className = 'panel-status no-data';
            content.innerHTML = `
                <div class="alert alert-secondary">
                    <i class="fas fa-info-circle"></i>
                    <strong>Device Data:</strong> Connect to device to view device-specific data.
                </div>
            `;
            return;
        }
        
        const deviceData = {
            memoryStates: jsonData.memory_states || 'N/A',
            analogValue1: jsonData.analog_value1 || 'N/A',
            analogValue2: jsonData.analog_value2 || 'N/A',
            integerValue1: jsonData.integer_value1 || 'N/A',
            integerValue2: jsonData.integer_value2 || 'N/A',
            sequenceCounter: jsonData.sequence_counter || 'N/A',
            uptime: jsonData.uptime || 'N/A'
        };
        
        indicator.textContent = 'Active';
        indicator.className = 'panel-status active';
        
        content.innerHTML = `
            <div class="monitoring-grid">
                <div class="monitoring-item">
                    <div class="label">Memory States</div>
                    <div class="value number">0x${deviceData.memoryStates.toString(16).toUpperCase().padStart(4, '0')}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Analog Value 1</div>
                    <div class="value number">${deviceData.analogValue1}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Analog Value 2</div>
                    <div class="value number">${deviceData.analogValue2}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Integer Value 1</div>
                    <div class="value number">${deviceData.integerValue1}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Integer Value 2</div>
                    <div class="value number">${deviceData.integerValue2}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Sequence Counter</div>
                    <div class="value number">${deviceData.sequenceCounter}</div>
                </div>
                <div class="monitoring-item">
                    <div class="label">Uptime</div>
                    <div class="value timestamp">${this.formatUptime(deviceData.uptime)}</div>
                </div>
            </div>
        `;
    }
}

// Initialize the application when the page loads
document.addEventListener('DOMContentLoaded', () => {
    window.deviceManager = new ESP32DeviceManager();
});