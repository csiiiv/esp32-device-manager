#include "debug.h"

/**
 * @brief Toggle this to false to silence all debug messages globally.
 */
bool globalDebugEnabled = true;

/**
 * @brief Convert a 4-byte device ID into an 8-digit hex string.
 */
String deviceIDToString(const uint8_t *id) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%02X%02X%02X%02X", id[3], id[2], id[1], id[0]);
    return String(buf);
}
