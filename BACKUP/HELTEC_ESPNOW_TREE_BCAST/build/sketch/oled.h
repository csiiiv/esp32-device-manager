#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/oled.h"
#ifndef OLED_H
#define OLED_H

// ============================================================================
// OLED ENABLE/DISABLE CONFIGURATION
// ============================================================================
// Comment out the next line to disable all OLED functionality
#define ENABLE_OLED 1  // Re-enabled with on-change update strategy

#if ENABLE_OLED
#include <U8g2lib.h>
#endif

#include "debug.h"

// Heltec V3 OLED Pin Definitions (from heltec.h)
#define SDA_OLED     GPIO_NUM_17   // Changed from GPIO_NUM_8
#define SCL_OLED     GPIO_NUM_18   // Changed from GPIO_NUM_9
#define RST_OLED     GPIO_NUM_21   // Changed from GPIO_NUM_12
#define VEXT_PIN     GPIO_NUM_36   // Changed from GPIO_NUM_10

// Button pin - using GPIO_0 (boot/prog button)
#define BUTTON_PIN   GPIO_NUM_0    // Keep as GPIO_0 for boot/prog button

#if ENABLE_OLED
/**
 * @brief The main U8G2 display object for a 128x64 SSD1306.
 *        Adjust the constructor as needed for your hardware/board.
 */
extern U8G2_SSD1306_128X64_NONAME_F_SW_I2C display;
#endif

/**
 * @brief Initialize the display (powering on, setting fonts, etc.).
 *        If OLED is disabled, this becomes a no-op.
 */
void setupDisplay();

/**
 * @brief Example functions to toggle external power if your board has it.
 *        If OLED is disabled, these become no-ops.
 */
void VextON();
void VextOFF();

/**
 * @brief Check if OLED functionality is enabled at compile time.
 */
bool isOLEDEnabled();

#endif
