#include "DataManager.h"
#include "debug.h"
#include "IoDevice.h"
#include "espnow_wrapper.h"
#include "TreeNetwork.h"
#include "MenuSystem.h"
#include "OutputPolicy.h"
#include <Preferences.h>

// Logging macros
#define MODULE_TITLE       "DATAMGR"
#define MODULE_DEBUG_LEVEL 1
#define dataLog(msg, lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

// ============================================================================
// SINGLETON IMPLEMENTATION
// ============================================================================

DataManager& DataManager::getInstance() {
    static DataManager instance;
    return instance;
}

DataManager::DataManager() : 
    sequenceCounter(0),
    aggregatedDeviceCount(0),
    preferences(nullptr) {
    // Initialize MAC address to zeros
    memset(nodeMac, 0, 6);
    memset(lastSenderMAC, 0, 6);
    
    // Initialize device data
    memset(&myDeviceData, 0, sizeof(DeviceSpecificData));
    
    // Initialize distributed I/O data
    memset(&distributedIOData, 0, sizeof(DistributedIOData));
    
    // Initialize aggregation arrays
    memset(globalDataArray, 0, sizeof(globalDataArray));
    memset(deviceHIDArray, 0, sizeof(deviceHIDArray));
    memset(deviceLastSeen, 0, sizeof(deviceLastSeen));
    
    // Create preferences object
    preferences = new Preferences();
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void DataManager::initialize() {
    // Get MAC address
    WiFi.macAddress(nodeMac);
    
    // Initialize system status
    systemStatus.currentStatus = "Initializing";
    systemStatus.uptime = millis();
    
    // Reset statistics
    resetNetworkStats();
    
    // Load HID from NVM
    if (loadHIDFromNVM()) {
        systemStatus.isRoot = (systemStatus.myHID == ROOT_HID);
        dataLog("HID loaded from NVM: " + formatHID(systemStatus.myHID) + 
               (systemStatus.isRoot ? " (ROOT)" : ""), 3);
    } else {
        dataLog("No HID configured - device not ready for tree network", 2);
    }
    
    // Load bit index from NVM
    if (loadBitIndexFromNVM()) {
        dataLog("Bit index loaded from NVM: " + String(systemStatus.myBitIndex), 3);
    } else {
        dataLog("No bit index configured", 2);
    }
    
    dataLog("DataManager initialized", 3);
    dataLog("MAC: " + formatMAC(nodeMac), 3);
}

void DataManager::getNodeMAC(uint8_t* mac) const {
    memcpy(mac, nodeMac, 6);
}

// ============================================================================
// HIERARCHICAL ID MANAGEMENT
// ============================================================================

bool DataManager::setMyHID(uint16_t hid) {
    if (hid == 0) {
        dataLog("Invalid HID: 0", 1);
        return false;
    }
    
    systemStatus.myHID = hid;
    systemStatus.isRoot = (hid == ROOT_HID);
    systemStatus.hidConfigured = true;
    
    saveHIDToNVM();
    
    dataLog("HID set to: " + formatHID(hid) + (systemStatus.isRoot ? " (ROOT)" : ""), 3);
    
    if (systemStatus.isRoot) {
        updateStatus("Root Node Ready");
    } else {
        updateStatus("Node " + formatHID(hid) + " Ready");
    }
    
    return true;
}

bool DataManager::isValidChild(uint16_t childHID) const {
    if (!systemStatus.hidConfigured) return false;
    
    // Child HID should have this node as parent
    return isValidParentChild(systemStatus.myHID, childHID);
}

bool DataManager::isMyDescendant(uint16_t targetHID) const {
    if (!systemStatus.hidConfigured) return false;
    
    // Convert HIDs to strings for prefix matching
    String myHIDStr = String(systemStatus.myHID);
    String targetHIDStr = String(targetHID);
    
    return targetHIDStr.startsWith(myHIDStr) && targetHID != systemStatus.myHID;
}

void DataManager::saveHIDToNVM() {
    preferences->begin("tree_network", false);
    preferences->putUShort("my_hid", systemStatus.myHID);
    preferences->putBool("hid_configured", systemStatus.hidConfigured);
    preferences->end();
    dataLog("HID saved to NVM", 4);
}

bool DataManager::loadHIDFromNVM() {
    dataLog("Loading HID from NVM...", 4);
    
    if (!preferences->begin("tree_network", true)) {
        dataLog("ERROR: Failed to open preferences for reading HID", 1);
        return false;
    }
    
    bool configured = preferences->getBool("hid_configured", false);
    dataLog("NVM hid_configured flag: " + String(configured ? "true" : "false"), 4);
    
    if (configured) {
        systemStatus.myHID = preferences->getUShort("my_hid", 0);
        systemStatus.hidConfigured = true;
        systemStatus.isRoot = (systemStatus.myHID == ROOT_HID);
        
        dataLog("NVM HID value: " + String(systemStatus.myHID), 4);
        
        if (systemStatus.myHID == 0) {
            dataLog("Invalid HID loaded from NVM: 0, clearing", 1);
            systemStatus.hidConfigured = false;
            systemStatus.isRoot = false;
        } else {
            dataLog("HID loaded from NVM: " + String(systemStatus.myHID), 2);
        }
    } else {
        dataLog("No HID configuration found in NVM", 3);
        systemStatus.myHID = 0;
        systemStatus.hidConfigured = false;
        systemStatus.isRoot = false;
    }
    
    preferences->end();
    return systemStatus.hidConfigured;
}

void DataManager::clearHIDFromNVM() {
    preferences->begin("tree_network", false);
    preferences->remove("my_hid");
    preferences->remove("hid_configured");
    preferences->end();
    
    systemStatus.myHID = 0;
    systemStatus.hidConfigured = false;
    systemStatus.isRoot = false;
    
    dataLog("HID cleared from NVM", 3);
}

// ============================================================================
// BIT INDEX MANAGEMENT
// ============================================================================

bool DataManager::setMyBitIndex(uint8_t bitIndex) {
    dataLog("setMyBitIndex called with value: " + String(bitIndex), 4);
    
    if (!isValidBitIndex(bitIndex)) {
        dataLog("Invalid bit index: " + String(bitIndex) + " (must be 0-" + String(MAX_DISTRIBUTED_IO_BITS-1) + ")", 1);
        return false;
    }
    
    dataLog("Setting bit index: " + String(bitIndex), 3);
    systemStatus.myBitIndex = bitIndex;
    systemStatus.bitIndexConfigured = true;
    
    dataLog("Bit index set in memory, now saving to NVM...", 4);
    saveBitIndexToNVM();
    
    updateStatus("Bit index set: " + String(bitIndex));
    dataLog("My bit index configured: " + String(bitIndex), 2);
    
    return true;
}

void DataManager::saveBitIndexToNVM() {
    if (!preferences) {
        dataLog("ERROR: Preferences not initialized, cannot save bit index", 1);
        return;
    }
    
    dataLog("Saving bit index to NVM: " + String(systemStatus.myBitIndex) + 
           " (configured: " + String(systemStatus.bitIndexConfigured ? "true" : "false") + ")", 4);
    
    if (!preferences->begin("tree_network", false)) {
        dataLog("ERROR: Failed to open preferences for writing", 1);
        return;
    }
    
    size_t bytesWritten1 = preferences->putUChar("my_bit_index", systemStatus.myBitIndex);
    size_t bytesWritten2 = preferences->putBool("bit_idx_conf", systemStatus.bitIndexConfigured);
    
    preferences->end();
    
    dataLog("Bit index saved to NVM: " + String(systemStatus.myBitIndex) + 
           " (bytes written: " + String(bytesWritten1) + ", " + String(bytesWritten2) + ")", 3);
    
    delay(10);
    
    // Verify the save by immediately reading it back
    preferences->begin("tree_network", true);
    bool savedConfigured = preferences->getBool("bit_idx_conf", false);
    uint8_t savedBitIndex = preferences->getUChar("my_bit_index", 255);
    preferences->end();
    
    if (savedConfigured == systemStatus.bitIndexConfigured && savedBitIndex == systemStatus.myBitIndex) {
        dataLog("Bit index save verified successfully", 4);
    } else {
        dataLog("ERROR: Bit index save verification failed! Read val: " + String(savedBitIndex) + 
               " (expected " + String(systemStatus.myBitIndex) + "), Read conf: " + String(savedConfigured) +
               " (expected " + String(systemStatus.bitIndexConfigured) + ")", 1);
    }
}

bool DataManager::loadBitIndexFromNVM() {
    if (!preferences) {
        dataLog("ERROR: Preferences not initialized, cannot load bit index", 1);
        return false;
    }
    
    dataLog("Loading bit index from NVM...", 4);
    
    if (!preferences->begin("tree_network", true)) {
        dataLog("ERROR: Failed to open preferences for reading", 1);
        return false;
    }
    
    systemStatus.bitIndexConfigured = preferences->getBool("bit_idx_conf", false);
    dataLog("NVM bit_index_configured flag: " + String(systemStatus.bitIndexConfigured ? "true" : "false"), 4);
    
    if (systemStatus.bitIndexConfigured) {
        systemStatus.myBitIndex = preferences->getUChar("my_bit_index", 255);
        dataLog("NVM bit index value: " + String(systemStatus.myBitIndex), 4);
        
        if (!isValidBitIndex(systemStatus.myBitIndex)) {
            dataLog("Invalid bit index loaded from NVM: " + String(systemStatus.myBitIndex) + ", clearing", 1);
            systemStatus.bitIndexConfigured = false;
            systemStatus.myBitIndex = 255;
        } else {
            dataLog("Bit index loaded from NVM: " + String(systemStatus.myBitIndex), 2);
            updateStatus("Bit index restored: " + String(systemStatus.myBitIndex));
        }
    } else {
        dataLog("No bit index configuration found in NVM", 3);
        systemStatus.myBitIndex = 255;
    }
    
    preferences->end();
    return systemStatus.bitIndexConfigured;
}

void DataManager::clearBitIndexFromNVM() {
    preferences->begin("tree_network", false);
    preferences->remove("my_bit_index");
    preferences->remove("bit_idx_conf");
    preferences->end();
    
    systemStatus.myBitIndex = 255;
    systemStatus.bitIndexConfigured = false;
    
    dataLog("Bit index cleared from NVM", 3);
}

// ============================================================================
// COMBINED DEVICE CONFIGURATION
// ============================================================================

void DataManager::clearAllConfiguration() {
    systemStatus.hidConfigured = false;
    systemStatus.myHID = 0;
    systemStatus.isRoot = false;
    systemStatus.bitIndexConfigured = false;
    systemStatus.myBitIndex = 255;
    
    clearHIDFromNVM();
    clearBitIndexFromNVM();
    
    updateStatus("All configuration cleared");
    dataLog("All device configuration cleared", 2);
}

// ============================================================================
// ROOT NODE DATA AGGREGATION
// ============================================================================

bool DataManager::updateDeviceData(uint16_t srcHID, const DeviceSpecificData& data) {
    if (!systemStatus.isRoot) {
        dataLog("Only root can aggregate device data", 2);
        return false;
    }
    
    // Find existing entry or create new one
    int index = findDeviceIndex(srcHID);
    
    if (index == -1) {
        // Create new entry if space available
        if (aggregatedDeviceCount >= MAX_AGGREGATED_DEVICES) {
            dataLog("Maximum aggregated devices reached", 2);
            return false;
        }
        
        index = aggregatedDeviceCount;
        deviceHIDArray[index] = srcHID;
        aggregatedDeviceCount++;
        
        dataLog("New device added to aggregation: " + formatHID(srcHID) + 
               " (total: " + String(aggregatedDeviceCount) + ")", 3);
    }
    
    // Update data and timestamp
    globalDataArray[index] = data;
    deviceLastSeen[index] = millis();
    
    dataLog("Updated aggregated data for device " + formatHID(srcHID) + 
           " at index " + String(index), 4);
    
    // **CUSTOM DISTRIBUTED I/O LOGIC**
    // After updating device data, compute and broadcast shared data
    // Skip during early initialization to avoid potential loops/crashes
    static bool initializationComplete = false;
    if (millis() > 5000) { // Allow 5 seconds for full system initialization
        initializationComplete = true;
    }
    
    if (initializationComplete) {
        computeAndBroadcastDistributedIO();
    }
    
    return true;
}

const DeviceSpecificData* DataManager::getDeviceData(uint16_t srcHID) const {
    if (!systemStatus.isRoot) return nullptr;
    
    int index = findDeviceIndex(srcHID);
    if (index == -1) return nullptr;
    
    return &globalDataArray[index];
}

int DataManager::findDeviceIndex(uint16_t srcHID) const {
    for (int i = 0; i < aggregatedDeviceCount; i++) {
        if (deviceHIDArray[i] == srcHID) {
            return i;
        }
    }
    return -1;
}

void DataManager::showAggregatedDevices() const {
    if (!systemStatus.isRoot) {
        dataLog("Only root has aggregated data", 2);
        return;
    }
    
    dataLog("Devices: " + String(aggregatedDeviceCount) + "/" + String(MAX_AGGREGATED_DEVICES), 3);
    
    dataLog("Aggregated devices (" + String(aggregatedDeviceCount) + "):", 3);
    for (int i = 0; i < aggregatedDeviceCount; i++) {
        uint32_t secondsAgo = (millis() - deviceLastSeen[i]) / 1000;
        dataLog("  [" + String(i) + "] HID:" + formatHID(deviceHIDArray[i]) + 
               " LastSeen:" + String(secondsAgo) + "s ago", 3);
    }
}

void DataManager::clearAggregatedData() {
    if (!systemStatus.isRoot) {
        dataLog("Only root can clear aggregated data", 2);
        return;
    }
    
    memset(globalDataArray, 0, sizeof(globalDataArray));
    memset(deviceHIDArray, 0, sizeof(deviceHIDArray));
    memset(deviceLastSeen, 0, sizeof(deviceLastSeen));
    aggregatedDeviceCount = 0;
    
    updateStatus("Aggregated data cleared");
    dataLog("All aggregated device data cleared", 3);
}

// ============================================================================
// MESSAGE CREATION AND VALIDATION
// ============================================================================

bool DataManager::createTreeMessage(uint8_t* buffer, size_t bufferSize, uint16_t destHID, 
                                   TreeMessageType msgType, const uint8_t* payload, size_t payloadLen) {
    
    size_t totalLen = TREE_MSG_OVERHEAD + payloadLen;
    if (totalLen > bufferSize || totalLen > 255) {
        dataLog("Message too large: " + String(totalLen), 1);
        return false;
    }
    
    // Build message
    TreeMessageHeader* header = (TreeMessageHeader*)buffer;
    header->soh = TREE_MSG_SOH;
    header->frame_len = totalLen;
    header->dest_hid = destHID;
    header->src_hid = systemStatus.myHID;           // Original source (me)
    header->broadcaster_hid = systemStatus.myHID;   // Initial broadcaster (me)
    header->msg_type = msgType;
    header->seq_num = getNextSequenceNumber();
    
    // Copy payload
    if (payload && payloadLen > 0) {
        memcpy(buffer + TREE_MSG_HEADER_SIZE, payload, payloadLen);
    }
    
    // Calculate CRC over frame_len to end of payload
    uint8_t crc = calculateCRC8(buffer + 1, TREE_MSG_HEADER_SIZE - 1 + payloadLen);
    buffer[TREE_MSG_HEADER_SIZE + payloadLen] = crc;
    
    // Add EOT
    buffer[TREE_MSG_HEADER_SIZE + payloadLen + 1] = TREE_MSG_EOT;
    
    dataLog("Created message: Type=" + String(msgType, HEX) + 
           " Dest=" + formatHID(destHID) + 
           " Src=" + formatHID(header->src_hid) +
           " Broadcaster=" + formatHID(header->broadcaster_hid) +
           " Len=" + String(totalLen), 4);
    
    return true;
}

uint8_t DataManager::calculateCRC8(const uint8_t* data, size_t len) {
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;  // CRC-8 polynomial
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool DataManager::validateTreeMessage(const uint8_t* data, int len) {
    if (len < TREE_MSG_OVERHEAD) {
        return false;
    }
    
    const TreeMessageHeader* header = (const TreeMessageHeader*)data;
    
    // Check SOH and EOT
    if (header->soh != TREE_MSG_SOH || data[len - 1] != TREE_MSG_EOT) {
        return false;
    }
    
    // Check frame length
    if (header->frame_len != len) {
        return false;
    }
    
    // Verify CRC
    size_t payloadLen = len - TREE_MSG_OVERHEAD;
    uint8_t expectedCRC = calculateCRC8(data + 1, TREE_MSG_HEADER_SIZE - 1 + payloadLen);
    uint8_t receivedCRC = data[len - 2]; // CRC is second to last byte
    
    return (expectedCRC == receivedCRC);
}

// ============================================================================
// ROUTING LOGIC (BROADCAST-BASED)
// ============================================================================

bool DataManager::shouldProcessMessage(uint16_t destHID, uint16_t srcHID) const {
    if (!systemStatus.hidConfigured) return false;
    
    // Always process if message is for me
    return (destHID == systemStatus.myHID);
}

bool DataManager::shouldForwardUpstream(uint16_t destHID, uint16_t broadcasterHID) const {
    if (!systemStatus.hidConfigured) return false;
    
    // Don't forward if I'm the root - I'm the final destination
    if (systemStatus.isRoot) return false;
    
    // Forward upstream if:
    // 1. Message is addressed to the root (destHID == ROOT_HID), OR
    // 2. Message is addressed to my ancestor (destHID is my parent/grandparent/etc)
    // AND the broadcaster is my valid child
    
    bool shouldForward = false;
    
    if (destHID == ROOT_HID) {
        // Always forward messages to root
        shouldForward = true;
    } else if (destHID != systemStatus.myHID) {
        // Check if destination is my ancestor (parent, grandparent, etc.)
        uint16_t currentHID = systemStatus.myHID;
        while (currentHID > ROOT_HID) {
            currentHID = currentHID / 10; // Move up one level
            if (currentHID == destHID) {
                shouldForward = true;
                break;
            }
        }
    }
    
    if (shouldForward) {
        // Security check: verify immediate broadcaster is my child
        if (isValidParentChild(systemStatus.myHID, broadcasterHID)) {
            return true;
        } else {
            // Security violation: immediate broadcaster claims to be my child but isn't
            dataLog("Security violation: " + formatHID(broadcasterHID) + 
                   " claims to be child of " + formatHID(systemStatus.myHID), 1);
            return false;
        }
    }
    
    return false;
}

bool DataManager::shouldForwardDownstream(uint16_t destHID, uint16_t broadcasterHID) const {
    if (!systemStatus.hidConfigured) return false;
    
    // Filter 1: Packet from my hierarchical parent?
    // Use broadcaster_hid to validate immediate sender
    uint16_t myParentHID = getParentHID();
    if (broadcasterHID != myParentHID) {
        return false; // Not from my immediate parent, ignore
    }
    
    // Filter 2: Is target me? (already handled in shouldProcessMessage)
    if (destHID == systemStatus.myHID) {
        return false; // Will be processed, not forwarded
    }
    
    // Filter 3: Is target my descendant?
    if (isMyDescendant(destHID)) {
        return true; // Forward to descendant
    }
    
    return false; // Target not my descendant, ignore
}

bool DataManager::isValidParentChild(uint16_t parentHID, uint16_t childHID) const {
    return (childHID / 10 == parentHID);
}

// ============================================================================
// MESSAGE HANDLING
// ============================================================================

bool DataManager::handleIncomingTreeMessage(const uint8_t* data, int len, const uint8_t* senderMAC, int rssi) {
    if (!validateTreeMessage(data, len)) {
        dataLog("Invalid tree message received", 2);
        incrementMessagesIgnored();
        return false;
    }
    
    const TreeMessageHeader* header = (const TreeMessageHeader*)data;
    size_t payloadLen = len - TREE_MSG_OVERHEAD;
    const uint8_t* payload = (payloadLen > 0) ? data + TREE_MSG_HEADER_SIZE : nullptr;
    
    // Update statistics
    incrementMessagesReceived();
    updateLastSender(senderMAC);
    if (rssi != 0) {
        updateSignalStrength(rssi);
    }
    
    dataLog("Tree message: Type=" + String(header->msg_type, HEX) + 
           " From=" + formatHID(header->src_hid) + 
           " To=" + formatHID(header->dest_hid) + 
           " Broadcaster=" + formatHID(header->broadcaster_hid) + 
           " Seq=" + String(header->seq_num), 3);
    
    // Add console-friendly message for data flow
    if (static_cast<TreeMessageType>(header->msg_type) == MSG_DEVICE_DATA_REPORT) {
        // Data reports don't change shared data, so no console message needed
    } else if (static_cast<TreeMessageType>(header->msg_type) == MSG_DISTRIBUTED_IO_UPDATE) {
        // Shared data updates will be handled in processDistributedIOUpdate
    }
    
    // --- SPECIAL HANDLING FOR DOWNSTREAM BROADCASTS ---
    // These messages are processed by all nodes that receive them from their parent,
    // so we handle them before the standard routing checks.
    if (static_cast<TreeMessageType>(header->msg_type) == MSG_DISTRIBUTED_IO_UPDATE) {
        processDistributedIOUpdate(header, payload, payloadLen, senderMAC);
        return true; // Message handled
    }

    // Check routing decisions for all other message types
    bool shouldProcess = shouldProcessMessage(header->dest_hid, header->src_hid);
    bool shouldForwardUp = shouldForwardUpstream(header->dest_hid, header->broadcaster_hid);
    bool shouldForwardDown = shouldForwardDownstream(header->dest_hid, header->broadcaster_hid);
    
    // Security check for upstream messages
    if (shouldForwardUp && !isValidParentChild(systemStatus.myHID, header->broadcaster_hid)) {
        dataLog("Security violation: " + formatHID(header->broadcaster_hid) + " claims to be child of " + formatHID(systemStatus.myHID), 1);
        incrementSecurityViolations();
        incrementMessagesIgnored();
        return false;
    }
    
    // Process message if it's for us
    if (shouldProcess) {
        switch (static_cast<TreeMessageType>(header->msg_type)) {
            case MSG_DEVICE_DATA_REPORT:
                processDataReport(header, payload, payloadLen, senderMAC);
                break;
                
            case MSG_DISTRIBUTED_IO_UPDATE:
                processDistributedIOUpdate(header, payload, payloadLen, senderMAC);
                break;
                
            case MSG_COMMAND_SET_OUTPUTS:
                processCommand(header, payload, payloadLen, senderMAC);
                break;
                
            case MSG_ACKNOWLEDGEMENT:
            case MSG_NACK:
                processAcknowledgement(header, payload, payloadLen, senderMAC);
                break;
                
            // Note: Bit index assignment is now done via menu, not dynamic protocol
                
            default:
                dataLog("Unknown tree message type: " + String(header->msg_type, HEX), 2);
                break;
        }
    }
    
    // Forward message if needed (implementation would be in ESP-NOW wrapper)
    if (shouldForwardUp || shouldForwardDown) {
        incrementMessagesForwarded();
        dataLog("Message needs forwarding: Up=" + String(shouldForwardUp) + 
               " Down=" + String(shouldForwardDown), 4);
        
        // Forwarding doesn't need console messages for button events
        
        return true; // Indicate message should be forwarded
    }
    
    // If we don't process or forward, it's ignored
    if (!shouldProcess && !shouldForwardUp && !shouldForwardDown) {
        incrementMessagesIgnored();
        dataLog("Message ignored (not for me, not for forwarding)", 4);
    }
    
    return shouldProcess;
}

void DataManager::processDataReport(const TreeMessageHeader* header, const uint8_t* payload, 
                                    size_t payloadLen, const uint8_t* sender) {
    if (payloadLen != sizeof(DeviceSpecificData)) {
        dataLog("Invalid data report size: " + String(payloadLen), 2);
        return;
    }
    
    const DeviceSpecificData* data = (const DeviceSpecificData*)payload;
        
    if (systemStatus.isRoot) {
        // Root node should only accept data reports from its direct children.
        // The broadcaster_hid identifies the node that sent the message to us.
        if (!isValidChild(header->broadcaster_hid)) {
            dataLog("Security: Root ignoring data report from non-child broadcaster " + formatHID(header->broadcaster_hid) + 
                   " (orig_src: " + formatHID(header->src_hid) + ")", 2);
            incrementSecurityViolations();
            return;
        }

        // Root node: aggregate data using the original source HID
        dataLog("MULTI-HOP: Root received data report from " + formatHID(header->src_hid) + 
               " (via " + formatHID(header->broadcaster_hid) + ") - " +
               "Input:" + String(data->input_states, BIN) + 
               " BitIndex:" + String(data->bit_index), 2);
        
        updateDeviceData(header->src_hid, *data);
        updateStatus("Data from " + formatHID(header->src_hid));
        
        dataLog("Data report from " + formatHID(header->src_hid) + 
               " - In:" + String(data->input_states, BIN) + 
               " Out:" + String(data->output_states, BIN), 3);
    } else {
        // Intermediate node: just log and prepare for forwarding
        dataLog("MULTI-HOP: Intermediate node " + formatHID(systemStatus.myHID) + 
               " forwarding data report from " + formatHID(header->src_hid), 2);
    }
}

void DataManager::processCommand(const TreeMessageHeader* header, const uint8_t* payload, 
                               size_t payloadLen, const uint8_t* sender) {
    // Non-root nodes process commands addressed to them
    if (!systemStatus.isRoot && header->dest_hid == systemStatus.myHID) {
        TreeMessageType cmdType = (TreeMessageType)header->msg_type;
        
        switch(cmdType) {
            case MSG_COMMAND_SET_OUTPUTS:
                if (payloadLen == 1) {
                    uint8_t outputState = payload[0];
                    IO_DEVICE.updateOutputs(outputState);
                    dataLog("CMD: Set Outputs to " + String(outputState, BIN), 2);
                }
                break;
            
            default:
                dataLog("Unknown command type received: " + String(cmdType, HEX), 2);
                break;
        }
    }
}

void DataManager::processDistributedIOUpdate(const TreeMessageHeader* header, const uint8_t* payload, size_t payloadLen, const uint8_t* sender) {
    dataLog("CHILD: Received MSG_DISTRIBUTED_IO_UPDATE - size=" + String(payloadLen) + 
           " src=" + formatHID(header->src_hid) + " broadcaster=" + formatHID(header->broadcaster_hid), 2);
    
    // Handle legacy (4 bytes), legacy multi-input (12 bytes), and current (sizeof(DistributedIOData)) formats
    const int LEGACY_ONE_INPUT_BYTES = 4;
    const int LEGACY_THREE_INPUTS_BYTES = 12; // 3 words inputs only
    if (payloadLen != sizeof(DistributedIOData) && payloadLen != LEGACY_ONE_INPUT_BYTES && payloadLen != LEGACY_THREE_INPUTS_BYTES) {
        dataLog("CHILD: Invalid distributed I/O update size: " + String(payloadLen) +
               " (expected " + String(sizeof(DistributedIOData)) + ", " + String(LEGACY_THREE_INPUTS_BYTES) + " or " + String(LEGACY_ONE_INPUT_BYTES) + ")", 1);
        return;
    }

    // For downstream messages, the key security check is that the message
    // comes from the node's direct parent. The original source (src_hid) is less
    // important than the chain of trust established by the broadcaster_hid.
    // The check for src_hid == ROOT_HID has been removed as it breaks multi-hop forwarding.

    // A non-root node should only accept messages from its direct parent.
    // The broadcaster_hid identifies the node that sent the message to us.
    uint16_t expectedParent = getParentHID();
    dataLog("CHILD: Security check - my HID: " + formatHID(systemStatus.myHID) + 
           ", expected parent: " + formatHID(expectedParent) + 
           ", broadcaster: " + formatHID(header->broadcaster_hid), 3);
    
    if (expectedParent != header->broadcaster_hid) {
        dataLog("CHILD: Security: Ignoring downstream message from non-parent broadcaster " + formatHID(header->broadcaster_hid) + 
               " (expected parent: " + formatHID(expectedParent) + ")", 1);
        incrementSecurityViolations();
        return;
    }
    
    DistributedIOData receivedData;
    memset(&receivedData, 0, sizeof(DistributedIOData));
    
    if (payloadLen == LEGACY_ONE_INPUT_BYTES) {
        // Backward compatibility: old 4-byte format (single input)
        // Copy the single word to input 0 of the new structure
        memcpy(&receivedData.sharedData[0][0], payload, 4);
        dataLog("CHILD: Received legacy 4-byte format, mapping to Input 1", 2);
    } else if (payloadLen == LEGACY_THREE_INPUTS_BYTES) {
        // Legacy 12-byte format: inputs only (3 words). Outputs remain zero.
        memcpy(&receivedData.sharedData[0][0], payload, LEGACY_THREE_INPUTS_BYTES);
        dataLog("CHILD: Received legacy 12-byte multi-input format (inputs only)", 2);
    } else {
        // Current format: full inputs+outputs struct
        memcpy(&receivedData, payload, sizeof(DistributedIOData));
        dataLog("CHILD: Received current inputs+outputs format", 2);
    }
    
    dataLog("CHILD: Device " + formatHID(systemStatus.myHID) + 
           " received shared data update from root (via " + formatHID(header->broadcaster_hid) + ") - " +
           "SharedData:" + formatDistributedIOData(receivedData), 2);
    
    // Get old shared data before updating (for backward compatibility logging)
    uint32_t oldSharedData = getDistributedIOSharedData().sharedData[0][0];
    
    // Update both DataManager's state (for display) and IoDevice's state (for outputs)
    setDistributedIOSharedData(receivedData);
    IO_DEVICE.processSharedDataUpdate(receivedData);
    
    // Log button press/release events to console (backward compatibility)
    uint32_t newSharedData = receivedData.sharedData[0][0];
    consoleLogSharedDataChange(oldSharedData, newSharedData);

    dataLog("CHILD: Processed distributed I/O update: " + formatDistributedIOData(receivedData), 2);
    
    // Forward to my direct children (tree-based distribution)
    forwardDistributedIOUpdateToChildren(receivedData);
}

void DataManager::forwardDistributedIOUpdateToChildren(const DistributedIOData& sharedData) {
    dataLog("Forwarding shared data to my children via broadcast", 3);

    // Send a single broadcast message to all listening children
    TREE_NET.sendBroadcastTreeCommand(MSG_DISTRIBUTED_IO_UPDATE, (const uint8_t*)&sharedData, sizeof(DistributedIOData));
}

void DataManager::processAcknowledgement(const TreeMessageHeader* header, const uint8_t* payload, 
                                       size_t payloadLen, const uint8_t* sender) {
    if (payloadLen < 1) return;
    
    uint8_t ackedSeq = payload[0];
    
    if (header->msg_type == MSG_ACKNOWLEDGEMENT) {
        dataLog("ACK received from " + formatHID(header->src_hid) + 
               " for seq " + String(ackedSeq), 4);
    } else {
        uint8_t reason = (payloadLen > 1) ? payload[1] : 0;
        dataLog("NACK received from " + formatHID(header->src_hid) + 
               " for seq " + String(ackedSeq) + " reason " + String(reason), 3);
    }
}

// ============================================================================
// LEGACY SUPPORT AND NETWORK STATISTICS
// ============================================================================

void DataManager::updateLastSender(const uint8_t* senderMAC) {
    networkStats.lastSenderMAC = formatMAC(senderMAC);
    memcpy(lastSenderMAC, senderMAC, 6);
    networkStats.lastMessageTime = millis();
}

void DataManager::resetNetworkStats() {
    networkStats.messagesSent = 0;
    networkStats.messagesReceived = 0;
    networkStats.messagesForwarded = 0;
    networkStats.messagesIgnored = 0;
    networkStats.securityViolations = 0;
    networkStats.lastMessageTime = 0;
    networkStats.lastSenderMAC = "None";
    networkStats.signalStrength = 0.0f;
    dataLog("Network statistics reset", 3);
}

// ============================================================================
// SYSTEM STATUS
// ============================================================================

void DataManager::updateStatus(const String& newStatus) {
    systemStatus.previousStatus = systemStatus.currentStatus;
    systemStatus.currentStatus = newStatus;
    dataLog("Status updated: " + newStatus, 4);
}

// ============================================================================
// DISTRIBUTED I/O CONTROL LOGIC
// ============================================================================

void DataManager::computeAndBroadcastDistributedIO() {
    #if !ENABLE_DISTRIBUTED_IO
    return; // Distributed I/O disabled for debugging
    #endif
    
    if (!systemStatus.isRoot) {
        return; // Only root computes distributed I/O
    }
    
    DistributedIOData newSharedData = computeSharedDataFromInputs();
    
    // Check if shared data changed
    DistributedIOData currentSharedData = getDistributedIOSharedData();
    bool dataChanged = false;
    
    // Compare all inputs and outputs in the new structure
    for (int inputIndex = 0; inputIndex < MAX_INPUTS && !dataChanged; inputIndex++) {
        for (int wordIndex = 0; wordIndex < MAX_DISTRIBUTED_IO_BITS / 32; wordIndex++) {
            if (newSharedData.sharedData[inputIndex][wordIndex] != currentSharedData.sharedData[inputIndex][wordIndex]) {
                dataChanged = true;
                break;
            }
        }
    }
    for (int outIndex = 0; outIndex < MAX_INPUTS && !dataChanged; outIndex++) {
        for (int wordIndex = 0; wordIndex < MAX_DISTRIBUTED_IO_BITS / 32; wordIndex++) {
            if (newSharedData.sharedOutputs[outIndex][wordIndex] != currentSharedData.sharedOutputs[outIndex][wordIndex]) {
                dataChanged = true;
                break;
            }
        }
    }
    
    if (dataChanged) {
        dataLog("ROOT: Shared data changed, broadcasting update", 2);
        dataLog("ROOT: Old shared data: " + formatDistributedIOData(currentSharedData), 3);
        dataLog("ROOT: New shared data: " + formatDistributedIOData(newSharedData), 3);
        
        // Log button press/release events to console (backward compatibility - use Input 1)
        consoleLogSharedDataChange(currentSharedData.sharedData[0][0], newSharedData.sharedData[0][0]);
        
        setDistributedIOSharedData(newSharedData);
        broadcastDistributedIOUpdate(newSharedData);
        
        dataLog("ROOT: Distributed I/O update: " + formatDistributedIOData(newSharedData), 2);
    } else {
        dataLog("ROOT: Shared data unchanged, no broadcast needed", 4);
    }
}

/**
 * build the tree-wide distributed I/O frame (inputs and outputs)
 *
 * Overview
 * - I (Inputs): 3 × 32-bit bitmaps. Each bitIndex corresponds to one device.
 *   If a device reports its local Input N active, we set bit "bitIndex" in
 *   sharedData[N]. The root folds its own inputs and all aggregated devices.
 * - Q (Outputs): 3 × 32-bit bitmaps. These are root-owned and define the
 *   target output state for every device (per-output line) at its bitIndex.
 *   Children do not compute outputs; they simply apply Q at their own bitIndex.
 *
 * Current policy
 * - Q is a pass-through of I (Q[N] = I[N]). This keeps the behavior consistent
 *   while we evolve the policy. To change global output logic, modify the
 *   section marked "Default output logic (Q)" below.
 *
 * Returns
 * - A fully-populated DistributedIOData structure with I and Q to be broadcast
 *   by the root, and applied by children.
 */
DistributedIOData DataManager::computeSharedDataFromInputs() const {
    DistributedIOData sharedData;
    memset(&sharedData, 0, sizeof(DistributedIOData));

    dataLog("Computing shared data from inputs...", 4);

    // --- Fold the root node's own inputs into I ---
    if (isDeviceFullyConfigured()) {
        DeviceSpecificData myData = getMyDeviceData();
        dataLog("Root node input processing - input_states: " + String(myData.input_states, BIN) + 
               " bit_index: " + String(myData.bit_index), 3);
        
        uint8_t myBitIndex = getMyBitIndex();
        if (!isValidBitIndex(myBitIndex)) {
            dataLog("ERROR: Root has invalid bit index: " + String(myBitIndex), 1);
            return sharedData;
        }
        
        // Process all three inputs (Input 1..3 => indices 0..2)
        for (int inputIndex = 0; inputIndex < MAX_INPUTS; inputIndex++) {
            // Check if this input is active (bit 0, 1, or 2)
            if ((myData.input_states & (1 << inputIndex)) != 0) {
                int wordIndex = myBitIndex / BITS_PER_WORD;
                int bitInWord = myBitIndex % BITS_PER_WORD;
                sharedData.sharedData[inputIndex][wordIndex] |= (1UL << bitInWord);
                dataLog("Root Input " + String(inputIndex + 1) + " active -> setting bit " + 
                       String(myBitIndex) + " in sharedData[" + String(inputIndex) + "]", 2);
            } else {
                dataLog("Root Input " + String(inputIndex + 1) + " not active", 4);
            }
        }
    } else {
        dataLog("Root not fully configured - HID:" + String(isHIDConfigured()) + 
               " BitIndex:" + String(isBitIndexConfigured()), 2);
    }

    // --- Fold all aggregated remote devices into I ---
    dataLog("Processing " + String(aggregatedDeviceCount) + " remote devices", 4);
    for (int i = 0; i < aggregatedDeviceCount; i++) {
        const DeviceSpecificData& deviceData = globalDataArray[i];
        uint8_t deviceBitIndex = deviceData.bit_index;
        
        if (!isValidBitIndex(deviceBitIndex)) {
            dataLog("ERROR: Ignoring input from HID " + formatHID(deviceHIDArray[i]) + 
                   " due to invalid bit index (" + String(deviceBitIndex) + 
                   "). Please re-flash the device.", 1);
            continue;
        }
        
        // Process all three inputs for this device
        for (int inputIndex = 0; inputIndex < MAX_INPUTS; inputIndex++) {
            // Check if this input is active (bit 0, 1, or 2)
            if ((deviceData.input_states & (1 << inputIndex)) != 0) {
                int wordIndex = deviceBitIndex / BITS_PER_WORD;
                int bitInWord = deviceBitIndex % BITS_PER_WORD;
                sharedData.sharedData[inputIndex][wordIndex] |= (1UL << bitInWord);
                
                dataLog("Device " + formatHID(deviceHIDArray[i]) + 
                       " (bit " + String(deviceBitIndex) + ") Input " + String(inputIndex + 1) + 
                       " active -> setting its bit in sharedData[" + String(inputIndex) + "]", 4);
            }
        }
    }
    
    // --- Compute Q (Outputs) ---
    // Delegated to OutputPolicy for easy user customization
    OutputPolicy::computeOutputsFromInputs(sharedData);

    dataLog("Final shared data computed: " + formatDistributedIOData(sharedData), 3);
    return sharedData;
}

// NOTE: Output policy moved to OutputPolicy.{h,cpp}

void DataManager::setDistributedIOSharedData(const DistributedIOData& sharedData) {
    distributedIOData = sharedData;
}

DistributedIOData DataManager::getDistributedIOSharedData() const {
    return distributedIOData;
}

void DataManager::broadcastDistributedIOUpdate(const DistributedIOData& sharedData) {
    // This function is called by the root node when the shared data changes.
    // It updates the local IoDevice instance and triggers network broadcast.
    IoDevice::getInstance().setSharedData(sharedData);

    // **CRITICAL FIX**: Update root node's own outputs based on shared data
    IoDevice::getInstance().updateOutputsFromSharedData(sharedData);

    // Trigger the actual network broadcast
    IoDevice::getInstance().broadcastSharedData();
}

// ============================================================================
// DISTRIBUTED I/O STATUS AND DIAGNOSTICS
// ============================================================================

String DataManager::getDistributedIOStatus() const {
    if (!systemStatus.isRoot) {
        return "Not root";
    }
    
    DistributedIOData sharedData = getDistributedIOSharedData();
    String status = "Shared: " + formatDistributedIOData(sharedData);
    
    return status;
}

// ============================================================================
// FORMATTING AND UTILITIES
// ============================================================================

String DataManager::formatMAC(const uint8_t* mac) const {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String DataManager::formatHID(uint16_t hid) const {
    return String(hid);
}

String DataManager::formatDistributedIOData(const DistributedIOData& data) const {
    String result = "";
    // Inputs (I)
    for (int inputIndex = 0; inputIndex < MAX_INPUTS; inputIndex++) {
        if (inputIndex > 0) result += " | ";
        result += "I" + String(inputIndex + 1) + ":";
        for (int wordIndex = 0; wordIndex < MAX_DISTRIBUTED_IO_BITS / 32; wordIndex++) {
            if (wordIndex > 0) result += " ";
            char wordStr[12];
            snprintf(wordStr, sizeof(wordStr), "0x%08lX", data.sharedData[inputIndex][wordIndex]);
            result += String(wordStr);
        }
    }
    result += " || ";
    // Outputs (Q)
    for (int outIndex = 0; outIndex < MAX_INPUTS; outIndex++) {
        if (outIndex > 0) result += " | ";
        result += "Q" + String(outIndex + 1) + ":";
        for (int wordIndex = 0; wordIndex < MAX_DISTRIBUTED_IO_BITS / 32; wordIndex++) {
            if (wordIndex > 0) result += " ";
            char wordStr[12];
            snprintf(wordStr, sizeof(wordStr), "0x%08lX", data.sharedOutputs[outIndex][wordIndex]);
            result += String(wordStr);
        }
    }
    return result;
}

void DataManager::update() {
    // Periodic maintenance tasks can be added here.
    systemStatus.uptime = millis();
}

// ============================================================================
// BACKWARD COMPATIBILITY FUNCTIONS (16-bit interface)
// ============================================================================

uint16_t DataManager::getDistributedIOSharedDataAsUint16() const {
    DistributedIOData data = getDistributedIOSharedData();
    return (uint16_t)(data.sharedData[0][0] & 0xFFFF);
}

// ============================================================================
// UTILITY FUNCTIONS FOR BIT MANIPULATION
// ============================================================================

void DataManager::setDistributedIOBit(int inputIndex, int bitIndex, bool value) {
    if (inputIndex < 0 || inputIndex >= MAX_INPUTS) {
        dataLog("Invalid input index: " + String(inputIndex), 2);
        return;
    }
    
    if (bitIndex < 0 || bitIndex >= MAX_DISTRIBUTED_IO_BITS) {
        dataLog("Invalid bit index: " + String(bitIndex), 2);
        return;
    }
    
    DistributedIOData data = getDistributedIOSharedData();
    int wordIndex = bitIndex / BITS_PER_WORD;
    int bitInWord = bitIndex % BITS_PER_WORD;
    
    if (value) {
        data.sharedData[inputIndex][wordIndex] |= (1UL << bitInWord);
    } else {
        data.sharedData[inputIndex][wordIndex] &= ~(1UL << bitInWord);
    }
    
    setDistributedIOSharedData(data);
}

bool DataManager::getDistributedIOBit(int inputIndex, int bitIndex) const {
    if (inputIndex < 0 || inputIndex >= MAX_INPUTS) {
        return false;
    }
    
    if (bitIndex < 0 || bitIndex >= MAX_DISTRIBUTED_IO_BITS) {
        return false;
    }
    
    int wordIndex = bitIndex / BITS_PER_WORD;
    int bitPosition = bitIndex % BITS_PER_WORD;
    
    return (distributedIOData.sharedData[inputIndex][wordIndex] & (1UL << bitPosition)) != 0;
}

bool DataManager::getMyBitState(int inputIndex) const {
    // Get the state of this device's assigned bit for specific input
    if (systemStatus.bitIndexConfigured) {
        return getDistributedIOBit(inputIndex, systemStatus.myBitIndex);
    }
    return false;
}

// Backward compatibility functions
void DataManager::setDistributedIOBit(int bitIndex, bool value) {
    // Maps to input 0 for compatibility
    setDistributedIOBit(0, bitIndex, value);
}

bool DataManager::getDistributedIOBit(int bitIndex) const {
    // Maps to input 0 for compatibility
    return getDistributedIOBit(0, bitIndex);
}

bool DataManager::getMyBitState() const {
    // Maps to input 0 for compatibility
    return getMyBitState(0);
}

// ============================================================================
// ADDITIONAL METHODS FOR SERIAL COMMAND HANDLER
// ============================================================================

uint32_t DataManager::getSharedData() const {
    // Return the first word of input 0 distributed I/O data as uint32 for compatibility
    return distributedIOData.sharedData[0][0];
}

uint16_t DataManager::getParentHID() const {
    if (systemStatus.isRoot || !systemStatus.hidConfigured) {
        return 0; // Root has no parent
    }
    return systemStatus.myHID / 10;
} 