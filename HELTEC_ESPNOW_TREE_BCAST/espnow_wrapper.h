#ifndef ESPNOW_WRAPPER_H
#define ESPNOW_WRAPPER_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "DataManager.h"

// ============================================================================
// ESP32 LONG RANGE (LR) MODE CONFIGURATION
// ============================================================================

/**
 * @brief Enable ESP32 Long Range mode for extended communication distance
 * 
 * Long Range mode can extend ESP-NOW communication up to 1km+ in ideal conditions
 * - Reduces data rate but increases sensitivity and range
 * - Both sender and receiver must be in LR mode
 * - Available on ESP32 and ESP32-S series
 * 
 * Set to 1 to enable LR mode, 0 to disable
 */
#define ENABLE_LONG_RANGE_MODE 1

// ============================================================================
// ESP-NOW CONFIGURATION
// ============================================================================

// Maximum number of peers
#define MAX_PEERS 20

// Broadcast MAC address for sending to all peers
extern uint8_t broadcastMAC[6];

// ============================================================================
// TREE NETWORK FUNCTIONS
// ============================================================================

/**
 * @brief Send tree network data report to parent
 * @return true if message sent successfully
 */
bool sendDataReportToParent();

/**
 * @brief Send command to specific device via tree routing
 * @param targetHID Target device HID
 * @param cmdType Command type
 * @param payload Command payload
 * @param payloadLen Payload length
 * @return true if message sent successfully
 */
bool sendTreeCommand(uint16_t targetHID, TreeMessageType cmdType, const uint8_t* payload, size_t payloadLen);

/**
 * @brief Send acknowledgement message
 * @param targetHID Target device HID
 * @param ackedSeqNum Sequence number being acknowledged
 * @param isNack true for NACK, false for ACK
 * @param reasonCode Reason code for NACK (ignored for ACK)
 * @return true if message sent successfully
 */
bool sendAcknowledgement(uint16_t targetHID, uint8_t ackedSeqNum, bool isNack = false, uint8_t reasonCode = 0);

/**
 * @brief Forward tree message (for intermediate nodes)
 * @param originalData Original message data
 * @param len Message length
 * @param isUpstream true for upstream forwarding, false for downstream
 * @return true if message forwarded successfully
 */
bool forwardTreeMessage(const uint8_t* originalData, int len, bool isUpstream);

// ============================================================================
// LEGACY ESP-NOW FUNCTIONS
// ============================================================================

/**
 * @brief Convert MAC address to string representation
 * @param mac MAC address array (6 bytes)
 * @return String representation of MAC address
 */
String macToString(const uint8_t* mac);

/**
 * @brief Initialize ESP-NOW with optional Long Range mode
 * @return true if initialization successful, false otherwise
 */
bool espnowInit();

/**
 * @brief Enable ESP32 Long Range mode
 * @return true if successful, false otherwise
 * @note Both sender and receiver must enable LR mode
 */
bool enableLongRangeMode();

/**
 * @brief Disable ESP32 Long Range mode (return to normal mode)
 * @return true if successful, false otherwise
 */
bool disableLongRangeMode();

/**
 * @brief Check if Long Range mode is currently enabled
 * @return true if LR mode is active, false otherwise
 */
bool isLongRangeModeEnabled();

/**
 * @brief Get current WiFi PHY rate for diagnostics
 * @return Current PHY rate as string
 */
String getCurrentPhyRate();

/**
 * @brief Send arbitrary data to a specified peer address.
 */
void espnowSendData(const uint8_t* peerAddr, const uint8_t* data, size_t len);

/**
 * @brief Test function to send a broadcast message with test data.
 */
void espnowSendBroadcastTest();

#endif
