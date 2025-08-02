#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <Arduino.h>
#include <WiFi.h>

// Forward declaration
class Preferences;

// ============================================================================
// TREE NETWORK CONFIGURATION
// ============================================================================

#define ROOT_HID 1
#define UNCONFIGURED_HID 0
#define MAX_HID_VALUE 999
#define BROADCAST_HID 0xFFFF

// Distributed I/O Configuration 
#define MAX_DISTRIBUTED_IO_BITS 32
#define MAX_INPUTS 3
#define SHARED_DATA_WORDS 1
#define BITS_PER_WORD 32

// Maximum number of devices the root node can track
#define MAX_AGGREGATED_DEVICES 64

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * @brief Distributed I/O data structure - 3 inputs x 32 bits each
 * Structure: sharedData[inputIndex][bitIndex]
 * - sharedData[0][bitIndex] = Input 1 status for each device
 * - sharedData[1][bitIndex] = Input 2 status for each device  
 * - sharedData[2][bitIndex] = Input 3 status for each device
 */
typedef struct {
    uint32_t sharedData[MAX_INPUTS][MAX_DISTRIBUTED_IO_BITS / 32]; // 3 inputs x 1 word each
} __attribute__((packed)) DistributedIOData;

/**
 * @brief Device-specific data payload
 */
typedef struct {
    uint8_t  input_states;
    uint8_t  output_states;
    uint16_t memory_states;
    uint16_t analog_values[2];
    uint16_t integer_values[2];
    uint8_t  bit_index;
    uint8_t  reserved;
} __attribute__((packed)) DeviceSpecificData;

/**
 * @brief Message types for tree network communication
 */
enum TreeMessageType : uint8_t {
    MSG_DEVICE_DATA_REPORT    = 0x01,
    MSG_DISTRIBUTED_IO_UPDATE = 0x22,
    // The following message types are still defined but not fully implemented
    // in the current simplified protocol.
    MSG_ACKNOWLEDGEMENT       = 0x02,
    MSG_NACK                  = 0x03,
    MSG_COMMAND_SET_OUTPUTS   = 0x10,
    
    // Bit assignment protocol messages
    MSG_REQUEST_BIT_INDEX     = 0x30,    // Device requests bit assignment
    MSG_ASSIGN_BIT_INDEX      = 0x31,    // Root assigns bit index
    MSG_CONFIRM_BIT_INDEX     = 0x32,    // Device confirms assignment
};

/**
 * @brief Tree network message frame structure
 */
typedef struct {
    uint8_t  soh;
    uint8_t  frame_len;
    uint16_t dest_hid;
    uint16_t src_hid;
    uint16_t broadcaster_hid;
    uint8_t  msg_type;
    uint8_t  seq_num;
} __attribute__((packed)) TreeMessageHeader;

#define TREE_MSG_SOH 0xAA
#define TREE_MSG_EOT 0x55
#define TREE_MSG_HEADER_SIZE 10
#define TREE_MSG_OVERHEAD 12

/**
 * @brief Bit assignment protocol structures
 */
typedef struct {
    uint16_t requesting_hid;
    uint8_t  preferred_bit;    // 0xFF = no preference
} __attribute__((packed)) BitIndexRequest;

typedef struct {
    uint16_t target_hid;
    uint8_t  assigned_bit;
    uint8_t  status;          // 0 = success, 1 = no available bits
} __attribute__((packed)) BitIndexAssignment;

typedef struct {
    uint16_t confirming_hid;
    uint8_t  confirmed_bit;
    uint8_t  status;          // 0 = accepted, 1 = rejected
} __attribute__((packed)) BitIndexConfirmation;

/**
 * @brief Network statistics for display and monitoring
 */
struct NetworkStats {
    uint32_t messagesSent = 0;
    uint32_t messagesReceived = 0;
    uint32_t messagesForwarded = 0;
    uint32_t messagesIgnored = 0;
    uint32_t securityViolations = 0;
    uint32_t lastMessageTime = 0;
    String lastSenderMAC = "None";
    float signalStrength = 0.0f;
};

/**
 * @brief System status information
 */
struct SystemStatus {
    String currentStatus = "Ready";
    String previousStatus = "";
    uint32_t uptime = 0;
    uint16_t myHID = 0;
    bool isRoot = false;
    bool hidConfigured = false;
    uint8_t myBitIndex = 255;
    bool bitIndexConfigured = false;
};

// ============================================================================
// DATAMANAGER CLASS
// ============================================================================

class DataManager {
public:
    static DataManager& getInstance();
    
    // Initialization
    void initialize();
    void getNodeMAC(uint8_t* mac) const;
    
    // HID Management
    bool setMyHID(uint16_t hid);
    bool setHID(uint16_t hid) { return setMyHID(hid); } // Alias for compatibility
    uint16_t getMyHID() const { return systemStatus.myHID; }
    uint16_t getHID() const { return systemStatus.myHID; } // Alias for compatibility
    uint16_t getParentHID() const;
    bool isRoot() const { return systemStatus.isRoot; }
    bool isHIDConfigured() const { return systemStatus.hidConfigured; }
    bool isValidChild(uint16_t childHID) const;
    bool isMyDescendant(uint16_t targetHID) const;
    void clearHIDFromNVM();
    
    // Bit Index Management
    bool setMyBitIndex(uint8_t bitIndex);
    bool setBitIndex(uint8_t bitIndex) { return setMyBitIndex(bitIndex); } // Alias for compatibility
    uint8_t getMyBitIndex() const { return systemStatus.myBitIndex; }
    uint8_t getBitIndex() const { return systemStatus.myBitIndex; } // Alias for compatibility
    bool isBitIndexConfigured() const { return systemStatus.bitIndexConfigured; }
    bool isValidBitIndex(uint8_t bitIndex) const { return bitIndex < MAX_DISTRIBUTED_IO_BITS; }
    void clearBitIndexFromNVM();
    
    // Configuration Status
    bool isDeviceFullyConfigured() const { return systemStatus.hidConfigured && systemStatus.bitIndexConfigured; }
    bool isConfigured() const { return isDeviceFullyConfigured(); } // Alias for compatibility
    void clearAllConfiguration();
    
    // Device Data Management
    void setMyDeviceData(const DeviceSpecificData& data) { myDeviceData = data; }
    const DeviceSpecificData& getMyDeviceData() const { return myDeviceData; }
    DeviceSpecificData getDeviceSpecificData() const { return myDeviceData; } // Alias for compatibility
    bool updateDeviceData(uint16_t srcHID, const DeviceSpecificData& data);
    const DeviceSpecificData* getDeviceData(uint16_t srcHID) const;
    uint8_t getAggregatedDeviceCount() const { return aggregatedDeviceCount; }
    void showAggregatedDevices() const;
    void clearAggregatedData();
    
    // Distributed I/O Control
    void computeAndBroadcastDistributedIO();
    DistributedIOData computeSharedDataFromInputs() const;
    void setDistributedIOSharedData(const DistributedIOData& sharedData);
    DistributedIOData getDistributedIOSharedData() const;
    uint32_t getSharedData() const; // Get shared data as uint32 for compatibility
    void broadcastDistributedIOUpdate(const DistributedIOData& sharedData);
    void forwardDistributedIOUpdateToChildren(const DistributedIOData& sharedData);
    String getDistributedIOStatus() const;
    
    // Multi-input bit manipulation functions
    void setDistributedIOBit(int inputIndex, int bitIndex, bool value);
    bool getDistributedIOBit(int inputIndex, int bitIndex) const;
    bool getMyBitState(int inputIndex) const; // Get the state of this device's assigned bit for specific input
    
    // Backward compatibility functions
    void setDistributedIOBit(int bitIndex, bool value); // Maps to input 0 for compatibility
    bool getDistributedIOBit(int bitIndex) const; // Maps to input 0 for compatibility
    bool getMyBitState() const; // Maps to input 0 for compatibility
    
    // Backward compatibility functions for menu system
    uint16_t getDistributedIOSharedDataAsUint16() const;
    
    // Message Handling
    uint8_t getNextSequenceNumber() { return ++sequenceCounter; }
    bool handleIncomingTreeMessage(const uint8_t* data, int len, const uint8_t* senderMAC, int rssi = 0);
    bool createTreeMessage(uint8_t* buffer, size_t bufferSize, uint16_t destHID, 
                          TreeMessageType msgType, const uint8_t* payload, size_t payloadLen);
    
    // Routing Logic
    bool shouldProcessMessage(uint16_t destHID, uint16_t srcHID) const;
    bool shouldForwardUpstream(uint16_t destHID, uint16_t broadcasterHID) const;
    bool shouldForwardDownstream(uint16_t destHID, uint16_t broadcasterHID) const;
    
    // System Status & Stats
    const NetworkStats& getNetworkStats() const { return networkStats; }
    void resetNetworkStats();
    const SystemStatus& getSystemStatus() const { return systemStatus; }
    void updateStatus(const String& newStatus);
    String getCurrentStatus() const { return systemStatus.currentStatus; }
    void update();
    const uint8_t* getLastSenderMAC() const { return lastSenderMAC; }

    // Formatting Utilities
    String formatMAC(const uint8_t* mac) const;
    String formatHID(uint16_t hid) const;
    String formatDistributedIOData(const DistributedIOData& data) const;

    // Public helpers needed by other modules
    uint8_t calculateCRC8(const uint8_t* data, size_t len);
    void incrementMessagesSent() { networkStats.messagesSent++; }
    void incrementMessagesForwarded() { networkStats.messagesForwarded++; }
    
    // NVM access (public for SerialCommandHandler)
    bool loadHIDFromNVM();
    bool loadBitIndexFromNVM();

private:
    DataManager();
    DataManager(const DataManager&) = delete;
    DataManager& operator=(const DataManager&) = delete;
    
    // NVM access (private methods)
    void saveHIDToNVM();
    void saveBitIndexToNVM();

    // Internal data members
    uint8_t nodeMac[6];
    uint8_t sequenceCounter;
    
    SystemStatus systemStatus;
    NetworkStats networkStats;
    
    DeviceSpecificData myDeviceData;
    DistributedIOData distributedIOData;
    
    // Root node data aggregation
    DeviceSpecificData globalDataArray[MAX_AGGREGATED_DEVICES];
    uint16_t deviceHIDArray[MAX_AGGREGATED_DEVICES];
    uint32_t deviceLastSeen[MAX_AGGREGATED_DEVICES];
    uint8_t aggregatedDeviceCount;
    int findDeviceIndex(uint16_t srcHID) const;
    
    Preferences* preferences;
    
    // Message processing functions
    bool validateTreeMessage(const uint8_t* data, int len);
    bool isValidParentChild(uint16_t parentHID, uint16_t childHID) const;
    void processDataReport(const TreeMessageHeader* header, const uint8_t* payload, size_t payloadLen, const uint8_t* sender);
    void processCommand(const TreeMessageHeader* header, const uint8_t* payload, size_t payloadLen, const uint8_t* sender);
    void processAcknowledgement(const TreeMessageHeader* header, const uint8_t* payload, size_t payloadLen, const uint8_t* sender);
    void processDistributedIOUpdate(const TreeMessageHeader* header, const uint8_t* payload, size_t payloadLen, const uint8_t* sender);

    // Stat tracking
    void incrementMessagesReceived() { networkStats.messagesReceived++; }
    void incrementMessagesIgnored() { networkStats.messagesIgnored++; }
    void incrementSecurityViolations() { networkStats.securityViolations++; }
    void updateLastSender(const uint8_t* senderMAC);
    void updateSignalStrength(float rssi) { networkStats.signalStrength = rssi; }
    
    // Last sender MAC address storage
    uint8_t lastSenderMAC[6];
};

#define DATA_MGR DataManager::getInstance()

#endif // DATAMANAGER_H 