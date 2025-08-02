#ifndef SERIAL_COMMAND_HANDLER_H
#define SERIAL_COMMAND_HANDLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "DataManager.h"
#include "IoDevice.h"
#include "TreeNetwork.h"
#include "MenuSystem.h"

// ============================================================================
// SERIAL COMMAND HANDLER
// ============================================================================

class SerialCommandHandler {
private:
    static const int MAX_COMMAND_LENGTH = 512;
    static const int JSON_DOCUMENT_SIZE = 1024;
    
    String commandBuffer;
    bool commandComplete;
    
    // Command types
    enum CommandType {
        CMD_CONFIG_SCHEMA,
        CMD_CONFIG_SAVE,
        CMD_CONFIG_LOAD,
        CMD_RESTART,
        CMD_STATUS,
        CMD_NETWORK_STATUS,
        CMD_NETWORK_STATS,
        CMD_IO_STATUS,
        CMD_DEVICE_DATA,
        CMD_UNKNOWN
    };
    
    CommandType parseCommand(const String& command);
    void sendResponse(const String& response);
    void sendJsonResponse(const JsonDocument& doc);
    
    // Command handlers
    void handleConfigSchema();
    void handleConfigSave(const String& jsonData);
    void handleConfigLoad();
    void handleRestart();
    void handleStatus();
    void handleNetworkStatus();
    void handleNetworkStats();
    void handleIOStatus();
    void handleDeviceData();
    
public:
    SerialCommandHandler();
    void initialize();
    void update();
    void processCommand(const String& command);
};

extern SerialCommandHandler SERIAL_CMD;

#endif // SERIAL_COMMAND_HANDLER_H 