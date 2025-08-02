#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/button.cpp"
#include "button.h"
#include "debug.h"

#define MODULE_TITLE       "BTN"
#define MODULE_DEBUG_LEVEL 1
#define btnLog(msg,lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

static uint8_t buttonPin;
static bool lastState = HIGH; 
static unsigned long pressStart = 0;
static bool handled = false;
static const unsigned long LONG_PRESS_MS = 1200; // Reduced from 1800ms to 1200ms for faster long press detection

// Double-click detection
static unsigned long lastReleaseTime = 0;
static const unsigned long DOUBLE_CLICK_WINDOW_MS = 600; // Reduced from 900ms to 600ms for faster double-click detection

// Debug counters
static unsigned long buttonPressCount = 0;
static unsigned long buttonReleaseCount = 0;
static unsigned long shortPressCount = 0;
static unsigned long longPressCount = 0;
static unsigned long doubleClickCount = 0;

void setupButton(uint8_t pin){
    buttonPin = pin;
    pinMode(buttonPin, INPUT_PULLUP);
    btnLog("Button setup complete on pin=" + String(pin) + " (INPUT_PULLUP)", 3);
    
    // Initialize state
    lastState = digitalRead(buttonPin);
    btnLog("Initial button state: " + String(lastState == HIGH ? "HIGH" : "LOW"), 3);
}

ButtonEvent checkButton(bool ignoreDoubleClick) {
    ButtonEvent ev = BUTTON_NONE;
    bool currentState = digitalRead(buttonPin);
    
    // Debug: Log state changes (but limit frequency to avoid spam)
    static unsigned long lastStateLog = 0;
    if (currentState != lastState && (millis() - lastStateLog > 50)) {
        btnLog("State change: " + String(lastState == HIGH ? "HIGH" : "LOW") + 
               " -> " + String(currentState == HIGH ? "HIGH" : "LOW"), 4);
        lastStateLog = millis();
    }
    
    // Falling edge => button pressed
    if (currentState == LOW && lastState == HIGH) {
        pressStart = millis();
        handled = false;
        buttonPressCount++;
        btnLog("=== BUTTON PRESSED ===", 3);
        btnLog("Press #" + String(buttonPressCount) + " at " + String(pressStart), 3);
        btnLog("Ignore double-click: " + String(ignoreDoubleClick ? "YES" : "NO"), 4);
    }
    
    // While button is pressed - check for long press
    if (currentState == LOW && !handled) {
        unsigned long pressDuration = millis() - pressStart;
        
        // Debug: Log long press progress (every 500ms)
        static unsigned long lastLongPressLog = 0;
        if (pressDuration > 500 && (millis() - lastLongPressLog > 500)) {
            btnLog("Long press progress: " + String(pressDuration) + "ms / " + String(LONG_PRESS_MS) + "ms", 4);
            lastLongPressLog = millis();
        }
        
        if (pressDuration >= LONG_PRESS_MS) {
            ev = BUTTON_LONG_PRESS;
            handled = true;
            longPressCount++;
            btnLog("=== LONG PRESS DETECTED ===", 3);
            btnLog("Long press #" + String(longPressCount) + " after " + String(pressDuration) + "ms", 3);
        }
    }
    
    // Rising edge => button released
    if (currentState == HIGH && lastState == LOW) {
        unsigned long pressDuration = millis() - pressStart;
        unsigned long now = millis();
        buttonReleaseCount++;
        
        btnLog("=== BUTTON RELEASED ===", 3);
        btnLog("Release #" + String(buttonReleaseCount) + " after " + String(pressDuration) + "ms", 3);
        btnLog("Handled: " + String(handled ? "YES" : "NO"), 4);
        
        if (pressDuration < LONG_PRESS_MS && !handled) {
            btnLog("Processing short press...", 4);
            
            if (ignoreDoubleClick) {
                ev = BUTTON_SHORT_PRESS;
                shortPressCount++;
                btnLog("=== SHORT PRESS DETECTED (no double-click logic) ===", 3);
                btnLog("Short press #" + String(shortPressCount), 3);
            } else {
                // Check for double-click
                if (lastReleaseTime > 0) {
                    unsigned long timeSinceLastRelease = now - lastReleaseTime;
                    btnLog("Time since last release: " + String(timeSinceLastRelease) + "ms", 4);
                    btnLog("Double-click window: " + String(DOUBLE_CLICK_WINDOW_MS) + "ms", 4);
                    
                    if (timeSinceLastRelease <= DOUBLE_CLICK_WINDOW_MS) {
                        ev = BUTTON_DOUBLE_CLICK;
                        doubleClickCount++;
                        lastReleaseTime = 0; // Reset to prevent triple-click
                        btnLog("=== DOUBLE-CLICK DETECTED ===", 3);
                        btnLog("Double-click #" + String(doubleClickCount), 3);
                    } else {
                        ev = BUTTON_SHORT_PRESS;
                        shortPressCount++;
                        lastReleaseTime = now; // Store time for potential double-click
                        btnLog("=== SHORT PRESS DETECTED ===", 3);
                        btnLog("Short press #" + String(shortPressCount), 3);
                    }
                } else {
                    ev = BUTTON_SHORT_PRESS;
                    shortPressCount++;
                    lastReleaseTime = now; // Store time for potential double-click
                    btnLog("=== SHORT PRESS DETECTED (first press) ===", 3);
                    btnLog("Short press #" + String(shortPressCount), 3);
                }
            }
            handled = true;
        } else if (handled) {
            btnLog("Button already handled (long press or other event)", 4);
        } else {
            btnLog("Press duration too long for short press: " + String(pressDuration) + "ms", 4);
        }
    }
    
    // Update last state
    lastState = currentState;
    
    // Debug: Log event if any
    if (ev != BUTTON_NONE) {
        String eventName;
        switch (ev) {
            case BUTTON_SHORT_PRESS: eventName = "SHORT_PRESS"; break;
            case BUTTON_LONG_PRESS: eventName = "LONG_PRESS"; break;
            case BUTTON_DOUBLE_CLICK: eventName = "DOUBLE_CLICK"; break;
            default: eventName = "UNKNOWN"; break;
        }
        btnLog("Returning event: " + eventName, 3);
    }
    
    return ev;
}

// Debug function to print button statistics
void printButtonStats() {
    btnLog("=== BUTTON STATISTICS ===", 3);
    btnLog("Presses: " + String(buttonPressCount), 3);
    btnLog("Releases: " + String(buttonReleaseCount), 3);
    btnLog("Short presses: " + String(shortPressCount), 3);
    btnLog("Long presses: " + String(longPressCount), 3);
    btnLog("Double-clicks: " + String(doubleClickCount), 3);
    btnLog("Current state: " + String(lastState == HIGH ? "HIGH" : "LOW"), 3);
    btnLog("Handled: " + String(handled ? "YES" : "NO"), 3);
}

// Function to get current button state for immediate checking
bool getCurrentButtonState() {
    return digitalRead(buttonPin);
}

// Function to check if button is currently pressed (for immediate response)
bool isButtonPressed() {
    return getCurrentButtonState() == LOW; // LOW = pressed (pull-up configuration)
}
