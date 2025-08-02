#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/IoDevice.h"
#ifndef IODEVICE_H
#define IODEVICE_H

#include <Arduino.h>
#include "DataManager.h"

// ============================================================================
// I/O DEVICE CONFIGURATION
// ============================================================================

// Set to 0 to disable IoDevice pin configuration (for debugging)
#define ENABLE_IO_DEVICE_PINS 1

// Set to 0 to disable distributed I/O features (for debugging)
#define ENABLE_DISTRIBUTED_IO 1

// Maximum number of I/O pins supported
#define MAX_INPUT_PINS 3
#define MAX_OUTPUT_PINS 3
#define DEBOUNCE_DELAY_MS 50
#define INPUT_SCAN_INTERVAL_MS 10

// ============================================================================
// I/O DEVICE CLASS
// ============================================================================

/**
 * @brief Remote I/O Device management class
 * Handles GPIO pins, automatic reporting, and shared data processing
 */
class IoDevice {
public:
    // Singleton pattern
    static IoDevice& getInstance();
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    void initialize();
    void configurePins(const uint8_t* inputPins, uint8_t inputCount, 
                      const uint8_t* outputPins, uint8_t outputCount);
    
    // ========================================================================
    // INPUT MANAGEMENT
    // ========================================================================
    void scanInputs();              // Call in main loop
    uint8_t getCurrentInputStates() const { return currentInputStates; }
    uint8_t getInputStates() const { return currentInputStates; } // Alias for compatibility
    bool hasInputChanged() const { return inputChanged; }
    void clearInputChangedFlag() { inputChanged = false; }
    
    // ========================================================================
    // OUTPUT MANAGEMENT  
    // ========================================================================
    void updateOutputs(uint8_t outputStates);
    void updateOutputsFromSharedData(const DistributedIOData& sharedData);
    uint8_t getCurrentOutputStates() const { return currentOutputStates; }
    uint8_t getOutputStates() const { return currentOutputStates; } // Alias for compatibility
    
    // ========================================================================
    // INPUT CHANGE TRACKING
    // ========================================================================
    uint32_t getInputChangeCount() const { return inputChangeCount; }
    uint32_t getLastInputChangeTime() const { return lastInputChangeTime; }
    
    // ========================================================================
    // SHARED DATA MANAGEMENT (ROOT NODE)
    // ========================================================================
    void setSharedData(const DistributedIOData& data);
    void broadcastSharedData();     // Root node broadcasts to all devices
    void processSharedDataUpdate(const DistributedIOData& newSharedData); // Non-root processes update
    
    // ========================================================================
    // AUTO REPORTING
    // ========================================================================
    void enableAutoReportOnInputChange(bool enable) { autoReportOnChange = enable; }
    bool isAutoReportEnabled() const { return autoReportOnChange; }
    void checkAndSendReport();      // Call in main loop
    
    // ========================================================================
    // DEVICE DATA INTEGRATION
    // ========================================================================
    void updateDeviceDataFromIO();  // Updates DataManager with current I/O states
    void processReceivedDeviceData(const DeviceSpecificData& data); // Process commands
    
    // ========================================================================
    // CONFIGURATION & STATUS
    // ========================================================================
    String getIOStatus() const;
    void showPinConfiguration() const;
    
    // ========================================================================
    // TESTING FEATURES
    // ========================================================================
    void enableTestMode(bool enable) { testModeEnabled = enable; }
    bool isTestModeEnabled() const { return testModeEnabled; }
    
private:
    IoDevice();
    ~IoDevice() = default;
    IoDevice(const IoDevice&) = delete;
    IoDevice& operator=(const IoDevice&) = delete;
    
    // ========================================================================
    // PRIVATE MEMBERS
    // ========================================================================
    
    // Pin configuration
    uint8_t inputPins[MAX_INPUT_PINS];
    uint8_t outputPins[MAX_OUTPUT_PINS];
    uint8_t inputCount;
    uint8_t outputCount;
    bool pinsConfigured;
    
    // Input state tracking
    uint8_t currentInputStates;     // The final, debounced state for external use
    uint8_t previousInputStates;    // The raw state from the previous scan, for debounce checking
    unsigned long lastInputScan;
    unsigned long lastDebounceTime;   // A single debounce timer for all inputs
    bool inputChanged;
    uint32_t inputChangeCount;      // Count of input changes
    uint32_t lastInputChangeTime;   // Timestamp of last input change
    
    // Output state tracking
    uint8_t currentOutputStates;
    
    // Shared data (32 bits)
    DistributedIOData distributedIOData;
    
    // Auto reporting
    bool autoReportOnChange;
    unsigned long lastReportTime;
    
    // Testing feature
    bool testModeEnabled;
    
    // Helper functions
    void logIOOperation(const String& operation, bool success, const String& details = "");
    uint8_t readInputPins();
    void writeOutputPins(uint8_t states);
    bool debounceInput(uint8_t pinIndex, bool currentState);
};

// Global access macro
#define IO_DEVICE IoDevice::getInstance()

#endif // IODEVICE_H 