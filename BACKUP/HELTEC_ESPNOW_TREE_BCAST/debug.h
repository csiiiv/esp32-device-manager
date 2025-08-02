#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include "DataManager.h" // Include DataManager to access status

/**
 * @brief Global flag controlling if debug prints are enabled at all.
 *        For instance, set this to false (0) in production builds to disable debug entirely.
 */
extern bool globalDebugEnabled;

/**
 * @brief Print a message to Serial with a specified module title and severity level.
 *
 * @param msg            The message text to log
 * @param moduleTitle    A short string identifying the module (e.g. "COMMS", "UI", "SYNC")
 * @param messageLevel   Numeric severity level for the message (0=FATAL,1=ERROR,2=WARN,3=INFO,4=DEBUG,5=TRACE)
 * @param moduleDebugLevel The maximum level this module will display (e.g. 3 means ignore levels above INFO)
 *
 * If messageLevel <= moduleDebugLevel, the message will be printed.
 * If globalDebugEnabled is false, no messages are printed at all.
 *
 * Example usage:
 *   debugPrint("Sync started", "SYNC", 3, 4);
 *   // => "[HID:1 B:0][SYNC][INFO][12345ms]: Sync started"
 *
 */
inline String getLogPrefix() {
    String hidStr = DATA_MGR.isHIDConfigured() ? String(DATA_MGR.getMyHID()) : "---";
    String bitStr = DATA_MGR.isBitIndexConfigured() ? String(DATA_MGR.getMyBitIndex()) : "-";
    return "[HID:" + hidStr + " B:" + bitStr + "]";
}

inline void debugPrint(const String &msg,
                       const String &moduleTitle = "GEN",
                       int messageLevel = 3,
                       int moduleDebugLevel = 3)
{
    if(!globalDebugEnabled || messageLevel > moduleDebugLevel) {
        // Global or module debug disabled => do nothing
        return;
    }

    // Build prefix from moduleTitle + severity label
    String prefix = getLogPrefix() + "[" + moduleTitle + "]";
    switch (messageLevel) {
      case 0: prefix += "[FATAL]"; break;
      case 1: prefix += "[ERROR]"; break;
      case 2: prefix += "[WARN]"; break;
      case 3: prefix += "[INFO]"; break;
      case 4: prefix += "[DEBUG]"; break;
      case 5: prefix += "[TRACE]"; break;
      default: prefix += "[UNK_LVL]"; break;
    }

    // Print with timestamp in ms
    Serial.println(prefix + "[" + String(millis()) + "ms]: " + msg);
}

/**
 * @brief Helper to convert a 4-byte device ID into a 8-digit hex string (e.g. "01020304").
 */
String deviceIDToString(const uint8_t *id);

#endif // DEBUG_H
