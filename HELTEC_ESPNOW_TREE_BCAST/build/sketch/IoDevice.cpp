#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/IoDevice.cpp"
#include "IoDevice.h"
#include "TreeNetwork.h"
#include "espnow_wrapper.h"
#include "debug.h"

// Logging macros
#define MODULE_TITLE       "IO_DEVICE"
#define MODULE_DEBUG_LEVEL 1
#define ioLog(msg, lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

// ============================================================================
// SINGLETON IMPLEMENTATION
// ============================================================================

IoDevice& IoDevice::getInstance() {
    static IoDevice instance;
    return instance;
}

IoDevice::IoDevice() :
    inputCount(0),
    outputCount(0),
    pinsConfigured(false),
    currentInputStates(0),
    previousInputStates(0),
    lastInputScan(0),
    lastDebounceTime(0),
    inputChanged(false),
    inputChangeCount(0),
    lastInputChangeTime(0),
    currentOutputStates(0),
    autoReportOnChange(true),
    lastReportTime(0),
    testModeEnabled(false) {  // Start with test mode enabled for range testing
    
    // Initialize arrays
    memset(inputPins, 0, sizeof(inputPins));
    memset(outputPins, 0, sizeof(outputPins));
    memset(&distributedIOData, 0, sizeof(DistributedIOData));
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void IoDevice::initialize() {
    ioLog("IoDevice initialization started", 3);
    
    // Initialize member variables first
    inputCount = 0;
    outputCount = 0;
    pinsConfigured = false;
    currentInputStates = 0;
    previousInputStates = 0;
    lastInputScan = 0;
    lastDebounceTime = 0;
    inputChanged = false;
    inputChangeCount = 0;
    lastInputChangeTime = 0;
    currentOutputStates = 0;
    autoReportOnChange = true;
    lastReportTime = 0;
    
    ioLog("Member variables initialized", 4);
    
    #if ENABLE_IO_DEVICE_PINS
    // Heltec V3 pin configuration
    // Input pins: 7, 6, 5 (as requested) - GPIO_0 handled by button system
    // Output pins: 4, 3, 2 (as requested)
    // OLED uses: GPIO17 (SDA), GPIO18 (SCL), GPIO21 (RST), GPIO36 (VEXT)
    // Button uses: GPIO0 (boot/prog button) - handled separately by button system
    uint8_t defaultInputs[] = {7, 6, 5};     // 3 input pins (7,6,5 as requested)
    uint8_t defaultOutputs[] = {4, 3, 2};   // 3 output pins (4,3,2 as requested)
    
    ioLog("About to configure pins", 4);
    configurePins(defaultInputs, 3, defaultOutputs, 3);
    ioLog("Pin configuration complete", 3);
    #else
    ioLog("Pin configuration disabled for debugging", 2);
    pinsConfigured = false;
    #endif
    
    ioLog("IoDevice initialization complete", 3);
}

void IoDevice::configurePins(const uint8_t* inputPins, uint8_t inputCount, 
                            const uint8_t* outputPins, uint8_t outputCount) {
    
    // Validate parameters
    if (inputCount > MAX_INPUT_PINS || outputCount > MAX_OUTPUT_PINS) {
        ioLog("Pin count exceeds maximum", 1);
        return;
    }
    
    // Define reserved pins that should not be used (OLED, SPI, radio, battery, etc.)
    // Heltec V3 pins from heltec.h:
    // - OLED: GPIO17 (SDA), GPIO18 (SCL), GPIO21 (RST)
    // - SPI: GPIO8 (SS), GPIO10 (MOSI), GPIO11 (MISO), GPIO9 (SCK)
    // - Radio: GPIO14 (DIO1), GPIO12 (RST_LoRa), GPIO13 (BUSY_LoRa)
    // - Battery: GPIO37 (VBAT_CTRL), GPIO1 (VBAT_ADC)
    // - External power: GPIO36 (VEXT)
    // - LED: GPIO35
    // - Boot pins: GPIO45, GPIO46
    // Note: GPIO_0 is intentionally NOT reserved as it's used for both button and I/O
    uint8_t reservedPins[] = {1, 8, 9, 10, 11, 12, 13, 14, 17, 18, 21, 35, 36, 37, 45, 46};
    uint8_t reservedCount = sizeof(reservedPins) / sizeof(reservedPins[0]);
    
    // Check for conflicts with reserved pins
    for (int i = 0; i < inputCount; i++) {
        for (int j = 0; j < reservedCount; j++) {
            if (inputPins[i] == reservedPins[j]) {
                ioLog("WARNING: Input pin " + String(inputPins[i]) + " conflicts with reserved pin", 2);
            }
        }
    }
    
    for (int i = 0; i < outputCount; i++) {
        for (int j = 0; j < reservedCount; j++) {
            if (outputPins[i] == reservedPins[j]) {
                ioLog("WARNING: Output pin " + String(outputPins[i]) + " conflicts with reserved pin", 2);
            }
        }
    }
    
    this->inputCount = inputCount;
    this->outputCount = outputCount;
    
    // Configure input pins with error handling
    for (int i = 0; i < inputCount; i++) {
        this->inputPins[i] = inputPins[i];
        
        // Validate pin number for ESP32-S3
        if (inputPins[i] > 48) {
            ioLog("ERROR: Invalid input pin " + String(inputPins[i]), 1);
            continue;
        }
        
        pinMode(inputPins[i], INPUT_PULLUP);
        ioLog("Input pin " + String(inputPins[i]) + " configured", 4);
    }
    
    // Configure output pins with error handling
    for (int i = 0; i < outputCount; i++) {
        this->outputPins[i] = outputPins[i];
        
        // Validate pin number for ESP32-S3
        if (outputPins[i] > 48) {
            ioLog("ERROR: Invalid output pin " + String(outputPins[i]), 1);
            continue;
        }
        
        pinMode(outputPins[i], OUTPUT);
        digitalWrite(outputPins[i], LOW);
        ioLog("Output pin " + String(outputPins[i]) + " configured", 4);
    }
    
    pinsConfigured = true;
    currentInputStates = readInputPins();
    previousInputStates = currentInputStates;
    
    ioLog("Pin configuration complete: " + String(inputCount) + " inputs, " + 
         String(outputCount) + " outputs", 3);
}

// ============================================================================
// INPUT MANAGEMENT
// ============================================================================

void IoDevice::scanInputs() {
    if (!pinsConfigured || inputCount == 0) {
        return;
    }
    
    // Scan inputs at a fixed interval
    unsigned long now = millis();
    if (now - lastInputScan < INPUT_SCAN_INTERVAL_MS) {
        return;
    }
    lastInputScan = now;
    
    uint8_t rawStates = readInputPins();
    
    // If the raw state is different from the last time we checked, it means things are unstable.
    // We reset the debounce timer.
    if (rawStates != previousInputStates) {
        lastDebounceTime = now;
    }
    
    // After the state has been stable for DEBOUNCE_DELAY_MS, we can consider it the new official state.
    if ((now - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        if (rawStates != currentInputStates) {
            ioLog("Input change detected: " + String(rawStates, BIN) + " (was " + String(currentInputStates, BIN) + ")", 2);
            currentInputStates = rawStates;
            inputChanged = true;
            inputChangeCount++;
            lastInputChangeTime = now;

            // Update the data manager ONLY when a debounced change has occurred.
            // This ensures the display has the most up-to-date information.
            updateDeviceDataFromIO();
        }
    }
    
    previousInputStates = rawStates;
}

uint8_t IoDevice::readInputPins() {
    uint8_t states = 0;
    
    if (!pinsConfigured || inputCount == 0) {
        return states;
    }
    
    // **TESTING FEATURE**: Make input bit 0 a 1000ms toggling bit for range/forwarding tests
    if (testModeEnabled) {
        static unsigned long lastToggleTime = 0;
        static bool toggleState = false;
        
        unsigned long now = millis();
        if (now - lastToggleTime >= 1000) { // Toggle every 1000ms
            toggleState = !toggleState;
            lastToggleTime = now;
            
            // Log the toggle for debugging
            ioLog("Test bit 0 toggled to: " + String(toggleState ? "1" : "0"), 3);
        }
        
        // Set bit 0 based on toggle state instead of GPIO 0
        if (toggleState) {
            states |= 0x01; // Set bit 0
        }
        
        // Process remaining physical input pins (1-7) normally
        for (int i = 1; i < inputCount; i++) {
            bool pinState = digitalRead(inputPins[i]);
            // For pull-up inputs: HIGH = not pressed, LOW = pressed
            if (pinState == LOW) {  // Pin pulled low = button pressed = set bit to 1
                states |= (1 << i);
            }
        }
    } else {
        // Normal mode: read all pins including GPIO 0
        for (int i = 0; i < inputCount; i++) {
            bool pinState = digitalRead(inputPins[i]);
            // For pull-up inputs: HIGH = not pressed, LOW = pressed
            if (pinState == LOW) {  // Pin pulled low = button pressed = set bit to 1
                states |= (1 << i);
            }
            
            // Debug output for GPIO 0 changes
            if (inputPins[i] == 0) {
                static bool lastGPIO0State = HIGH;
                if (pinState != lastGPIO0State) {
                    ioLog("GPIO_0: " + String(pinState ? "HIGH" : "LOW") + 
                         " -> bit " + String((states & (1 << i)) ? "1" : "0"), 1);
                    lastGPIO0State = pinState;
                }
            }
        }
    }
    
    return states;
}

// ============================================================================
// OUTPUT MANAGEMENT
// ============================================================================

void IoDevice::updateOutputs(uint8_t outputStates) {
    if (!pinsConfigured) {
        ioLog("Pins not configured", 2);
        return;
    }
    
    currentOutputStates = outputStates;
    writeOutputPins(outputStates);
    
    ioLog("Outputs updated: " + String(outputStates, BIN), 3);
}

void IoDevice::updateOutputsFromSharedData(const DistributedIOData& sharedData) {
    // This maps the multi-input shared data to local outputs
    // Input 1 shared data -> Output 1-8 (bits 0-7)
    // Input 2 shared data -> Output 9-16 (bits 8-15) 
    // Input 3 shared data -> Output 17-24 (bits 16-23)
    
    uint8_t outputStates = 0;
    
    // Map Input 1 shared data to outputs 1-8 (if we have enough outputs)
    if (outputCount >= 1) {
        uint8_t input1Bits = sharedData.sharedData[0][0] & 0xFF;
        outputStates |= (input1Bits & 0x07); // Use first 3 bits for 3 outputs
    }
    
    // Map Input 2 shared data to outputs 4-6 (if we have enough outputs)
    if (outputCount >= 4) {
        uint8_t input2Bits = (sharedData.sharedData[1][0] >> 0) & 0x07;
        outputStates |= (input2Bits << 3); // Use bits 3-5
    }
    
    // Map Input 3 shared data to outputs 7-9 (if we have enough outputs)
    if (outputCount >= 7) {
        uint8_t input3Bits = (sharedData.sharedData[2][0] >> 0) & 0x07;
        outputStates |= (input3Bits << 6); // Use bits 6-8
    }
    
    ioLog("Updating outputs from multi-input shared data - old outputs: " + String(currentOutputStates, BIN) + 
         " new outputs: " + String(outputStates, BIN) + 
         " (shared: " + DATA_MGR.formatDistributedIOData(sharedData) + ")", 2);
    
    updateOutputs(outputStates);
    
    ioLog("Outputs updated from multi-input shared data: " + String(outputStates, BIN) + 
         " (shared: " + DATA_MGR.formatDistributedIOData(sharedData) + ")", 3);

    // Manually update the data manager to ensure the display reflects the new output state immediately.
    updateDeviceDataFromIO();
}

void IoDevice::writeOutputPins(uint8_t states) {
    if (!pinsConfigured || outputCount == 0) {
        return;
    }
    
    for (int i = 0; i < outputCount; i++) {
        bool state = (states >> i) & 0x01;
        digitalWrite(outputPins[i], state ? HIGH : LOW);
    }
}

// ============================================================================
// SHARED DATA MANAGEMENT
// ============================================================================

void IoDevice::setSharedData(const DistributedIOData& data) {
    // This function is likely obsolete now that the root computes shared data, but keep for testing
    if (memcmp(&distributedIOData, &data, sizeof(DistributedIOData)) != 0) {
        distributedIOData = data;
        ioLog("Shared data updated: " + DATA_MGR.formatDistributedIOData(data), 3);
        
        // If root node, broadcast to all devices.
        // **REMOVED**: This is now handled by DataManager::computeAndBroadcastDistributedIO to prevent duplicate broadcasts.
        // if (DATA_MGR.isRoot()) {
        //     broadcastSharedData();
        // }
    }
}

void IoDevice::broadcastSharedData() {
    if (!DATA_MGR.isRoot()) {
        ioLog("Only root can broadcast shared data", 2);
        return;
    }
    
    ioLog("ROOT: Starting shared data broadcast", 2);
    
    // Create shared data payload
    DistributedIOData data = DATA_MGR.getDistributedIOSharedData();
    
    ioLog("ROOT: Broadcasting shared data: " + DATA_MGR.formatDistributedIOData(data), 2);
    
    // Send a single broadcast message to all listening children
    TREE_NET.sendBroadcastTreeCommand(MSG_DISTRIBUTED_IO_UPDATE, (const uint8_t*)&data, sizeof(DistributedIOData));
}

void IoDevice::processSharedDataUpdate(const DistributedIOData& newSharedData) {
    if (DATA_MGR.isRoot()) {
        ioLog("Root node ignores shared data updates", 4);
        return;
    }
    
    ioLog("CHILD: Processing shared data update - old shared: " + DATA_MGR.formatDistributedIOData(distributedIOData) + 
         " new shared: " + DATA_MGR.formatDistributedIOData(newSharedData), 2);
    
    // Update the local copy of the shared data
    distributedIOData = newSharedData;
    
    // Update local outputs based on the new shared data
    updateOutputsFromSharedData(distributedIOData);
    
    ioLog("CHILD: Processed shared data update: " + DATA_MGR.formatDistributedIOData(newSharedData), 3);
}

// ============================================================================
// AUTO REPORTING
// ============================================================================

void IoDevice::checkAndSendReport() {
    if (!TREE_NET.isHIDConfigured()) {
        return;  // Unconfigured nodes can't do anything
    }
    
    unsigned long now = millis();
    
    // Handle input changes differently for root vs non-root nodes
    if (inputChanged && (now - lastReportTime > 50)) { // Rate limit to 20Hz
        if (DATA_MGR.isRoot()) {
            // Root node: recompute and broadcast shared data when its input changes
            ioLog("Root input changed, recomputing shared data", 3);
            DATA_MGR.computeAndBroadcastDistributedIO();
        } else {
            // Non-root nodes: send data reports upstream
            ioLog("Input changed, sending data report.", 3);
            TREE_NET.sendDataReport();
        }
        
        inputChanged = false; // Reset flag for all nodes after processing
        lastReportTime = now;
    }
}

// ============================================================================
// DEVICE DATA INTEGRATION
// ============================================================================

void IoDevice::updateDeviceDataFromIO() {
    // This function is called frequently to update the local data representation
    // in DataManager with the latest I/O states.
    DeviceSpecificData data;
    data.input_states = getCurrentInputStates();
    data.output_states = getCurrentOutputStates();
    data.bit_index = DATA_MGR.getMyBitIndex(); // ** Include the configured bit index **
    data.memory_states = 0; // Not used yet
    data.analog_values[0] = 0; // Not used yet
    data.analog_values[1] = 0; // Not used yet
    data.integer_values[0] = 0; // Not used yet
    data.integer_values[1] = 0; // Not used yet
    
    // Debug: Log what we're about to send to DataManager
    ioLog("Updating DataManager with input_states: " + String(data.input_states, BIN) + 
          " (" + String(data.input_states) + "), output_states: " + String(data.output_states, BIN), 1);
    
    DATA_MGR.setMyDeviceData(data);
    
    // Debug: Verify what DataManager actually stored
    const DeviceSpecificData& storedData = DATA_MGR.getMyDeviceData();
    ioLog("DataManager now has input_states: " + String(storedData.input_states, BIN) + 
          " (" + String(storedData.input_states) + "), output_states: " + String(storedData.output_states, BIN), 1);
}

void IoDevice::processReceivedDeviceData(const DeviceSpecificData& data) {
    // This device has received data intended for it (e.g., a command)
    // For now, this is mainly for updating outputs based on some external logic
    
    // Update outputs if commanded
    if (data.output_states != currentOutputStates) {
        updateOutputs(data.output_states);
    }
    
    ioLog("Processed received device data command", 4);
}

// ============================================================================
// CONFIGURATION & STATUS
// ============================================================================

String IoDevice::getIOStatus() const {
    if (!pinsConfigured) {
        return "I/O not configured";
    }
    
    String status = "In:" + String(currentInputStates, BIN) + 
                   " Out:" + String(currentOutputStates, BIN) + 
                   " Shared:" + DATA_MGR.formatDistributedIOData(distributedIOData);
    return status;
}

void IoDevice::showPinConfiguration() const {
    if (!pinsConfigured) {
        ioLog("I/O pins not configured", 2);
        return;
    }
    
    ioLog("Pin Configuration:", 3);
    
    String inputStr = "Inputs (" + String(inputCount) + "): ";
    for (int i = 0; i < inputCount; i++) {
        inputStr += String(inputPins[i]);
        if (i < inputCount - 1) inputStr += ",";
    }
    ioLog(inputStr, 3);
    
    String outputStr = "Outputs (" + String(outputCount) + "): ";
    for (int i = 0; i < outputCount; i++) {
        outputStr += String(outputPins[i]);
        if (i < outputCount - 1) outputStr += ",";
    }
    ioLog(outputStr, 3);
    
    ioLog("Current States - " + getIOStatus(), 3);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void IoDevice::logIOOperation(const String& operation, bool success, const String& details) {
    String message = operation + ": " + (success ? "SUCCESS" : "FAILED");
    if (details.length() > 0) {
        message += " - " + details;
    }
    ioLog(message, success ? 3 : 2);
} 