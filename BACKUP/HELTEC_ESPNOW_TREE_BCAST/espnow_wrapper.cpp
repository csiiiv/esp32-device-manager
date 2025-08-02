#include "espnow_wrapper.h"
#include "debug.h"
#include "DataManager.h"
#include "MenuSystem.h"

// Logging macros for the ESP-NOW module
#define MODULE_TITLE       "ESP-NOW"
#define MODULE_DEBUG_LEVEL 1
#define espnowLog(msg,lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

String macToString(const uint8_t* mac) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Broadcast MAC address (FF:FF:FF:FF:FF:FF)
uint8_t broadcastMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Long Range mode state
static bool longRangeModeActive = false;

// ============================================================================
// ESP32 LONG RANGE MODE FUNCTIONS
// ============================================================================

bool enableLongRangeMode() {
    espnowLog("Enabling ESP32 Long Range mode...", 3);
    
    // Set WiFi to Long Range mode
    esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
    if (result != ESP_OK) {
        espnowLog("Failed to enable LR mode on STA interface: " + String(result), 1);
        return false;
    }
    
    // Also set AP interface if available
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_LR);
    
    // Configure Long Range specific parameters
    wifi_country_t country = {
        .cc = "AU",           // Country code for maximum power
        .schan = 1,
        .nchan = 13,
        .max_tx_power = 20,   // Maximum allowed power
        .policy = WIFI_COUNTRY_POLICY_MANUAL
    };
    esp_wifi_set_country(&country);
    
    longRangeModeActive = true;
    espnowLog("ESP32 Long Range mode enabled successfully", 3);
    
    return true;
}

bool disableLongRangeMode() {
    espnowLog("Disabling ESP32 Long Range mode...", 3);
    
    // Set WiFi back to normal mode
    esp_err_t result = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    if (result != ESP_OK) {
        espnowLog("Failed to disable LR mode: " + String(result), 1);
        return false;
    }
    
    esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    
    longRangeModeActive = false;
    espnowLog("ESP32 Long Range mode disabled", 3);
    
    return true;
}

bool isLongRangeModeEnabled() {
    return longRangeModeActive;
}

String getCurrentPhyRate() {
    uint8_t protocol_bitmap;
    esp_err_t result = esp_wifi_get_protocol(WIFI_IF_STA, &protocol_bitmap);
    
    if (result != ESP_OK) {
        return "Unknown";
    }
    
    if (protocol_bitmap & WIFI_PROTOCOL_LR) {
        return "Long Range (LR)";
    } else if (protocol_bitmap & WIFI_PROTOCOL_11N) {
        return "802.11n";
    } else if (protocol_bitmap & WIFI_PROTOCOL_11G) {
        return "802.11g";
    } else if (protocol_bitmap & WIFI_PROTOCOL_11B) {
        return "802.11b";
    }
    
    return "Mixed";
}

// ============================================================================
// ESP-NOW CORE FUNCTIONS
// ============================================================================

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        DATA_MGR.incrementMessagesSent();
    }
    // Note: We are not explicitly tracking lost messages anymore,
    // as the send status only indicates if the message was queued, not if it was ACKed.
    // A more robust system would use ACKs to confirm delivery.
}

void onDataReceived(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len) {
    if (!info || !incomingData || len <= 0) return;
    
    int8_t rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;
    
    // The entire system now uses a single, modern message format.
    // We pass all incoming data to the tree message handler.
    bool handled = DATA_MGR.handleIncomingTreeMessage(incomingData, len, info->src_addr, rssi);
    
    if (handled && len >= TREE_MSG_OVERHEAD) {
        const TreeMessageHeader* header = (const TreeMessageHeader*)incomingData;
        
        // Check if we need to forward this message
        bool shouldForwardUp = DATA_MGR.shouldForwardUpstream(header->dest_hid, header->broadcaster_hid);
        bool shouldForwardDown = DATA_MGR.shouldForwardDownstream(header->dest_hid, header->broadcaster_hid);
        
        if (shouldForwardUp) {
            espnowLog("MULTI-HOP: Forwarding message UPSTREAM - Type=" + String(header->msg_type, HEX) + 
                     " From=" + DATA_MGR.formatHID(header->src_hid) + 
                     " To=" + DATA_MGR.formatHID(header->dest_hid) + 
                     " Via=" + DATA_MGR.formatHID(DATA_MGR.getMyHID()), 2);
            // Forwarding doesn't need console messages for button events
            forwardTreeMessage(incomingData, len, true);
        } else if (shouldForwardDown) {
            espnowLog("MULTI-HOP: Forwarding message DOWNSTREAM - Type=" + String(header->msg_type, HEX) + 
                     " From=" + DATA_MGR.formatHID(header->src_hid) + 
                     " To=" + DATA_MGR.formatHID(header->dest_hid) + 
                     " Via=" + DATA_MGR.formatHID(DATA_MGR.getMyHID()), 2);
            // Forwarding doesn't need console messages for button events
            forwardTreeMessage(incomingData, len, false);
        }
    }
    
    // Update status with RSSI info for display
    if (rssi != 0) {
    DATA_MGR.updateStatus("RX: " + String(rssi) + "dBm");
    }
}

bool espnowInit() {
    espnowLog("Initializing ESP-NOW...", 3);
    
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    
    // Start WiFi
    esp_err_t result = esp_wifi_start();
    if (result != ESP_OK) {
        espnowLog("Failed to start WiFi: " + String(result), 1);
        return false;
    }
    
    // Enable Long Range mode if configured
    #if ENABLE_LONG_RANGE_MODE
    if (!enableLongRangeMode()) {
        espnowLog("Warning: Failed to enable Long Range mode", 2);
    } else {
        espnowLog("Long Range mode enabled - extended range available", 3);
    }
    #else
    espnowLog("Long Range mode disabled - using standard range", 3);
    #endif
    
    // Initialize ESP-NOW
    result = esp_now_init();
    if (result != ESP_OK) {
        espnowLog("ESP-NOW init failed: " + String(result), 1);
        return false;
    }
    
    // Register callbacks
    esp_now_register_send_cb(onDataSent);
    esp_now_register_recv_cb(onDataReceived);
    
    // Add broadcast peer for general communication
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    result = esp_now_add_peer(&peerInfo);
    if (result != ESP_OK) {
        espnowLog("Failed to add broadcast peer: " + String(result), 2);
    }
    
    String mode = isLongRangeModeEnabled() ? "Long Range" : "Standard";
    espnowLog("ESP-NOW initialized successfully in " + mode + " mode", 3);
    espnowLog("Current PHY Rate: " + getCurrentPhyRate(), 3);
    
    return true;
}

void espnowSendData(const uint8_t* peerAddr, const uint8_t* data, size_t len){
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peerAddr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    
    esp_err_t result = esp_now_send(peerAddr, data, len);
    if(result == ESP_OK){
        espnowLog("Data queued for transmission", 3);
    } else {
        espnowLog("Failed to queue data: " + String(result), 2);
    }
}

// ============================================================================
// TREE NETWORK FUNCTIONS
// ============================================================================

bool sendDataReportToParent() {
    if (!DATA_MGR.isHIDConfigured()) {
        espnowLog("HID not configured - cannot send data report", 2);
        return false;
    }
    
    if (DATA_MGR.isRoot()) {
        espnowLog("Root node cannot send data report to parent", 2);
        return false;
    }
    
    // Data reports should always be addressed to the root (HID 1)
    // Intermediate nodes will automatically forward them based on routing logic
    uint16_t rootHID = ROOT_HID;
    const DeviceSpecificData& myData = DATA_MGR.getMyDeviceData();
    
    uint8_t buffer[TREE_MSG_OVERHEAD + sizeof(DeviceSpecificData)];
    
    if (!DATA_MGR.createTreeMessage(buffer, sizeof(buffer), rootHID, 
                                   MSG_DEVICE_DATA_REPORT, 
                                   (const uint8_t*)&myData, sizeof(DeviceSpecificData))) {
        espnowLog("Failed to create data report message", 2);
        return false;
    }
    
    espnowSendData(broadcastMAC, buffer, sizeof(buffer));
    return true;
}

bool sendTreeCommand(uint16_t targetHID, TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen) {
    if (!DATA_MGR.isHIDConfigured()) {
        espnowLog("Cannot send command, HID not configured", 2);
        return false;
    }
    
    size_t msgSize = TREE_MSG_OVERHEAD + payloadLen;
    uint8_t buffer[msgSize];
    
    if (!DATA_MGR.createTreeMessage(buffer, sizeof(buffer), targetHID, cmdType, payload, payloadLen)) {
        espnowLog("Failed to create command message", 2);
        return false;
    }
    
    espnowSendData(broadcastMAC, buffer, sizeof(buffer));
        return true;
}

bool sendAcknowledgement(uint16_t targetHID, uint8_t ackedSeqNum, bool isNack, uint8_t reasonCode) {
    TreeMessageType msgType = isNack ? MSG_NACK : MSG_ACKNOWLEDGEMENT;
    
    uint8_t payload[2];
    payload[0] = ackedSeqNum;
    if (isNack) {
    payload[1] = reasonCode;
    }
    size_t payloadLen = isNack ? 2 : 1;
    
    size_t msgSize = TREE_MSG_OVERHEAD + payloadLen;
    uint8_t buffer[msgSize];
    
    if (!DATA_MGR.createTreeMessage(buffer, sizeof(buffer), targetHID, msgType, payload, payloadLen)) {
        espnowLog("Failed to create acknowledgement message", 2);
        return false;
    }
    
    espnowSendData(broadcastMAC, buffer, sizeof(buffer));
        return true;
}

bool forwardTreeMessage(const uint8_t* originalData, int len, bool isUpstream) {
    if (len > 250) {
        espnowLog("Cannot forward message, too large", 1);
        return false;
    }
    
    uint8_t buffer[len];
    memcpy(buffer, originalData, len);
    
    TreeMessageHeader* header = (TreeMessageHeader*)buffer;
        header->broadcaster_hid = DATA_MGR.getMyHID();
    
    // Recalculate CRC since broadcaster_hid has changed
    size_t payloadLen = len - TREE_MSG_OVERHEAD;
    uint8_t newCRC = DATA_MGR.calculateCRC8(buffer + 1, TREE_MSG_HEADER_SIZE - 1 + payloadLen);
    buffer[len - 2] = newCRC;
    
    espnowSendData(broadcastMAC, buffer, len);
        DATA_MGR.incrementMessagesForwarded();
    
        return true;
}

// ============================================================================
// LEGACY BROADCAST TEST FUNCTION
// ============================================================================

void espnowSendBroadcastTest(){
    espnowLog("Sending legacy broadcast test...", 3);
    
    // Simple test payload without legacy header
    struct TestBroadcastMessage {
        uint32_t deviceHID;
        uint32_t timestamp;
        char testData[16];
    } __attribute__((packed));
    
    TestBroadcastMessage msg;
    msg.deviceHID = DATA_MGR.getMyHID();
    msg.timestamp = millis();
    strncpy(msg.testData, "TEST_DATA", sizeof(msg.testData));
    
    esp_err_t result = esp_now_send(broadcastMAC, (uint8_t*)&msg, sizeof(msg));
    if(result == ESP_OK){
        espnowLog("Legacy broadcast test sent", 3);
        DATA_MGR.incrementMessagesSent();
    } else {
        espnowLog("Failed to send legacy broadcast test: " + String(result), 2);
    }
}
