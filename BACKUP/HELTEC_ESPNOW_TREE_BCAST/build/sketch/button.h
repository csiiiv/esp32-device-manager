#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/button.h"
#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

/**
 * @brief Enumeration for button events
 */
enum ButtonEvent {
    BUTTON_NONE,
    BUTTON_SHORT_PRESS,
    BUTTON_LONG_PRESS,
    BUTTON_DOUBLE_CLICK
};

/**
 * @brief Setup the button hardware with pull-up, etc.
 */
void setupButton(uint8_t pin);

/**
 * @brief Check the button each loop; returns the event if triggered.
 */
ButtonEvent checkButton(bool ignoreDoubleClick = false);

/**
 * @brief Print button statistics for debugging
 */
void printButtonStats();

/**
 * @brief Get current button state (HIGH/LOW)
 */
bool getCurrentButtonState();

/**
 * @brief Check if button is currently pressed
 */
bool isButtonPressed();

#endif // BUTTON_H
