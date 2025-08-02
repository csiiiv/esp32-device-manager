#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/TreeNetwork.h"
#ifndef TREENETWORK_H
#define TREENETWORK_H

#include <Arduino.h>
#include "DataManager.h"

// ============================================================================
// TREE NETWORK CLASS
// ============================================================================

/**
 * @brief Tree Network management class
 * Provides high-level interface for tree network operations
 */
class TreeNetwork {
public:
    // Singleton pattern
    static TreeNetwork& getInstance();
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    void initialize();
    
    // ========================================================================
    // HID MANAGEMENT
    // ========================================================================
    bool configureHID(uint16_t hid);
    void setManualHID(uint32_t hid);
    void clearHIDConfiguration();
    uint16_t getMyHID() const;
    uint16_t getParentHID() const;
    bool isRoot() const;
    bool isRootDevice() const { return isRoot(); } // Alias for compatibility
    bool isHIDConfigured() const;
    String getHIDStatus() const;
    uint8_t getTreeDepth() const; // Get current tree depth
    uint8_t getChildCount() const; // Get number of child devices
    
    // ========================================================================
    // DATA REPORTING
    // ========================================================================
    bool sendDataReport();
    bool sendDataReportWithCurrentSensorData();
    void enableAutoReporting(bool enable);
    bool isAutoReportingEnabled() const;
    void processAutoReporting(); // Call in main loop
    
    // ========================================================================
    // COMMAND OPERATIONS
    // ========================================================================
    bool sendTestCommand();
    bool sendCommandToDevice(uint16_t targetHID, TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen);
    bool sendSetOutputsCommand(uint16_t targetHID, uint8_t outputStates);
    bool sendSetIntegersCommand(uint16_t targetHID, uint16_t val1, uint16_t val2);
    bool sendGetAllDataCommand(uint16_t targetHID);
    
    // ========================================================================
    // STATISTICS AND MONITORING
    // ========================================================================
    String getTreeNetworkStats() const;
    void resetTreeStats();
    void updateDeviceSensorData();
    
    // ========================================================================
    // ROOT NODE DATA AGGREGATION
    // ========================================================================
    void showAggregatedDevices() const;
    void clearAggregatedData();
    uint8_t getAggregatedDeviceCount() const;
    
    // ========================================================================
    // DEMO AND TESTING
    // ========================================================================
    void cycleDemoHID(); // For testing - cycles through demo HIDs
    
    // ========================================================================
    // MESSAGE SENDING
    // ========================================================================
    bool sendTreeCommand(uint16_t destHID, TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen);
    bool sendBroadcastTreeCommand(TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen);
    
private:
    TreeNetwork();
    ~TreeNetwork() = default;
    TreeNetwork(const TreeNetwork&) = delete;
    TreeNetwork& operator=(const TreeNetwork&) = delete;
    
    // Private members
    bool autoReportingEnabled;
    unsigned long lastAutoReportTime;
    static const unsigned long AUTO_REPORT_INTERVAL = 5000; // 5 seconds
    
    // Demo data for testing
    static const uint16_t DEMO_HIDS[];
    static const int DEMO_HIDS_COUNT;
    int currentDemoHIDIndex;
    
    // Helper methods
    void logTreeOperation(const String& operation, bool success, const String& details = "") const;
    DeviceSpecificData getCurrentSensorData() const;
};

// Global access macro
#define TREE_NET TreeNetwork::getInstance()

#endif // TREENETWORK_H 