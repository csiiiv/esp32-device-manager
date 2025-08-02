#include "oled.h"
#include "button.h"
#include "espnow_wrapper.h"
#include "debug.h"
#include "helper.h"
#include "DataManager.h"
#include "TreeNetwork.h"
#include "MenuSystem.h"
#include "IoDevice.h"
#include "SerialCommandHandler.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Display timing
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200;  // Increased from 100ms to 200ms to reduce blocking

// Continuous broadcast mode
bool continuousBroadcastEnabled = false;
unsigned long lastBroadcastTime = 0;
const unsigned long BROADCAST_INTERVAL = 2000;  // 2 seconds between broadcasts

// Menu display update flag
bool menuNeedsUpdate = false; // Flag to track if menu needs update due to user input

// ============================================================================
// SETUP FUNCTION
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait up to 3 seconds for Serial
    
    Serial.println("\n=== ESP-NOW Tree Network Starting ===");
    
    // Initialize core systems with small delays to prevent stack buildup
    DATA_MGR.initialize();
    delay(100);
    
    TREE_NET.initialize();
    delay(100);
    
    Serial.println("About to initialize IoDevice...");
    IO_DEVICE.initialize();  // Re-enable with debugging
    Serial.println("IoDevice initialization complete");
    delay(100);
    
    // Initialize display system
    #if ENABLE_OLED
    setupDisplay();
    Serial.println("OLED display initialized");
    #else
    Serial.println("OLED display disabled - using Serial output");
    #endif
    delay(100);
    
    // Initialize ESP-NOW
    if (espnowInit()) {
        Serial.println("ESP-NOW initialized successfully");
    } else {
        Serial.println("ESP-NOW initialization failed!");
    }
    delay(100);
    
    // Initialize menu system
    MENU_SYS.initialize();
    delay(100);
    
    // Initialize serial command handler
    SERIAL_CMD.initialize();
    delay(100);
    
    // Initialize button
    setupButton(BUTTON_PIN);
    
    Serial.println("=== Setup Complete ===\n");
    DATA_MGR.updateStatus("System Ready");
    
    // Print initial button statistics
    printButtonDebugStats();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    unsigned long loopStart = millis();
    
    // IMMEDIATE BUTTON STATE CHECK (highest priority)
    static bool lastImmediateButtonState = HIGH;
    bool currentImmediateButtonState = getCurrentButtonState();
    
    // Log immediate state changes for debugging
    if (currentImmediateButtonState != lastImmediateButtonState) {
        Serial.println("IMMEDIATE: Button state changed to " + String(currentImmediateButtonState == HIGH ? "HIGH" : "LOW"));
        lastImmediateButtonState = currentImmediateButtonState;
    }
    
    // PRIORITY 1: Button input handling (highest priority)
    handleButtonInput();
    
    // PRIORITY 2: Serial command handling (high priority)
    SERIAL_CMD.update();
    
    // PRIORITY 3: Core system updates (medium priority)
    DATA_MGR.update();
    TREE_NET.processAutoReporting();
    
    // PRIORITY 4: I/O operations (lower priority, but still important)
    if (millis() > 1000) {
        IO_DEVICE.scanInputs();           // Scan for input changes
        IO_DEVICE.checkAndSendReport();   // Handle auto-reporting based on I/O changes
    }
    
    // PRIORITY 5: Network operations (lowest priority)
    if (continuousBroadcastEnabled && millis() - lastBroadcastTime >= BROADCAST_INTERVAL) {
        lastBroadcastTime = millis();
    }
    
    // PRIORITY 6: Display updates (lowest priority, can be delayed)
    // On-change strategy: Menu updates only on user input, status/console on regular intervals
    
    if (MENU_SYS.isInMenuMode()) {
        // Menu mode: Only update on user input (not on timer)
        if (menuNeedsUpdate) {
            MENU_SYS.updateDisplay();
            menuNeedsUpdate = false; // Reset flag after update
        }
    } else {
        // Status/Console mode: Update on regular intervals
        if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
            // Skip display updates if loop is taking too long
            MENU_SYS.updateDisplay();
            lastDisplayUpdate = millis();
        }
    }
    
    // PRIORITY 7: Debug operations (lowest priority)
    static unsigned long lastButtonStatsTime = 0;
    if (millis() - lastButtonStatsTime >= 10000) {
        printButtonDebugStats();
        lastButtonStatsTime = millis();
    }
}

// ============================================================================
// DISPLAY UPDATE TRIGGERS
// ============================================================================

// Function to trigger menu display update (called from button handling)
void triggerMenuDisplayUpdate() {
    menuNeedsUpdate = true;
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButtonInput() {
    static unsigned long lastButtonEvent = 0;
    static const unsigned long BUTTON_DEBOUNCE_MS = 50; // Reduced from 100ms to 50ms for faster response
    static unsigned long eventCount = 0;
    
    bool inMenu = MENU_SYS.isInMenuMode();
    ButtonEvent btnEvent = checkButton(inMenu); // Pass true to ignore double-clicks in menu
    
    if (btnEvent != BUTTON_NONE) {
        eventCount++;
        unsigned long now = millis();
        
        Serial.println("=== BUTTON EVENT RECEIVED ===");
        Serial.println("Event #" + String(eventCount) + " at " + String(now));
        Serial.println("In menu mode: " + String(inMenu ? "YES" : "NO"));
        
        // Debounce button events (reduced time for faster response)
        if (now - lastButtonEvent < BUTTON_DEBOUNCE_MS) {
            Serial.println("EVENT DEBOUNCED - too soon after last event");
            Serial.println("Time since last: " + String(now - lastButtonEvent) + "ms");
            Serial.println("Debounce window: " + String(BUTTON_DEBOUNCE_MS) + "ms");
            return; // Ignore rapid button events
        }
        
        lastButtonEvent = now;
        Serial.println("Event accepted - processing...");
        
        // Convert event to string for logging
        String eventName;
        switch (btnEvent) {
            case BUTTON_SHORT_PRESS: eventName = "SHORT_PRESS"; break;
            case BUTTON_LONG_PRESS: eventName = "LONG_PRESS"; break;
            case BUTTON_DOUBLE_CLICK: eventName = "DOUBLE_CLICK"; break;
            default: eventName = "UNKNOWN"; break;
        }
        
        Serial.println("Processing event: " + eventName);
        
        switch (btnEvent) {
            case BUTTON_SHORT_PRESS:
                Serial.println("=== SHORT PRESS HANDLING ===");
                if (MENU_SYS.isInMenuMode()) {
                    Serial.println("In menu mode - calling navigateDown()");
                    MENU_SYS.navigateDown();
                    triggerMenuDisplayUpdate(); // Trigger menu display update
                    Serial.println("Menu navigate down - COMPLETED");
                } else if (MENU_SYS.isInConsoleMode()) {
                    Serial.println("In console mode - short press to exit console");
                    MENU_SYS.setConsoleMode(false);
                    MENU_SYS.setDisplayMode(false); // Ensure we're in status mode
                    DATA_MGR.updateStatus("Status Mode");
                    Serial.println("Exited console mode - COMPLETED");
                } else {
                    Serial.println("Not in menu mode - short press ignored for menu navigation");
                    Serial.println("(Physical pin change will be picked up by IO_DEVICE.scanInputs())");
                }
                break;
                
            case BUTTON_LONG_PRESS:
                Serial.println("=== LONG PRESS HANDLING ===");
                if (MENU_SYS.isInMenuMode()) {
                    Serial.println("In menu mode - calling selectCurrentItem()");
                    MENU_SYS.selectCurrentItem();
                    triggerMenuDisplayUpdate(); // Trigger menu display update
                    Serial.println("Menu item selected - COMPLETED");
                } else if (MENU_SYS.isInConsoleMode()) {
                    Serial.println("In console mode - long press to clear console");
                    MENU_SYS.clearConsoleMessages();
                    DATA_MGR.updateStatus("Console cleared");
                    Serial.println("Console cleared - COMPLETED");
                } else {
                    Serial.println("Not in menu mode - long press for status update");
                    DATA_MGR.updateStatus("Long press detected");
                    Serial.println("Status mode long press - COMPLETED");
                }
                break;
                
            case BUTTON_DOUBLE_CLICK:
                Serial.println("=== DOUBLE CLICK HANDLING ===");
                if (MENU_SYS.isInMenuMode()) {
                    Serial.println("In menu mode - double-click IGNORED (prevents accidental menu exit)");
                } else if (MENU_SYS.isInConsoleMode()) {
                    Serial.println("In console mode - double-click to enter menu mode");
                    MENU_SYS.setConsoleMode(false);
                    MENU_SYS.setDisplayMode(true);
                    triggerMenuDisplayUpdate(); // Trigger menu display update when entering menu
                    DATA_MGR.updateStatus("Menu Mode");
                    Serial.println("Entered menu mode from console - COMPLETED");
                } else {
                    Serial.println("Not in menu mode - entering menu mode");
                    MENU_SYS.setDisplayMode(true);
                    triggerMenuDisplayUpdate(); // Trigger menu display update when entering menu
                    DATA_MGR.updateStatus("Menu Mode");
                    Serial.println("Entered menu mode - COMPLETED");
                }
                break;
                
            default:
                Serial.println("Unknown button event - no action taken");
                break;
        }
        
        Serial.println("=== BUTTON EVENT PROCESSING COMPLETE ===");
    }
}

// ============================================================================
// CONTINUOUS BROADCAST CONTROL
// ============================================================================

void setContinuousBroadcast(bool enabled) {
    continuousBroadcastEnabled = enabled;
    if (enabled) {
        lastBroadcastTime = millis();
    }
}

bool isContinuousBroadcastEnabled() {
    return continuousBroadcastEnabled;
}

// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================

void printButtonDebugStats() {
    Serial.println("=== BUTTON DEBUG STATISTICS ===");
    printButtonStats(); // This calls the function from button.cpp
    Serial.println("===============================");
}
