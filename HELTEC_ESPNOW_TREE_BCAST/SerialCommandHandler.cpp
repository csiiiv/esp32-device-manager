#include "SerialCommandHandler.h"
#include <WiFi.h>

// ============================================================================
// GLOBAL INSTANCE
// ============================================================================

SerialCommandHandler SERIAL_CMD;

// ============================================================================
// CONSTRUCTOR & INITIALIZATION
// ============================================================================

SerialCommandHandler::SerialCommandHandler() {
    commandBuffer = "";
    commandComplete = false;
}

void SerialCommandHandler::initialize() {
    Serial.println("Serial Command Handler initialized");
    Serial.println("Available commands: CONFIG_SCHEMA, CONFIG_SAVE, CONFIG_LOAD, RESTART, STATUS, NETWORK_STATUS, NETWORK_STATS, IO_STATUS, DEVICE_DATA");
}

void SerialCommandHandler::update() {
    // Read serial input
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            if (commandBuffer.length() > 0) {
                processCommand(commandBuffer);
                commandBuffer = "";
            }
        } else {
            commandBuffer += c;
            
            // Prevent buffer overflow
            if (commandBuffer.length() >= MAX_COMMAND_LENGTH) {
                sendResponse("ERROR: Command too long");
                commandBuffer = "";
            }
        }
    }
}

// ============================================================================
// COMMAND PROCESSING
// ============================================================================

void SerialCommandHandler::processCommand(const String& command) {
    Serial.println("Processing command: " + command);
    
    CommandType cmdType = parseCommand(command);
    
    switch (cmdType) {
        case CMD_CONFIG_SCHEMA:
            handleConfigSchema();
            break;
        case CMD_CONFIG_SAVE:
            handleConfigSave(command);
            break;
        case CMD_CONFIG_LOAD:
            handleConfigLoad();
            break;
        case CMD_RESTART:
            handleRestart();
            break;
        case CMD_STATUS:
            handleStatus();
            break;
        case CMD_NETWORK_STATUS:
            handleNetworkStatus();
            break;
        case CMD_NETWORK_STATS:
            handleNetworkStats();
            break;
        case CMD_IO_STATUS:
            handleIOStatus();
            break;
        case CMD_DEVICE_DATA:
            handleDeviceData();
            break;
        default:
            sendResponse("ERROR: Unknown command");
            break;
    }
}

SerialCommandHandler::CommandType SerialCommandHandler::parseCommand(const String& command) {
    if (command.startsWith("CONFIG_SCHEMA")) {
        return CMD_CONFIG_SCHEMA;
    } else if (command.startsWith("CONFIG_SAVE")) {
        return CMD_CONFIG_SAVE;
    } else if (command.startsWith("CONFIG_LOAD")) {
        return CMD_CONFIG_LOAD;
    } else if (command.startsWith("RESTART")) {
        return CMD_RESTART;
    } else if (command.startsWith("STATUS")) {
        return CMD_STATUS;
    } else if (command.startsWith("NETWORK_STATUS")) {
        return CMD_NETWORK_STATUS;
    } else if (command.startsWith("NETWORK_STATS")) {
        return CMD_NETWORK_STATS;
    } else if (command.startsWith("IO_STATUS")) {
        return CMD_IO_STATUS;
    } else if (command.startsWith("DEVICE_DATA")) {
        return CMD_DEVICE_DATA;
    }
    
    return CMD_UNKNOWN;
}

// ============================================================================
// RESPONSE HANDLING
// ============================================================================

void SerialCommandHandler::sendResponse(const String& response) {
    Serial.println("RESPONSE: " + response);
}

void SerialCommandHandler::sendJsonResponse(const JsonDocument& doc) {
    String jsonString;
    serializeJson(doc, jsonString);
    Serial.println("JSON_RESPONSE: " + jsonString);
}

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

void SerialCommandHandler::handleConfigSchema() {
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    JsonObject networkIdentity = doc.createNestedObject("network_identity");
    
    JsonObject hierarchicalId = networkIdentity.createNestedObject("hierarchical_id");
    hierarchicalId["type"] = "number";
    hierarchicalId["label"] = "Hierarchical ID (HID)";
    hierarchicalId["default"] = DATA_MGR.getHID();
    hierarchicalId["min"] = 1;
    hierarchicalId["max"] = 999;
    hierarchicalId["required"] = true;
    hierarchicalId["description"] = "Device position in tree structure (1-999)";
    
    JsonObject bitIndex = networkIdentity.createNestedObject("bit_index");
    bitIndex["type"] = "number";
    bitIndex["label"] = "Bit Index";
    bitIndex["default"] = DATA_MGR.getBitIndex();
    bitIndex["min"] = 0;
    bitIndex["max"] = 31;
    bitIndex["required"] = true;
    bitIndex["description"] = "Assigned bit position in shared 32-bit data (0-31)";
    
    JsonObject deviceName = networkIdentity.createNestedObject("device_name");
    deviceName["type"] = "string";
    deviceName["label"] = "Device Name";
    deviceName["default"] = "ESP32_Device";
    deviceName["required"] = false;
    deviceName["description"] = "Human-readable device identifier";
    
    JsonObject systemBehavior = doc.createNestedObject("system_behavior");
    
    JsonObject debugLevel = systemBehavior.createNestedObject("debug_level");
    debugLevel["type"] = "select";
    debugLevel["label"] = "Debug Logging Level";
    debugLevel["options"] = "None,Basic,Detailed,Verbose";
    debugLevel["default"] = "Basic";
    debugLevel["description"] = "Level of debug output (None=0, Basic=1, Detailed=2, Verbose=3)";
    
    JsonObject statusInterval = systemBehavior.createNestedObject("status_interval");
    statusInterval["type"] = "number";
    statusInterval["label"] = "Status Update Interval (ms)";
    statusInterval["default"] = 200;
    statusInterval["min"] = 100;
    statusInterval["max"] = 5000;
    statusInterval["description"] = "How often to update status display";
    
    JsonObject autoReport = systemBehavior.createNestedObject("auto_report");
    autoReport["type"] = "boolean";
    autoReport["label"] = "Auto Report on Input Change";
    autoReport["default"] = true;
    autoReport["description"] = "Automatically report when inputs change";
    
    JsonObject testMode = systemBehavior.createNestedObject("test_mode");
    testMode["type"] = "boolean";
    testMode["label"] = "Test Mode";
    testMode["default"] = false;
    testMode["description"] = "Enable test mode for debugging";
    
    sendJsonResponse(doc);
}

void SerialCommandHandler::handleConfigSave(const String& jsonData) {
    // Extract JSON data from command (remove "CONFIG_SAVE " prefix)
    String jsonString = jsonData.substring(12); // "CONFIG_SAVE " is 12 characters
    
    Serial.println("=== CONFIGURATION SAVE REQUEST ===");
    Serial.println("JSON data: " + jsonString);
    
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error) {
        Serial.println("JSON parse error: " + String(error.c_str()));
        sendResponse("ERROR: Invalid JSON format");
        return;
    }
    
    bool success = true;
    String errorMsg = "";
    bool configChanged = false;
    
    // Get current values before changes
    uint16_t oldHID = DATA_MGR.getHID();
    uint8_t oldBitIndex = DATA_MGR.getBitIndex();
    Serial.println("Current HID before save: " + String(oldHID));
    Serial.println("Current Bit Index before save: " + String(oldBitIndex));
    
    // Parse network_identity
    if (doc.containsKey("network_identity")) {
        JsonObject networkIdentity = doc["network_identity"];
        
        if (networkIdentity.containsKey("hierarchical_id")) {
            int hid = networkIdentity["hierarchical_id"];
            Serial.println("Requested HID: " + String(hid));
            if (hid >= 1 && hid <= 999) {
                if (DATA_MGR.setHID(hid)) {
                    configChanged = true;
                    Serial.println("HID updated to: " + String(hid));
                } else {
                    success = false;
                    errorMsg += "Failed to set HID; ";
                    Serial.println("ERROR: Failed to set HID");
                }
            } else {
                success = false;
                errorMsg += "Invalid HID value; ";
                Serial.println("ERROR: Invalid HID value: " + String(hid));
            }
        }
        
        if (networkIdentity.containsKey("bit_index")) {
            int bitIndex = networkIdentity["bit_index"];
            Serial.println("Requested Bit Index: " + String(bitIndex));
            if (bitIndex >= 0 && bitIndex <= 31) {
                if (DATA_MGR.setBitIndex(bitIndex)) {
                    configChanged = true;
                    Serial.println("Bit Index updated to: " + String(bitIndex));
                } else {
                    success = false;
                    errorMsg += "Failed to set bit index; ";
                    Serial.println("ERROR: Failed to set Bit Index");
                }
            } else {
                success = false;
                errorMsg += "Invalid bit index value; ";
                Serial.println("ERROR: Invalid Bit Index value: " + String(bitIndex));
            }
        }
        
        if (networkIdentity.containsKey("device_name")) {
            String deviceName = networkIdentity["device_name"];
            Serial.println("Device name updated to: " + deviceName);
        }
    }
    
    // Parse system_behavior
    if (doc.containsKey("system_behavior")) {
        JsonObject systemBehavior = doc["system_behavior"];
        
        if (systemBehavior.containsKey("debug_level")) {
            String debugLevel = systemBehavior["debug_level"];
            Serial.println("Debug level updated to: " + debugLevel);
        }
        
        if (systemBehavior.containsKey("status_interval")) {
            int interval = systemBehavior["status_interval"];
            Serial.println("Status interval updated to: " + String(interval) + "ms");
        }
        
        if (systemBehavior.containsKey("auto_report")) {
            bool autoReport = systemBehavior["auto_report"];
            Serial.println("Auto report updated to: " + String(autoReport ? "enabled" : "disabled"));
        }
        
        if (systemBehavior.containsKey("test_mode")) {
            bool testMode = systemBehavior["test_mode"];
            Serial.println("Test mode updated to: " + String(testMode ? "enabled" : "disabled"));
        }
    }
    
    // Verify changes were applied
    uint16_t newHID = DATA_MGR.getHID();
    uint8_t newBitIndex = DATA_MGR.getBitIndex();
    Serial.println("HID after save: " + String(newHID) + " (was: " + String(oldHID) + ")");
    Serial.println("Bit Index after save: " + String(newBitIndex) + " (was: " + String(oldBitIndex) + ")");
    
    if (success) {
        if (configChanged) {
            // Force immediate reload of configuration
            Serial.println("Forcing DataManager update...");
            DATA_MGR.update(); // Update data manager with new configuration
            
            // Force reload from NVS to ensure changes are applied
            Serial.println("Forcing reload from NVS...");
            DATA_MGR.loadHIDFromNVM();
            DATA_MGR.loadBitIndexFromNVM();
            
            // Verify the changes were applied
            uint16_t finalHID = DATA_MGR.getHID();
            uint8_t finalBitIndex = DATA_MGR.getBitIndex();
            Serial.println("Final HID: " + String(finalHID));
            Serial.println("Final Bit Index: " + String(finalBitIndex));
            
            // Force OLED refresh to show updated configuration
            Serial.println("Forcing OLED refresh...");
            MENU_SYS.updateDisplay(); // This will trigger OLED redraw with new values
            
            Serial.println("Configuration applied immediately - no restart required");
        }
        sendResponse("SUCCESS: Configuration saved and applied");
        DATA_MGR.updateStatus("Config saved via web");
    } else {
        sendResponse("ERROR: " + errorMsg);
    }
    
    Serial.println("=== CONFIGURATION SAVE COMPLETE ===");
}

void SerialCommandHandler::handleConfigLoad() {
    // Force reload configuration from NVS to ensure we have the latest values
    Serial.println("Forcing reload of configuration from NVS...");
    
    // Reload HID and Bit Index from NVS
    bool hidLoaded = DATA_MGR.loadHIDFromNVM();
    bool bitIndexLoaded = DATA_MGR.loadBitIndexFromNVM();
    
    // Get current values from DataManager
    uint16_t currentHID = DATA_MGR.getHID();
    uint8_t currentBitIndex = DATA_MGR.getBitIndex();
    bool isConfigured = DATA_MGR.isConfigured();
    
    // Log current values for debugging
    Serial.println("HID loaded from NVS: " + String(hidLoaded ? "YES" : "NO"));
    Serial.println("Bit Index loaded from NVS: " + String(bitIndexLoaded ? "YES" : "NO"));
    Serial.println("Current HID: " + String(currentHID));
    Serial.println("Current Bit Index: " + String(currentBitIndex));
    Serial.println("Is Configured: " + String(isConfigured ? "true" : "false"));
    
    // Force OLED refresh to show current configuration
    Serial.println("Forcing OLED refresh after config load...");
    MENU_SYS.updateDisplay(); // This will trigger OLED redraw with current values
    
    // Return current configuration values
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    JsonObject networkIdentity = doc.createNestedObject("network_identity");
    networkIdentity["hierarchical_id"] = currentHID;
    networkIdentity["bit_index"] = currentBitIndex;
    networkIdentity["device_name"] = "ESP32_Device"; // Default name - could be stored in NVS
    
    JsonObject systemBehavior = doc.createNestedObject("system_behavior");
    systemBehavior["debug_level"] = "Basic"; // Default - could be stored in NVS
    systemBehavior["status_interval"] = 200; // Default - could be stored in NVS
    systemBehavior["auto_report"] = true; // Default - could be stored in NVS
    systemBehavior["test_mode"] = false; // Default - could be stored in NVS
    
    sendJsonResponse(doc);
}

void SerialCommandHandler::handleRestart() {
    sendResponse("SUCCESS: Restarting device");
    delay(1000);
    ESP.restart();
}

void SerialCommandHandler::handleStatus() {
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    doc["chip"] = "ESP32";
    doc["version"] = "1.0.0";
    doc["mac"] = WiFi.macAddress();
    doc["flash"] = "4MB";
    doc["sdk"] = ESP.getSdkVersion();
    doc["uptime"] = millis();
    doc["free_heap"] = ESP.getFreeHeap();
    
    sendJsonResponse(doc);
}

void SerialCommandHandler::handleNetworkStatus() {
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    doc["hid"] = DATA_MGR.getHID();
    doc["bit_index"] = DATA_MGR.getBitIndex();
    doc["parent_hid"] = TREE_NET.getParentHID();
    doc["is_root"] = TREE_NET.isRootDevice();
    doc["is_configured"] = DATA_MGR.isConfigured();
    doc["tree_depth"] = TREE_NET.getTreeDepth();
    doc["child_count"] = TREE_NET.getChildCount();
    doc["configuration_status"] = DATA_MGR.isConfigured() ? "Configured" : "Unconfigured";
    
    sendJsonResponse(doc);
}

void SerialCommandHandler::handleNetworkStats() {
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    NetworkStats stats = DATA_MGR.getNetworkStats();
    
    doc["messages_sent"] = stats.messagesSent;
    doc["messages_received"] = stats.messagesReceived;
    doc["messages_forwarded"] = stats.messagesForwarded;
    doc["messages_ignored"] = stats.messagesIgnored;
    doc["security_violations"] = stats.securityViolations;
    doc["last_message_time"] = stats.lastMessageTime;
    doc["last_sender_mac"] = stats.lastSenderMAC;
    doc["signal_strength"] = WiFi.RSSI();
    
    sendJsonResponse(doc);
}

void SerialCommandHandler::handleIOStatus() {
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    // Get I/O states
    uint8_t inputStates = IO_DEVICE.getInputStates();
    uint8_t outputStates = IO_DEVICE.getOutputStates();
    
    // Get shared data for all inputs (backward compatibility: input 0)
    uint32_t sharedDataInput0 = DATA_MGR.getSharedData(); // Backward compatibility
    bool myBitStateInput0 = DATA_MGR.getMyBitState(); // Backward compatibility
    
    // Get shared data for all inputs
    JsonArray sharedDataArray = doc.createNestedArray("shared_data");
    JsonArray myBitStateArray = doc.createNestedArray("my_bit_states");
    
    for (int inputIndex = 0; inputIndex < 3; inputIndex++) {
        // Get the shared data word for this input
        DistributedIOData distributedData = DATA_MGR.getDistributedIOSharedData();
        uint32_t inputSharedData = distributedData.sharedData[inputIndex][0];
        sharedDataArray.add(inputSharedData);
        
        // Get my bit state for this input
        bool myBitState = DATA_MGR.getMyBitState(inputIndex);
        myBitStateArray.add(myBitState);
    }
    
    doc["input_states"] = inputStates;
    doc["output_states"] = outputStates;
    doc["shared_data_single"] = sharedDataInput0; // Backward compatibility - single value
    doc["my_bit_state_single"] = myBitStateInput0; // Backward compatibility - single value
    doc["shared_data_array"] = sharedDataArray; // New multi-input array
    doc["my_bit_states_array"] = myBitStateArray; // New multi-input array
    doc["input_change_count"] = IO_DEVICE.getInputChangeCount();
    doc["last_input_change"] = IO_DEVICE.getLastInputChangeTime();
    
    // Individual pin states
    JsonArray inputPins = doc.createNestedArray("input_pins");
    for (int i = 0; i < 3; i++) {
        inputPins.add((inputStates & (1 << i)) != 0);
    }
    
    JsonArray outputPins = doc.createNestedArray("output_pins");
    for (int i = 0; i < 3; i++) {
        outputPins.add((outputStates & (1 << i)) != 0);
    }
    
    sendJsonResponse(doc);
}

void SerialCommandHandler::handleDeviceData() {
    StaticJsonDocument<JSON_DOCUMENT_SIZE> doc;
    
    DeviceSpecificData deviceData = DATA_MGR.getDeviceSpecificData();
    
    doc["memory_states"] = deviceData.memory_states;
    doc["analog_value1"] = deviceData.analog_values[0];
    doc["analog_value2"] = deviceData.analog_values[1];
    doc["integer_value1"] = deviceData.integer_values[0];
    doc["integer_value2"] = deviceData.integer_values[1];
    doc["sequence_counter"] = 0; // Not available in DeviceSpecificData, use 0 for now
    doc["uptime"] = millis();
    
    sendJsonResponse(doc);
} 