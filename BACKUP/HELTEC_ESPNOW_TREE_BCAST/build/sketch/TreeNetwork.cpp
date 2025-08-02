#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/TreeNetwork.cpp"
#include "TreeNetwork.h"
#include "espnow_wrapper.h"
#include "debug.h"
#include "button.h"
#include "oled.h"
#include "MenuSystem.h"

// Logging macros
#define MODULE_TITLE       "TREE_NET"
#define MODULE_DEBUG_LEVEL 1
#define treeLog(msg, lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

// ============================================================================
// DEMO DATA FOR TESTING
// ============================================================================

const uint16_t TreeNetwork::DEMO_HIDS[] = {1, 12, 13, 121, 122, 131, 132, 1211, 1212};
const int TreeNetwork::DEMO_HIDS_COUNT = sizeof(DEMO_HIDS) / sizeof(DEMO_HIDS[0]);

// ============================================================================
// SINGLETON IMPLEMENTATION
// ============================================================================

TreeNetwork& TreeNetwork::getInstance() {
    static TreeNetwork instance;
    return instance;
}

TreeNetwork::TreeNetwork() : 
    autoReportingEnabled(false),
    lastAutoReportTime(0),
    currentDemoHIDIndex(0) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void TreeNetwork::initialize() {
    treeLog("TreeNetwork initialized", 3);
    autoReportingEnabled = false;
    lastAutoReportTime = millis();
}

// ============================================================================
// HID MANAGEMENT
// ============================================================================

bool TreeNetwork::configureHID(uint16_t hid) {
    bool success = DATA_MGR.setMyHID(hid);
    logTreeOperation("Configure HID", success, "HID: " + String(hid));
    return success;
}

void TreeNetwork::setManualHID(uint32_t hid) {
    // Note: The system uses uint16_t for HIDs. This will truncate the value.
    configureHID(static_cast<uint16_t>(hid));
}

void TreeNetwork::clearHIDConfiguration() {
    DATA_MGR.clearHIDFromNVM();
    autoReportingEnabled = false; // Disable auto reporting when HID cleared
    logTreeOperation("Clear HID", true, "Configuration cleared");
}

uint16_t TreeNetwork::getMyHID() const {
    return DATA_MGR.getMyHID();
}

uint16_t TreeNetwork::getParentHID() const {
    return DATA_MGR.getParentHID();
}

bool TreeNetwork::isRoot() const {
    return DATA_MGR.isRoot();
}

bool TreeNetwork::isHIDConfigured() const {
    return DATA_MGR.isHIDConfigured();
}

String TreeNetwork::getHIDStatus() const {
    if (!isHIDConfigured()) {
        return "HID not configured";
    }
    
    String status = "HID:" + DATA_MGR.formatHID(getMyHID());
    if (isRoot()) {
        status += " (ROOT)";
    } else {
        status += " P:" + DATA_MGR.formatHID(getParentHID());
    }
    return status;
}

void TreeNetwork::cycleDemoHID() {
    uint16_t newHID = DEMO_HIDS[currentDemoHIDIndex];
    currentDemoHIDIndex = (currentDemoHIDIndex + 1) % DEMO_HIDS_COUNT;
    
    if (configureHID(newHID)) {
        String status = "HID set: " + DATA_MGR.formatHID(newHID);
        if (isRoot()) {
            status += " (ROOT)";
        }
        DATA_MGR.updateStatus(status);
        treeLog("Demo HID configured: " + status, 3);
    } else {
        DATA_MGR.updateStatus("HID config failed");
        treeLog("Failed to configure demo HID", 1);
    }
}

// ============================================================================
// DATA REPORTING
// ============================================================================

bool TreeNetwork::sendDataReport() {
    if (!isHIDConfigured()) {
        DATA_MGR.updateStatus("HID not configured");
        logTreeOperation("Send Data Report", false, "HID not configured");
        return false;
    }
    
    if (isRoot()) {
        DATA_MGR.updateStatus("Root doesn't send reports");
        logTreeOperation("Send Data Report", false, "Root node cannot send reports");
        return false;
    }
    
    bool success = sendDataReportToParent();
    logTreeOperation("Send Data Report", success, "To root " + DATA_MGR.formatHID(ROOT_HID) + " via tree routing");
    
    if (success) {
        DATA_MGR.updateStatus("Data report sent");
        // Data reports don't need console messages for button events
    } else {
        DATA_MGR.updateStatus("Report send failed");
    }
    
    return success;
}

bool TreeNetwork::sendDataReportWithCurrentSensorData() {
    if (!isHIDConfigured() || isRoot()) {
        return false;
    }
    
    // Update device data with current sensor readings
    updateDeviceSensorData();
    
    return sendDataReportToParent();
}

void TreeNetwork::enableAutoReporting(bool enable) {
    if (enable && (!isHIDConfigured() || isRoot())) {
        DATA_MGR.updateStatus("Cannot auto report");
        logTreeOperation("Enable Auto Reporting", false, "Root node or HID not configured");
        return;
    }
    
    autoReportingEnabled = enable;
    lastAutoReportTime = millis(); // Reset timer
    
    if (enable) {
        DATA_MGR.updateStatus("Auto Report ON");
        logTreeOperation("Auto Reporting", true, "Enabled");
    } else {
        DATA_MGR.updateStatus("Auto Report OFF");
        logTreeOperation("Auto Reporting", true, "Disabled");
    }
}

bool TreeNetwork::isAutoReportingEnabled() const {
    return autoReportingEnabled;
}

void TreeNetwork::processAutoReporting() {
    if (!autoReportingEnabled || !isHIDConfigured() || isRoot()) {
        return;
    }
    
    if (millis() - lastAutoReportTime >= AUTO_REPORT_INTERVAL) {
        if (sendDataReportWithCurrentSensorData()) {
            treeLog("Auto data report sent to parent " + DATA_MGR.formatHID(getParentHID()), 4);
            // Auto reports don't need console messages for button events
        } else {
            treeLog("Auto data report failed", 2);
        }
        lastAutoReportTime = millis();
    }
}

// ============================================================================
// COMMAND OPERATIONS
// ============================================================================

bool TreeNetwork::sendTestCommand() {
    if (!isHIDConfigured()) {
        DATA_MGR.updateStatus("HID not configured");
        logTreeOperation("Send Test Command", false, "HID not configured");
        return false;
    }
    
    // For demo purposes, send a test command to a child device
    uint16_t targetHID = getMyHID() * 10 + 1; // First child
    uint8_t outputState = 0x55; // Test pattern
    
    bool success = sendTreeCommand(targetHID, MSG_COMMAND_SET_OUTPUTS, &outputState, 1);
    logTreeOperation("Send Test Command", success, "To device " + DATA_MGR.formatHID(targetHID));
    
    if (success) {
        DATA_MGR.updateStatus("Command sent to " + DATA_MGR.formatHID(targetHID));
    } else {
        DATA_MGR.updateStatus("Command send failed");
    }
    
    return success;
}

bool TreeNetwork::sendCommandToDevice(uint16_t targetHID, TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen) {
    if (!isHIDConfigured()) {
        return false;
    }
    
    bool success = sendTreeCommand(targetHID, cmdType, payload, payloadLen);
    logTreeOperation("Send Command", success, 
                    "Type:" + String(cmdType, HEX) + " To:" + DATA_MGR.formatHID(targetHID));
    return success;
}

bool TreeNetwork::sendSetOutputsCommand(uint16_t targetHID, uint8_t outputStates) {
    return sendCommandToDevice(targetHID, MSG_COMMAND_SET_OUTPUTS, &outputStates, 1);
}

bool TreeNetwork::sendBroadcastTreeCommand(TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen) {
    if (!isHIDConfigured()) {
        treeLog("Cannot send broadcast command, HID not configured", 2);
        return false;
    }

    uint8_t buffer[TREE_MSG_OVERHEAD + payloadLen];
    
    // Create a message with a broadcast destination HID
    if (!DATA_MGR.createTreeMessage(buffer, sizeof(buffer), BROADCAST_HID, cmdType, payload, payloadLen)) {
        treeLog("Failed to create broadcast message", 1);
        return false;
    }

    // Send to the ESP-NOW broadcast MAC address
    espnowSendData(broadcastMAC, buffer, sizeof(buffer));
    treeLog("Broadcast message queued: Type=" + String(cmdType, HEX) + " Len=" + String(sizeof(buffer)), 3);
    
    return true;
}

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

String TreeNetwork::getTreeNetworkStats() const {
    const NetworkStats& stats = DATA_MGR.getNetworkStats();
    String info = "RX:" + String(stats.messagesReceived) +
                  " FWD:" + String(stats.messagesForwarded) +
                  " IGN:" + String(stats.messagesIgnored);
    return info;
}

void TreeNetwork::resetTreeStats() {
    DATA_MGR.resetNetworkStats();
    DATA_MGR.updateStatus("Tree stats reset");
    logTreeOperation("Reset Tree Stats", true, "All statistics cleared");
}

void TreeNetwork::updateDeviceSensorData() {
    DeviceSpecificData data = DATA_MGR.getMyDeviceData();
    
    // Update with current sensor readings
    data.input_states = digitalRead(BUTTON_PIN) ? 0x01 : 0x00;
    data.output_states = autoReportingEnabled ? 0x80 : 0x00; // Indicate auto reporting
    data.analog_values[0] = analogRead(A0);
    data.analog_values[1] = millis() & 0xFFFF;
    data.integer_values[0] = DATA_MGR.getNetworkStats().messagesSent;
    data.integer_values[1] = DATA_MGR.getNetworkStats().messagesReceived;
    
    DATA_MGR.setMyDeviceData(data);
}

DeviceSpecificData TreeNetwork::getCurrentSensorData() const {
    DeviceSpecificData data = {};
    
    data.input_states = digitalRead(BUTTON_PIN) ? 0x01 : 0x00;
    data.output_states = autoReportingEnabled ? 0x80 : 0x00;
    data.analog_values[0] = analogRead(A0);
    data.analog_values[1] = millis() & 0xFFFF;
    data.integer_values[0] = DATA_MGR.getNetworkStats().messagesSent;
    data.integer_values[1] = DATA_MGR.getNetworkStats().messagesReceived;
    
    return data;
}

// ============================================================================
// ROOT NODE DATA AGGREGATION
// ============================================================================

void TreeNetwork::showAggregatedDevices() const {
    if (!isRoot()) {
        logTreeOperation("Show Aggregated Devices", false, "Only root can show aggregated data");
        return;
    }
    
    DATA_MGR.showAggregatedDevices();
    logTreeOperation("Show Aggregated Devices", true, "Displayed aggregated device list");
}

void TreeNetwork::clearAggregatedData() {
    if (!isRoot()) {
        logTreeOperation("Clear Aggregated Data", false, "Only root can clear aggregated data");
        return;
    }
    
    DATA_MGR.clearAggregatedData();
    logTreeOperation("Clear Aggregated Data", true, "All aggregated data cleared");
}

uint8_t TreeNetwork::getAggregatedDeviceCount() const {
    return DATA_MGR.getAggregatedDeviceCount();
}

// ============================================================================
// HELPER METHODS
// ============================================================================

void TreeNetwork::logTreeOperation(const String& operation, bool success, const String& details) const {
    String message = operation + ": " + (success ? "SUCCESS" : "FAILED");
    if (details.length() > 0) {
        message += " - " + details;
    }
    treeLog(message, success ? 2 : 1);
}

// ============================================================================
// ADDITIONAL METHODS FOR SERIAL COMMAND HANDLER
// ============================================================================

uint8_t TreeNetwork::getTreeDepth() const {
    // Calculate tree depth based on HID
    if (!isHIDConfigured()) {
        return 0;
    }
    
    uint16_t hid = getMyHID();
    uint8_t depth = 0;
    
    while (hid > 0) {
        depth++;
        hid = hid / 10;
    }
    
    return depth;
}

uint8_t TreeNetwork::getChildCount() const {
    // For now, return 0 as child tracking is not fully implemented
    // This could be enhanced to track actual child devices
    return 0;
} 