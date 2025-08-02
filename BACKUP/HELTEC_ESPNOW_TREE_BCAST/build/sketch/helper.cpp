#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/helper.cpp"
#include "helper.h"
#include "debug.h"

/**
 * We define a module-specific debug macro. Since "helper" is more general,
 * you can just pick a suitable title and debug level. 
 */
#define MODULE_TITLE       "HELP"
#define MODULE_DEBUG_LEVEL 1
#define helpLog(msg, lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

// Status messages
static String statusMsg1 = "Ready";
static String statusMsg2 = "";

/**
 * @brief By default, debug.h has globalDebugEnabled = true. 
 *        We'll expose toggles via these functions.
 */
extern bool globalDebugEnabled;

void enableGlobalDebug(bool enabled){
    globalDebugEnabled = enabled;
}

bool isGlobalDebugEnabled(){
    return globalDebugEnabled;
}

// Status updates
void updateStatus(const String &newMsg) {
    statusMsg2 = statusMsg1;
    statusMsg1 = newMsg;
    helpLog("Status updated => " + newMsg, 4); // DEBUG
}
String getStatusMessage() {
    return statusMsg1;
}
String getPreviousStatusMessage() {
    return statusMsg2;
}

// consoleLog => simple console output for backward compatibility
void consoleLog(const String &msg) {
    Serial.println(msg);
}

// Build a binary string from a test value (since CommsManager is deprecated)
String getGlobalSyncString() {
    // For testing purposes, return a simple binary pattern
    uint16_t testValue = (millis() / 1000) & 0xFFFF; // Changes every second
    char binBuf[17]; // 16 bits + null terminator
    binBuf[16] = '\0';
    for(int i=0;i<16;i++){
        int bitPos = 15 - i; // for left->right
        binBuf[i] = ((testValue >> bitPos) &1) ? '1':'0';
    }
    return String(binBuf);
} 