#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/helper.h"
#ifndef HELPER_H
#define HELPER_H

#include <Arduino.h>

/**
 * @brief Toggle whether global debug messages are enabled at all.
 *        Provided as an example if you want a quick global on/off switch.
 */
void enableGlobalDebug(bool enabled);
bool isGlobalDebugEnabled();

/**
 * @brief Simple console logging function for backward compatibility
 */
void consoleLog(const String &msg);

#endif // HELPER_H 