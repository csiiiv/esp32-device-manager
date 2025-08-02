#line 1 "/mnt/6E9A84429A8408B3/_VSC/WEB_UPLOAD/HELTEC_ESPNOW_TREE_BCAST/MenuSystem.cpp"
#include "MenuSystem.h"
#include "DataManager.h"
#include "TreeNetwork.h"
#include "espnow_wrapper.h"
#include "debug.h"
#include "helper.h"
#include "oled.h"
#include "IoDevice.h"

// Logging macros
#define MODULE_TITLE       "MENU_SYS"
#define MODULE_DEBUG_LEVEL 1
#define menuLog(msg, lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

// Forward declarations for main application functions
void setContinuousBroadcast(bool enabled);
bool isContinuousBroadcastEnabled();

// ============================================================================
// HID CONFIG MENU PROVIDER IMPLEMENTATION
// ============================================================================

// Static buffer definitions
char HidConfigMenuProvider::textBuffers[6][20];
char BitIndexConfigMenuProvider::textBuffers[10][25];

HidConfigMenuProvider::HidConfigMenuProvider() : currentHid(1), hidConfigDepth(1) {
    menuLog("HidConfigMenuProvider created.", 4);
}

// ============================================================================
// BIT INDEX CONFIG MENU PROVIDER IMPLEMENTATION
// ============================================================================

BitIndexConfigMenuProvider::BitIndexConfigMenuProvider() : currentPage(0) {
    // Start with page containing current bit index if configured
    if (DATA_MGR.isBitIndexConfigured()) {
        uint8_t currentBit = DATA_MGR.getMyBitIndex();
        currentPage = currentBit / BITS_PER_PAGE;
    }
    menuLog("BitIndexConfigMenuProvider created. Starting page: " + String(currentPage) + 
           " (bits " + String(getPageStartBit()) + "-" + String(getPageEndBit()) + ")", 4);
}

const char* HidConfigMenuProvider::getItemText(int index) {
    int itemCount = getItemCount();
    if (index < 0 || index >= itemCount) return "Invalid";

    // Item 0 is always "Set HID"
    if (index == 0) {
        snprintf(textBuffers[0], sizeof(textBuffers[0]), "Set HID: %lu", currentHid);
        return textBuffers[0];
    }

    // "Go Up" is item 1 if not at root
    if (hidConfigDepth > 1 && index == 1) {
        return "Go Up";
    }

    // "Back" is always the last item
    if (index == itemCount - 1) {
        return "Back";
    }
    
    // Child nodes
    int childIndex = index - (hidConfigDepth > 1 ? 2 : 1);
    snprintf(textBuffers[childIndex + 1], sizeof(textBuffers[childIndex + 1]), "Child: %lu%d", currentHid, childIndex + 1);
    return textBuffers[childIndex + 1];
}

void HidConfigMenuProvider::selectItem(int index) {
    int itemCount = getItemCount();
    if (index < 0 || index >= itemCount) return;

    // "Set HID"
    if (index == 0) {
        TREE_NET.setManualHID(currentHid);
        DATA_MGR.updateStatus("HID Set: " + String(currentHid));
        menuLog("HID configured to " + String(currentHid), 3);
        
        // Check if we're in device configuration mode
        if (MENU_SYS.isInDeviceConfigMode()) {
            // Proceed to bit index configuration
            MENU_SYS.proceedToBitIndexConfig();
        } else {
            // Normal exit
            MENU_SYS.exitHidConfigMode();
        }
        return;
    }

    // "Go Up"
    if (hidConfigDepth > 1 && index == 1) {
        currentHid /= 10;
        hidConfigDepth--;
        menuLog("HID config: up to " + String(currentHid), 4);
        return;
    }

    // "Back"
    if (index == itemCount - 1) {
        back();
        return;
    }

    // Child nodes
    int childIndex = index - (hidConfigDepth > 1 ? 2 : 1);
    if (hidConfigDepth < 4) {
        currentHid = currentHid * 10 + (childIndex + 1);
        hidConfigDepth++;
        menuLog("HID config: down to " + String(currentHid), 4);
    }
}

void HidConfigMenuProvider::back() {
    MENU_SYS.exitHidConfigMode();
}

int BitIndexConfigMenuProvider::getItemCount() {
    int count = 0;
    
    // Navigation items
    if (hasPrevPage()) count++; // Prev Page
    if (hasNextPage()) count++; // Next Page
    
    // Bit indices for current page
    count += BITS_PER_PAGE;
    
    // Back item
    count += 1;
    
    return count;
}

const char* BitIndexConfigMenuProvider::getItemText(int index) {
    int itemIndex = 0;
    
    // Prev Page
    if (hasPrevPage()) {
        if (index == itemIndex) {
            snprintf(textBuffers[itemIndex], sizeof(textBuffers[itemIndex]), "< Prev Page");
            return textBuffers[itemIndex];
        }
        itemIndex++;
    }
    
    // Next Page  
    if (hasNextPage()) {
        if (index == itemIndex) {
            snprintf(textBuffers[itemIndex], sizeof(textBuffers[itemIndex]), "Next Page >");
            return textBuffers[itemIndex];
        }
        itemIndex++;
    }
    
    // Bit indices for current page
    uint8_t pageStart = getPageStartBit();
    for (int i = 0; i < BITS_PER_PAGE; i++) {
        if (index == itemIndex) {
            uint8_t bitIndex = pageStart + i;
            bool isConfigured = DATA_MGR.isBitIndexConfigured() && (DATA_MGR.getMyBitIndex() == bitIndex);
            snprintf(textBuffers[itemIndex], sizeof(textBuffers[itemIndex]), 
                    "Bit %d%s", bitIndex, isConfigured ? " *" : "");
            return textBuffers[itemIndex];
        }
        itemIndex++;
    }
    
    // Back
    if (index == itemIndex) {
        return "Back";
    }
    
    return "Invalid";
}

void BitIndexConfigMenuProvider::selectItem(int index) {
    int itemIndex = 0;
    
    // Prev Page
    if (hasPrevPage()) {
        if (index == itemIndex) {
            currentPage--;
            menuLog("Changed to page " + String(currentPage) + 
                   " (bits " + String(getPageStartBit()) + "-" + String(getPageEndBit()) + ")", 4);
            return;
        }
        itemIndex++;
    }
    
    // Next Page
    if (hasNextPage()) {
        if (index == itemIndex) {
            currentPage++;
            menuLog("Changed to page " + String(currentPage) + 
                   " (bits " + String(getPageStartBit()) + "-" + String(getPageEndBit()) + ")", 4);
            return;
        }
        itemIndex++;
    }
    
    // Bit indices for current page
    uint8_t pageStart = getPageStartBit();
    for (int i = 0; i < BITS_PER_PAGE; i++) {
        if (index == itemIndex) {
            uint8_t bitIndex = pageStart + i;
            menuLog("User selected bit index: " + String(bitIndex), 3);
            
            if (DATA_MGR.setMyBitIndex(bitIndex)) {
                DATA_MGR.updateStatus("Bit " + String(bitIndex) + " set");
                menuLog("Bit index configured to " + String(bitIndex), 3);
            } else {
                DATA_MGR.updateStatus("Invalid bit " + String(bitIndex));
                menuLog("Failed to set bit index: " + String(bitIndex), 1);
            }
            MENU_SYS.exitBitIndexConfigMode();
            return;
        }
        itemIndex++;
    }
    
    // Back
    if (index == itemIndex) {
        back();
        return;
    }
}

void BitIndexConfigMenuProvider::back() {
    MENU_SYS.exitBitIndexConfigMode();
}

// ============================================================================
// CONSOLE MESSAGE FUNCTIONS
// ============================================================================

void consoleLogSharedDataChange(uint32_t oldSharedData, uint32_t newSharedData) {
    // Check each bit for changes (B0 = bit 0, B1 = bit 1, etc.)
    for (int bitIndex = 0; bitIndex < 32; bitIndex++) {
        uint32_t oldBit = (oldSharedData >> bitIndex) & 1;
        uint32_t newBit = (newSharedData >> bitIndex) & 1;
        
        if (oldBit != newBit) {
            String msg;
            if (newBit == 1) {
                // Button pressed
                msg = "B" + String(bitIndex) + " Pressed";
            } else {
                // Button released
                msg = "B" + String(bitIndex) + " Released";
            }
            MENU_SYS.addConsoleMessage(msg);
        }
    }
}



// ============================================================================
// CONSOLE DISPLAY IMPLEMENTATION
// ============================================================================

ConsoleDisplay::ConsoleDisplay() : messageCount(0), oldestIndex(0) {
    // Initialize all messages to empty
    for (int i = 0; i < MAX_MESSAGES; i++) {
        messages[i].message = "";
        messages[i].timestamp = 0;
    }
}

void ConsoleDisplay::addMessage(const String& msg) {
    if (messageCount < MAX_MESSAGES) {
        // Add to next available slot
        messages[messageCount].message = msg;
        messages[messageCount].timestamp = millis();
        messageCount++;
    } else {
        // Replace oldest message
        messages[oldestIndex].message = msg;
        messages[oldestIndex].timestamp = millis();
        oldestIndex = (oldestIndex + 1) % MAX_MESSAGES;
    }
}

void ConsoleDisplay::clear() {
    messageCount = 0;
    oldestIndex = 0;
    for (int i = 0; i < MAX_MESSAGES; i++) {
        messages[i].message = "";
        messages[i].timestamp = 0;
    }
}

const ConsoleMessage& ConsoleDisplay::getMessage(int index) const {
    if (index < 0 || index >= messageCount) {
        static ConsoleMessage emptyMsg = {"", 0};
        return emptyMsg;
    }
    
    // Calculate actual index considering the circular buffer
    int actualIndex;
    if (messageCount < MAX_MESSAGES) {
        actualIndex = index;
    } else {
        actualIndex = (oldestIndex + index) % MAX_MESSAGES;
    }
    
    return messages[actualIndex];
}

// ============================================================================
// SINGLETON IMPLEMENTATION
// ============================================================================

MenuSystem& MenuSystem::getInstance() {
    static MenuSystem instance;
    return instance;
}

MenuSystem::MenuSystem() : 
    currentMenu(nullptr),
    currentMenuSize(0),
    selectedIndex(0),
    inMenuMode(false),
    inConsoleMode(false),
    inDeviceConfigMode(false),
    menuStackDepth(0),
    dynamicProvider(nullptr) {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void MenuSystem::initialize() {
    Serial.println("MENU_SYS: initialize() - Start");
    
    // Ensure any existing dynamic provider is cleaned up
    if (dynamicProvider) {
        Serial.println("MENU_SYS: initialize() - Deleting existing dynamic provider");
        delete dynamicProvider;
        dynamicProvider = nullptr;
    }
    
    Serial.println("MENU_SYS: initialize() - Setting main menu");
    // Directly set the main menu without a chain of calls
    currentMenu = mainMenu;
    currentMenuSize = mainMenuSize;
    selectedIndex = 0;
    
    Serial.println("MENU_SYS: initialize() - Setting initial state");
    inMenuMode = false;      // Start in status display mode
    menuStackDepth = 0;      // Clear menu stack
    
    Serial.println("MENU_SYS: initialize() - Complete");
    menuLog("MenuSystem initialized - starting in status display mode", 3);
}

// ============================================================================
// MENU NAVIGATION
// ============================================================================

void MenuSystem::navigateUp() {
    if (selectedIndex > 0) {
        selectedIndex--;
    } else {
        selectedIndex = getCurrentMenuSize() - 1; // Wrap around
    }
    menuLog("Navigate up to index " + String(selectedIndex), 4);
}

void MenuSystem::navigateDown() {
    if (selectedIndex < getCurrentMenuSize() - 1) {
        selectedIndex++;
    } else {
        selectedIndex = 0; // Wrap around
    }
    menuLog("Navigate down to index " + String(selectedIndex), 4);
}

void MenuSystem::selectCurrentItem() {
    if (dynamicProvider) {
        dynamicProvider->selectItem(selectedIndex);
        if (dynamicProvider) {
            selectedIndex = 0;
        }
        return;
    }

    if (!currentMenu || selectedIndex >= currentMenuSize) {
        return;
    }
    
    const MenuItem& item = currentMenu[selectedIndex];
    
    menuLog("Selected: " + String(item.text), 3);
    
    if (item.submenu != nullptr) {
        // Push current menu onto stack before navigating to submenu
        if (menuStackDepth < MAX_MENU_DEPTH) {
            menuStack[menuStackDepth].menu = currentMenu;
            menuStack[menuStackDepth].menuSize = currentMenuSize;
            menuStack[menuStackDepth].selectedIndex = selectedIndex;
            menuStackDepth++;
        }
        
        // Navigate to submenu
        setCurrentMenu(item.submenu, item.submenuSize);
        selectedIndex = 0;
        menuLog("Entered submenu: " + String(item.text) + " (depth: " + String(menuStackDepth) + ")", 3);
    } else if (item.action) {
        // Execute action
        item.action();
        menuLog("Executed action: " + String(item.text), 3);
    }
}

void MenuSystem::backToMain() {
    Serial.println("MENU_SYS: backToMain() - Start");
    // Clean up any dynamic provider before going to main menu
    if (dynamicProvider) {
        delete dynamicProvider;
        dynamicProvider = nullptr;
    }
    setCurrentMenu(mainMenu, mainMenuSize);
    selectedIndex = 0;
    menuStackDepth = 0;  // Clear the stack
    DATA_MGR.updateStatus("Main Menu");
    Serial.println("MENU_SYS: backToMain() - Complete");
    menuLog("Returned to main menu", 3);
}

void MenuSystem::backToPreviousMenu() {
    if (dynamicProvider) {
        dynamicProvider->back();
        return;
    }

    if (menuStackDepth > 0) {
        // Pop the previous menu from the stack
        menuStackDepth--;
        MenuLevel& prevLevel = menuStack[menuStackDepth];
        
        currentMenu = prevLevel.menu;
        currentMenuSize = prevLevel.menuSize;
        selectedIndex = prevLevel.selectedIndex;
        
        menuLog("Returned to previous menu (depth: " + String(menuStackDepth) + ")", 3);
    } else {
        // No previous menu, go to main menu
        backToMain();
        menuLog("No previous menu, returned to main menu", 3);
    }
}

// ============================================================================
// MENU STATE
// ============================================================================

void MenuSystem::setCurrentMenu(const MenuItem* menu, int menuSize) {
    Serial.println("MENU_SYS: setCurrentMenu() - Start");
    // The call to exitHidConfigMode here was creating a recursive loop.
    // It's better to handle dynamicProvider cleanup explicitly where menus are set.
    // exitHidConfigMode(); 
    currentMenu = menu;
    currentMenuSize = menuSize;
    selectedIndex = 0;
    Serial.println("MENU_SYS: setCurrentMenu() - Complete");
}

int MenuSystem::getCurrentMenuSize() const {
    if (dynamicProvider) {
        return dynamicProvider->getItemCount();
    }
    return currentMenuSize;
}

const char* MenuSystem::getCurrentItemText() const {
    if (dynamicProvider) {
        return dynamicProvider->getItemText(selectedIndex);
    }
    if (currentMenu && selectedIndex >= 0 && selectedIndex < currentMenuSize) {
        return currentMenu[selectedIndex].text;
    }
    return "Invalid";
}

// ============================================================================
// DISPLAY UPDATE
// ============================================================================

void MenuSystem::updateDisplay() {
    if (inMenuMode) {
        showMenuDisplay();
    } else if (inConsoleMode) {
        showConsoleDisplay();
    } else {
        showStatusDisplay();
    }
}

void MenuSystem::showStatusDisplay() {
#if ENABLE_OLED
    drawStatusOLED();
#else
    displayStatusSerial();
#endif
}

void MenuSystem::showMenuDisplay() {
#if ENABLE_OLED
    drawMenuOLED();
#else
    displaySerial();
#endif
}

void MenuSystem::setDisplayMode(bool menuMode) {
    if (inMenuMode != menuMode) {
        inMenuMode = menuMode;
        if (menuMode) {
            menuLog("Entered menu mode", 3);
            // Reset to main menu when entering menu mode
            setCurrentMenu(mainMenu, mainMenuSize);
            selectedIndex = 0;
            menuStackDepth = 0;  // Clear menu stack
        } else {
            menuLog("Exited to status display mode", 3);
            menuStackDepth = 0;  // Clear menu stack when exiting
        }
    }
}

void MenuSystem::setConsoleMode(bool consoleMode) {
    if (inConsoleMode != consoleMode) {
        inConsoleMode = consoleMode;
        if (consoleMode) {
            menuLog("Entered console display mode", 3);
        } else {
            menuLog("Exited console display mode", 3);
        }
    }
}

void MenuSystem::addConsoleMessage(const String& msg) {
    consoleDisplay.addMessage(msg);
}

void MenuSystem::clearConsoleMessages() {
    consoleDisplay.clear();
}

void MenuSystem::showConsoleDisplay() {
#if ENABLE_OLED
    drawConsoleOLED();
#else
    displayConsoleSerial();
#endif
}

void MenuSystem::drawConsoleOLED() {
#if ENABLE_OLED
    display.clearBuffer();
    
    // Header with HID and bit index info
    display.setFont(u8g2_font_ncenB08_tr);
    display.setCursor(0, 10);
    if (DATA_MGR.isHIDConfigured()) {
        String header = "HID:" + DATA_MGR.formatHID(DATA_MGR.getMyHID());
        if (DATA_MGR.isRoot()) {
            header += "(R)";
        }
        
        // Always show bit index status - either value or "None"
        if (DATA_MGR.isBitIndexConfigured()) {
            header += " B:" + String(DATA_MGR.getMyBitIndex());
        } else {
            header += " B:None";
        }
        
        // Ensure header fits on display (128 pixels wide, ~16 chars max for this font)
        if (header.length() > 16) {
            // Truncate if too long, but prioritize showing bit index
            if (DATA_MGR.isBitIndexConfigured()) {
                header = "HID:" + String(DATA_MGR.getMyHID()) + " B:" + String(DATA_MGR.getMyBitIndex());
                if (DATA_MGR.isRoot()) {
                    header = "HID:" + String(DATA_MGR.getMyHID()) + "(R) B:" + String(DATA_MGR.getMyBitIndex());
                }
            } else {
                header = "HID:" + String(DATA_MGR.getMyHID()) + " B:None";
                if (DATA_MGR.isRoot()) {
                    header = "HID:" + String(DATA_MGR.getMyHID()) + "(R) B:None";
                }
            }
        }
        
        display.print(header);
    } else {
        display.print("Device Not Configured");
    }
    
    // Draw separator line
    display.drawHLine(0, 12, 128);
    
    // Console messages section
    display.setFont(u8g2_font_ncenR08_tr);
    
    const int maxVisibleLines = 4; // Lines 24, 36, 48, 60 (4 lines total)
    const int lineHeight = 12;
    const int startY = 24;
    
    int messageCount = consoleDisplay.getMessageCount();
    
    // Show most recent messages first (at the top)
    for (int i = 0; i < maxVisibleLines; i++) {
        // Calculate index from most recent to oldest
        int messageIndex = messageCount - 1 - i;
        
        // Skip if no more messages
        if (messageIndex < 0) break;
        
        const ConsoleMessage& msg = consoleDisplay.getMessage(messageIndex);
        int y = startY + (i * lineHeight);
        
        // Truncate message if too long for display
        String displayMsg = msg.message;
        if (displayMsg.length() > 16) {
            displayMsg = displayMsg.substring(0, 16);
        }
        
        display.setCursor(0, y);
        display.print(displayMsg);
    }
    
    display.sendBuffer();
#endif
}

void MenuSystem::displayConsoleSerial() {
    // Console disabled - output to Serial instead
    static unsigned long lastSerialUpdate = 0;
    
    // Update serial output every 1 second
    if (millis() - lastSerialUpdate > 1000) {
        Serial.println("=== ESP-NOW Console ===");
        
        if (DATA_MGR.isHIDConfigured()) {
            String header = "HID:" + DATA_MGR.formatHID(DATA_MGR.getMyHID());
            if (DATA_MGR.isRoot()) {
                header += "(R)";
            }
            if (DATA_MGR.isBitIndexConfigured()) {
                header += " B:" + String(DATA_MGR.getMyBitIndex());
            } else {
                header += " B:None";
            }
            Serial.println(header);
        }
        
        int messageCount = consoleDisplay.getMessageCount();
        Serial.println("Recent messages (" + String(messageCount) + "):");
        
        for (int i = 0; i < messageCount && i < 10; i++) {
            const ConsoleMessage& msg = consoleDisplay.getMessage(i);
            Serial.println("  " + msg.message);
        }
        
        Serial.println("======================");
        lastSerialUpdate = millis();
    }
}

void MenuSystem::drawMenuOLED() {
#if ENABLE_OLED
    unsigned long drawStart = millis();
    
    display.clearBuffer();
    unsigned long clearTime = millis() - drawStart;
    
    display.setFont(u8g2_font_ncenB08_tr);
    unsigned long fontTime = millis() - drawStart - clearTime;

    // Get the menu title
    const char* title = (menuStackDepth > 0) ? menuStack[menuStackDepth - 1].menu[menuStack[menuStackDepth - 1].selectedIndex].text : "Main Menu";
    display.drawStr(0, 10, title);
    display.drawHLine(0, 12, 128);
    unsigned long titleTime = millis() - drawStart - clearTime - fontTime;

    display.setFont(u8g2_font_ncenR08_tr);
    int menuSize = getCurrentMenuSize();
    
    // Calculate scrolling parameters
    const int maxVisibleItems = 4; // Maximum items that fit on screen (lines 24, 36, 48, 60)
    const int lineHeight = 12;
    const int startY = 24;
    
    // Calculate scroll offset to keep selected item visible
    int scrollOffset = 0;
    if (selectedIndex >= maxVisibleItems) {
        scrollOffset = selectedIndex - maxVisibleItems + 1;
    }
    
    // Draw visible menu items
    for (int i = 0; i < maxVisibleItems && (i + scrollOffset) < menuSize; i++) {
        int itemIndex = i + scrollOffset;
        int y = startY + (i * lineHeight);
        
        // Draw cursor for selected item
        if (itemIndex == selectedIndex) {
            display.drawStr(0, y, ">");
        }
        
        // Draw menu item text
        const char* itemText = getCurrentItemText(itemIndex);
        display.drawStr(10, y, itemText);
    }
    unsigned long menuTime = millis() - drawStart - clearTime - fontTime - titleTime;
    
    // Draw scroll indicators if needed
    if (scrollOffset > 0) {
        // Up arrow indicator
        display.drawStr(120, 20, "^");
    }
    if (scrollOffset + maxVisibleItems < menuSize) {
        // Down arrow indicator  
        display.drawStr(120, 60, "v");
    }
    
    unsigned long preSendTime = millis() - drawStart;
    
    // This is likely the blocking operation
    display.sendBuffer();
    
    unsigned long totalTime = millis() - drawStart;
    unsigned long sendTime = totalTime - preSendTime;
    
    // Log timing if it's taking too long
    if (totalTime > 50) {
        Serial.println("=== OLED MENU TIMING ANALYSIS ===");
        Serial.println("Clear buffer: " + String(clearTime) + "ms");
        Serial.println("Set font: " + String(fontTime) + "ms");
        Serial.println("Draw title: " + String(titleTime) + "ms");
        Serial.println("Draw menu items: " + String(menuTime) + "ms");
        Serial.println("Send buffer: " + String(sendTime) + "ms");
        Serial.println("Total time: " + String(totalTime) + "ms");
        Serial.println("================================");
    }
#endif
}

const char* MenuSystem::getCurrentItemText(int index) const {
    if (dynamicProvider) {
        return dynamicProvider->getItemText(index);
    }
    if (currentMenu && index >= 0 && index < currentMenuSize) {
        return currentMenu[index].text;
    }
    return "Invalid";
}

void MenuSystem::displaySerial() {
    // OLED disabled - output status to Serial instead
    static unsigned long lastSerialUpdate = 0;
    static String lastStatus = "";
    static int lastSelectedIndex = -1;
    
    String currentStatus = DATA_MGR.getCurrentStatus();
    bool needsUpdate = (currentStatus != lastStatus || selectedIndex != lastSelectedIndex);
    
    // Update serial output every 2 seconds or when status changes
    if (millis() - lastSerialUpdate > 2000 || needsUpdate) {
        Serial.println("=== ESP-NOW Tree Status ===");
        
        if (inMenuMode) {
            Serial.println("=== MENU MODE ===");
            Serial.println("Selected item: " + String(selectedIndex + 1) + "/" + String(getCurrentMenuSize()));
            Serial.println("Current item: " + String(getCurrentItemText()));
            
            // Show menu items
            int menuSize = getCurrentMenuSize();
            for (int i = 0; i < menuSize; i++) {
                String prefix = (i == selectedIndex) ? "> " : "  ";
                Serial.println(prefix + String(getCurrentItemText(i)));
            }
        } else if (inConsoleMode) {
            Serial.println("=== CONSOLE MODE ===");
            Serial.println("Console messages: " + String(consoleDisplay.getMessageCount()));
        } else {
            Serial.println("=== STATUS MODE ===");
            if (TREE_NET.isHIDConfigured()) {
                Serial.println("HID: " + TREE_NET.getHIDStatus());
            }
            
            const NetworkStats& stats = DATA_MGR.getNetworkStats();
            Serial.println("TX: " + String(stats.messagesSent) + 
                          " RX: " + String(stats.messagesReceived));
            if(stats.signalStrength != 0) {
                Serial.println("RSSI: " + String((int)stats.signalStrength) + " dBm");
            }
        }
        
        Serial.println("Navigation: Press=Nav, Long=Select, Double=Menu");
        Serial.println("Status: " + currentStatus);
        Serial.println("==========================");
        
        lastSerialUpdate = millis();
        lastStatus = currentStatus;
        lastSelectedIndex = selectedIndex;
    }
}

void MenuSystem::drawStatusOLED() {
#if ENABLE_OLED
    unsigned long drawStart = millis();
    
    static int lastSelectedIndex = -1;
    static const MenuItem* lastMenu = nullptr;
    static bool wasDynamic = false;
    static uint8_t lastInputStates = 255; // Track I/O state changes
    static uint8_t lastOutputStates = 255;
    static uint32_t lastSharedData = 0xFFFFFFFF; // Track shared data changes
    static uint16_t lastHID = 0xFFFF; // Track HID changes
    static uint8_t lastBitIndex = 0xFF; // Track Bit Index changes
    static bool lastHIDConfigured = false; // Track HID configuration status
    static bool lastBitIndexConfigured = false; // Track Bit Index configuration status
    static char buffer[12]; // Smaller static buffer to reduce stack usage
    bool isDynamic = (dynamicProvider != nullptr);

    // Get current I/O states and shared data to check for changes
    const DeviceSpecificData& myData = DATA_MGR.getMyDeviceData();
    uint8_t currentInputStates = myData.input_states;
    uint8_t currentOutputStates = myData.output_states;
    uint32_t currentSharedData = DATA_MGR.getDistributedIOSharedData().words[0];
    
    // Get current configuration values
    uint16_t currentHID = DATA_MGR.getMyHID();
    uint8_t currentBitIndex = DATA_MGR.getMyBitIndex();
    bool currentHIDConfigured = DATA_MGR.isHIDConfigured();
    bool currentBitIndexConfigured = DATA_MGR.isBitIndexConfigured();

    // Check if we need to redraw: menu state changed OR I/O state changed OR shared data changed OR configuration changed
    bool needsRedraw = (isDynamic || wasDynamic || 
                       selectedIndex != lastSelectedIndex || 
                       currentMenu != lastMenu ||
                       currentInputStates != lastInputStates ||
                       currentOutputStates != lastOutputStates ||
                       currentSharedData != lastSharedData ||
                       currentHID != lastHID ||
                       currentBitIndex != lastBitIndex ||
                       currentHIDConfigured != lastHIDConfigured ||
                       currentBitIndexConfigured != lastBitIndexConfigured);

    if (!needsRedraw) {
        return; // Skip redraw if nothing changed
    }

    // Update state for next frame
    lastSelectedIndex = selectedIndex;
    lastMenu = currentMenu;
    wasDynamic = isDynamic;
    lastInputStates = currentInputStates;
    lastOutputStates = currentOutputStates;
    lastSharedData = currentSharedData;
    lastHID = currentHID;
    lastBitIndex = currentBitIndex;
    lastHIDConfigured = currentHIDConfigured;
    lastBitIndexConfigured = currentBitIndexConfigured;

    // Debug: Check bit index configuration status
    if (currentBitIndexConfigured != lastBitIndexConfigured || currentBitIndex != lastBitIndex) {
        Serial.println("Bit Index Status: " + String(currentBitIndexConfigured ? "Configured" : "Not Configured") + 
                      " Value: " + String(currentBitIndex));
    }
    
    // Debug: Check HID configuration status
    if (currentHIDConfigured != lastHIDConfigured || currentHID != lastHID) {
        Serial.println("HID Status: " + String(currentHIDConfigured ? "Configured" : "Not Configured") + 
                      " Value: " + String(currentHID));
    }

    display.clearBuffer();
    unsigned long clearTime = millis() - drawStart;
    
    // Header with HID and bit index info
    display.setFont(u8g2_font_ncenB08_tr);
    display.setCursor(0, 10);
    if (DATA_MGR.isHIDConfigured()) {
        String header = "HID:" + DATA_MGR.formatHID(DATA_MGR.getMyHID());
        if (DATA_MGR.isRoot()) {
            header += "(R)";
        }
        
        // Always show bit index status - either value or "None"
        if (DATA_MGR.isBitIndexConfigured()) {
            header += " B:" + String(DATA_MGR.getMyBitIndex());
        } else {
            header += " B:None";
        }
        
        // Debug: Log the header to see what's being displayed
        static String lastHeader = "";
        if (header != lastHeader) {
            Serial.println("OLED Header: " + header);
            lastHeader = header;
        }
        
        // Ensure header fits on display (128 pixels wide, ~16 chars max for this font)
        if (header.length() > 16) {
            // Truncate if too long, but prioritize showing bit index
            if (DATA_MGR.isBitIndexConfigured()) {
                header = "HID:" + String(DATA_MGR.getMyHID()) + " B:" + String(DATA_MGR.getMyBitIndex());
                if (DATA_MGR.isRoot()) {
                    header = "HID:" + String(DATA_MGR.getMyHID()) + "(R) B:" + String(DATA_MGR.getMyBitIndex());
                }
            } else {
                header = "HID:" + String(DATA_MGR.getMyHID()) + " B:None";
                if (DATA_MGR.isRoot()) {
                    header = "HID:" + String(DATA_MGR.getMyHID()) + "(R) B:None";
                }
            }
        }
        
        display.print(header);
    } else {
        display.print("Device Not Configured");
    }
    unsigned long headerTime = millis() - drawStart - clearTime;
    
    // I/O states section
    display.setFont(u8g2_font_ncenR08_tr);
    
    uint8_t inputs = myData.input_states;
    uint8_t outputs = myData.output_states;
    uint32_t shared = DATA_MGR.getDistributedIOSharedData().words[0];
    
    // Debug: Log what we're about to display
    static uint8_t lastDisplayedInputs = 255; // Initialize to invalid value
    static uint8_t lastDisplayedOutputs = 255;
    static uint32_t lastDisplayedShared = 0xFFFFFFFF;
    
    if (inputs != lastDisplayedInputs || outputs != lastDisplayedOutputs || shared != lastDisplayedShared) {
        Serial.println("[DISPLAY][UPDATE] OLED showing:");
        Serial.println("  Input:  " + String(inputs, BIN) + " (" + String(inputs) + ")");
        Serial.println("  Output: " + String(outputs, BIN) + " (" + String(outputs) + ")");
        Serial.println("  Shared: 0x" + String(shared, HEX) + " = " + String(shared, BIN));
        lastDisplayedInputs = inputs;
        lastDisplayedOutputs = outputs;
        lastDisplayedShared = shared;
    }
    
    // Input states (as binary)
    display.setCursor(0, 24);
    display.print("Input: ");
    for (int i = 7; i >= 0; i--) {
        display.print((inputs & (1 << i)) ? "1" : "0");
        if (i == 4) display.print(" "); // Space between nibbles
    }
    
    // Output states (as binary)
    display.setCursor(0, 36);
    display.print("Output:");
    for (int i = 7; i >= 0; i--) {
        display.print((outputs & (1 << i)) ? "1" : "0");
        if (i == 4) display.print(" "); // Space between nibbles
    }
    
    // Shared data (as binary bitmap)
    display.setCursor(0, 48);
    display.print("Shared:");
    // Display bits 12-0 (13 bits total), grouped for readability
    // Show most significant bits first (bits 12-0)
    for (int i = 12; i >= 0; i--) {
        display.print((shared & (1UL << i)) ? "1" : "0");
        // Add space every 4 bits for readability
        if (i > 0 && (i % 4) == 0) {
            display.print(" ");
        }
    }
    unsigned long ioTime = millis() - drawStart - clearTime - headerTime;
    
    // Network stats section
    display.setCursor(0, 60);
    display.setFont(u8g2_font_6x10_tr);
    
    const NetworkStats& stats = DATA_MGR.getNetworkStats();
    if (stats.lastMessageTime > 0) {
        String logLine = "RX:" + String(stats.messagesReceived);
        if (stats.signalStrength != 0) {
            logLine += " RSSI:" + String((int)stats.signalStrength);
        }
        display.print(logLine);
    } else {
        display.print("RX: No traffic");
    }
    unsigned long statsTime = millis() - drawStart - clearTime - headerTime - ioTime;
    
    unsigned long preSendTime = millis() - drawStart;
    
    // This is likely the blocking operation
    display.sendBuffer();
    
    unsigned long totalTime = millis() - drawStart;
    unsigned long sendTime = totalTime - preSendTime;
    
    // Log timing if it's taking too long
    if (totalTime > 50) {
        Serial.println("=== OLED STATUS TIMING ANALYSIS ===");
        Serial.println("Clear buffer: " + String(clearTime) + "ms");
        Serial.println("Draw header: " + String(headerTime) + "ms");
        Serial.println("Draw I/O states: " + String(ioTime) + "ms");
        Serial.println("Draw stats: " + String(statsTime) + "ms");
        Serial.println("Send buffer: " + String(sendTime) + "ms");
        Serial.println("Total time: " + String(totalTime) + "ms");
        Serial.println("===================================");
    }
#endif
}

void MenuSystem::displayStatusSerial() {
    static unsigned long lastSerialUpdate = 0;
    
    // Update serial output every 2 seconds
    if (millis() - lastSerialUpdate > 2000) {
        Serial.println("=== Device Status ===");
        
        // Device configuration status
        if (DATA_MGR.isHIDConfigured()) {
            String hidStatus = "HID: " + DATA_MGR.formatHID(DATA_MGR.getMyHID());
            if (DATA_MGR.isRoot()) hidStatus += " (Root)";
            Serial.println(hidStatus);
        } else {
            Serial.println("HID: Not configured");
        }
        
        if (DATA_MGR.isBitIndexConfigured()) {
            Serial.println("Bit Index: " + String(DATA_MGR.getMyBitIndex()));
        } else {
            Serial.println("Bit Index: Not configured");
        }
        
        // I/O states
        const DeviceSpecificData& myData = DATA_MGR.getMyDeviceData();
        Serial.print("Input:  ");
        for (int i = 7; i >= 0; i--) {
            Serial.print((myData.input_states & (1 << i)) ? "1" : "0");
        }
        Serial.println();
        
        Serial.print("Output: ");
        for (int i = 7; i >= 0; i--) {
            Serial.print((myData.output_states & (1 << i)) ? "1" : "0");
        }
        Serial.println();
        
        Serial.print("Shared: ");
        uint32_t shared = DATA_MGR.getDistributedIOSharedData().words[0];
        char sharedStr[12];
        snprintf(sharedStr, sizeof(sharedStr), "0x%08lX", shared);
        Serial.println(sharedStr);
        
        // Network stats
        const NetworkStats& stats = DATA_MGR.getNetworkStats();
        Serial.println("TX: " + String(stats.messagesSent) + 
                      " RX: " + String(stats.messagesReceived));
        if (stats.signalStrength != 0) {
            Serial.println("RSSI: " + String((int)stats.signalStrength) + " dBm");
        }
        
        Serial.println("Status: " + DATA_MGR.getCurrentStatus());
        Serial.println("Double-click GPIO_0 for menu");
        Serial.println("====================");
        
        lastSerialUpdate = millis();
    }
}

// ============================================================================
// MENU ACTION FUNCTIONS
// ============================================================================

void actionConfigureDevice() {
    MENU_SYS.startDeviceConfiguration();
}

void actionClearAllConfig() {
    DATA_MGR.clearAllConfiguration();
    DATA_MGR.updateStatus("All config cleared");
    menuLog("All device configuration cleared", 3);
}

void actionShowDeviceStatus() {
    MENU_SYS.setDisplayMode(false);
    
    String status = "";
    if (DATA_MGR.isHIDConfigured()) {
        status += "HID:" + String(DATA_MGR.getMyHID());
        if (DATA_MGR.isRoot()) status += "(R)";
    } else {
        status += "HID:None";
    }
    
    if (DATA_MGR.isBitIndexConfigured()) {
        status += " B:" + String(DATA_MGR.getMyBitIndex());
    } else {
        status += " B:None";
    }
    
    // Debug output to serial
    Serial.println("Device Status Debug:");
    Serial.println("  HID Configured: " + String(DATA_MGR.isHIDConfigured() ? "Yes" : "No"));
    Serial.println("  HID Value: " + String(DATA_MGR.getMyHID()));
    Serial.println("  Bit Index Configured: " + String(DATA_MGR.isBitIndexConfigured() ? "Yes" : "No"));
    Serial.println("  Bit Index Value: " + String(DATA_MGR.getMyBitIndex()));
    Serial.println("  Status String: " + status);
    
    DATA_MGR.updateStatus(status);
    menuLog("Device status displayed: " + status, 3);
}

void actionSendDataReport() {
    MENU_SYS.setDisplayMode(false);
    if (DATA_MGR.isRoot()) {
        DATA_MGR.updateStatus("Root: No report needed");
        menuLog("Root node doesn't send data reports", 3);
    } else if (!DATA_MGR.isHIDConfigured()) {
        DATA_MGR.updateStatus("HID not configured");
        menuLog("Cannot send data report - HID not configured", 2);
    } else {
        bool success = sendDataReportToParent();
        if (success) {
            DATA_MGR.updateStatus("Data report sent");
            menuLog("Data report sent to parent", 3);
        } else {
            DATA_MGR.updateStatus("Report failed");
            menuLog("Failed to send data report", 2);
        }
    }
}

void actionShowAggregatedDevices() {
    DATA_MGR.showAggregatedDevices();
}

void actionClearAggregatedData() {
    DATA_MGR.clearAggregatedData();
    menuLog("Aggregated data cleared", 3);
}

// Distributed I/O actions
void actionShowDistributedIOStatus() {
    String status = DATA_MGR.getDistributedIOStatus();
    DATA_MGR.updateStatus(status);
    menuLog("Distributed I/O status displayed: " + status, 4);
}

// Settings actions
void actionToggleLongRange() {
    bool currentState = isLongRangeModeEnabled();
    bool success = false;
    
    if (currentState) {
        success = disableLongRangeMode();
        DATA_MGR.updateStatus(success ? "LR Mode OFF" : "LR Toggle Failed");
    } else {
        success = enableLongRangeMode();
        DATA_MGR.updateStatus(success ? "LR Mode ON" : "LR Toggle Failed");
    }
    
    String newState = isLongRangeModeEnabled() ? "ON" : "OFF";
    menuLog("Long Range mode toggled to " + newState, 3);
}

void actionShowNetworkInfo() {
    const NetworkStats& stats = DATA_MGR.getNetworkStats();
    String info = "TX:" + String(stats.messagesSent) + 
                  " RX:" + String(stats.messagesReceived);
    if(stats.signalStrength != 0) {
        info += " " + String((int)stats.signalStrength) + "dBm";
    }
    DATA_MGR.updateStatus(info);
    menuLog("Network info displayed", 4);
}

// Navigation actions
void actionBackToPrevious() {
    MENU_SYS.backToPreviousMenu();
}

void actionExitMenu() {
    MENU_SYS.setDisplayMode(false);  // Exit to status display mode
    DATA_MGR.updateStatus("Status Mode");
    menuLog("Exited menu to status display", 3);
}

void actionToggleConsole() {
    static bool consoleEnabled = false;
    consoleEnabled = !consoleEnabled;
    
    if (consoleEnabled) {
        MENU_SYS.setConsoleMode(true);
        MENU_SYS.setDisplayMode(false);  // Exit menu mode when entering console mode
        DATA_MGR.updateStatus("Console Mode");
        menuLog("Entered console display mode", 3);
    } else {
        MENU_SYS.setConsoleMode(false);
        DATA_MGR.updateStatus("Status Mode");
        menuLog("Exited console display mode", 3);
    }
}

// I/O Device actions
void actionShowIOStatus() {
    IoDevice& ioDevice = IoDevice::getInstance();
    String status = ioDevice.getIOStatus();
    
    // Add test mode status
    status += " | Test: " + String(ioDevice.isTestModeEnabled() ? "ON" : "OFF");
    
    DATA_MGR.updateStatus(status);
    menuLog("I/O status displayed: " + status, 4);
}

void actionShowPinConfig() {
    MENU_SYS.setDisplayMode(false);
    DATA_MGR.updateStatus("Pin Config");
    menuLog("Showing pin configuration", 3);
}

void actionToggleIOAutoReport() {
    MENU_SYS.setDisplayMode(false);
    static bool enabled = false;
    enabled = !enabled;
    DATA_MGR.updateStatus(enabled ? "IO Auto ON" : "IO Auto OFF");
    menuLog("I/O auto report: " + String(enabled ? "ON" : "OFF"), 3);
}

void actionSetSharedData() {
    MENU_SYS.setDisplayMode(false);
    
    if (!DATA_MGR.isRoot()) {
        DATA_MGR.updateStatus("Only root can set");
        menuLog("Only root node can set shared data", 2);
        return;
    }
    
    // Test shared data by setting a pattern based on current time
    static uint8_t testPattern = 0;
    testPattern = (testPattern + 1) % 8; // Cycle through 0-7
    
    // Create test shared data with rotating pattern
    DistributedIOData testData;
    memset(&testData, 0, sizeof(DistributedIOData));
    testData.words[0] = (1UL << testPattern); // Set one bit at a time
    
    DATA_MGR.setDistributedIOSharedData(testData);
    IO_DEVICE.broadcastSharedData();
    
    DATA_MGR.updateStatus("Test bit " + String(testPattern) + " set");
    menuLog("Shared data test: set bit " + String(testPattern) + 
           " (0x" + String(testData.words[0], HEX) + ")", 3);
}

void actionToggleTestMode() {
    IoDevice& ioDevice = IoDevice::getInstance();
    bool currentMode = ioDevice.isTestModeEnabled();
    ioDevice.enableTestMode(!currentMode);
    
    String status = currentMode ? "Test Mode OFF" : "Test Mode ON";
    DATA_MGR.updateStatus(status);
    menuLog("Test mode toggled: " + String(!currentMode ? "enabled" : "disabled"), 3);
}

void actionTestOutputs() {
    MENU_SYS.setDisplayMode(false);
    
    // Test output functionality
    static uint8_t testPattern = 0;
    testPattern = (testPattern + 1) % 8; // Cycle through patterns
    
    IO_DEVICE.updateOutputs(testPattern);
    
    DATA_MGR.updateStatus("Output test: " + String(testPattern, BIN));
    menuLog("Output test pattern: " + String(testPattern, BIN), 3);
}

// Tree Network submenu
const MenuItem treeNetworkMenu[] = {
    {"Show Aggregated Devices", actionShowAggregatedDevices, nullptr, 0},
    {"Clear Aggregated Data", actionClearAggregatedData, nullptr, 0},
    {"Show Distributed I/O", actionShowDistributedIOStatus, nullptr, 0},
    {"Send Broadcast Test", actionSendBroadcast, nullptr, 0},
    {"Send Data Report", actionSendDataReport, nullptr, 0},
    {"Set Shared Data", actionSetSharedData, nullptr, 0},
    {"Show Tree Stats", actionShowTreeStats, nullptr, 0},
    {"Reset Stats", actionResetStats, nullptr, 0},
    {"Back", actionBackToPrevious, nullptr, 0}
};
const int treeNetworkMenuSize = sizeof(treeNetworkMenu) / sizeof(MenuItem);

// Basic settings submenu
const MenuItem settingsMenu[] = {
    {"Toggle Long Range", actionToggleLongRange, nullptr, 0},
    {"Show Network Info", actionShowNetworkInfo, nullptr, 0},
    {"Back", actionBackToPrevious, nullptr, 0}
};
const int settingsMenuSize = sizeof(settingsMenu) / sizeof(MenuItem);

// Device Configuration submenu (Advanced)
const MenuItem deviceConfigMenu[] = {
    {"Clear All Config", actionClearAllConfig, nullptr, 0},
    {"Back", actionBackToPrevious, nullptr, 0}
};
const int deviceConfigMenuSize = sizeof(deviceConfigMenu) / sizeof(MenuItem);

// I/O Device submenu
const MenuItem ioDeviceMenu[] = {
    {"Show I/O Status", actionShowIOStatus, nullptr, 0},
    {"Toggle Test Mode", actionToggleTestMode, nullptr, 0},
    {"Test Outputs", actionTestOutputs, nullptr, 0},
    {"Back", actionBackToPrevious, nullptr, 0}
};
const int ioDeviceMenuSize = sizeof(ioDeviceMenu) / sizeof(MenuItem);

// Info submenu  
const MenuItem infoMenu[] = {
    {"Show Uptime", actionShowUptime, nullptr, 0},
    {"Show Last Sender", actionShowLastSender, nullptr, 0},
    {"Show Network Info", actionShowNetworkInfo, nullptr, 0},
    {"Back", actionBackToPrevious, nullptr, 0}
};
const int infoMenuSize = sizeof(infoMenu) / sizeof(MenuItem);

// Advanced menu for testing and debugging
const MenuItem advancedMenu[] = {
    {"Device Config", nullptr, deviceConfigMenu, deviceConfigMenuSize},
    {"Tree Network", nullptr, treeNetworkMenu, treeNetworkMenuSize},
    {"I/O Device", nullptr, ioDeviceMenu, ioDeviceMenuSize},
    {"Info", nullptr, infoMenu, infoMenuSize},
    {"Back", actionBackToPrevious, nullptr, 0}
};
const int advancedMenuSize = sizeof(advancedMenu) / sizeof(MenuItem);

// Simple main menu for end users
const MenuItem mainMenu[] = {
    {"Configure Device", actionConfigureDevice, nullptr, 0},
    {"Show Device Status", actionShowDeviceStatus, nullptr, 0},
    {"Console Mode", actionToggleConsole, nullptr, 0},
    {"Advanced", nullptr, advancedMenu, advancedMenuSize},
    {"Exit Menu", actionExitMenu, nullptr, 0}
};
const int mainMenuSize = sizeof(mainMenu) / sizeof(MenuItem);

void MenuSystem::exitHidConfigMode() {
    Serial.println("MENU_SYS: exitHidConfigMode() - Start");
    if (dynamicProvider) {
        delete dynamicProvider;
        dynamicProvider = nullptr;
    }
    // Directly set main menu instead of calling backToMain to avoid recursion
    setCurrentMenu(mainMenu, mainMenuSize);
    selectedIndex = 0;
    menuStackDepth = 0;
    DATA_MGR.updateStatus("Main Menu");
    Serial.println("MENU_SYS: exitHidConfigMode() - Complete");
    menuLog("Exited HID config mode", 3);
}

void MenuSystem::enterHidConfigMode() {
    if (dynamicProvider) {
        delete dynamicProvider;
    }
    dynamicProvider = new HidConfigMenuProvider();
    inMenuMode = true;
    menuLog("Entered HID config mode", 3);
}

void MenuSystem::startDeviceConfiguration() {
    inDeviceConfigMode = true;
    enterHidConfigMode();
    menuLog("Started device configuration sequence", 3);
}



void actionShowUptime() {
    MENU_SYS.setDisplayMode(false);
    unsigned long uptime = millis() / 1000; // Convert to seconds
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    unsigned long seconds = uptime % 60;
    
    char status[32];
    snprintf(status, sizeof(status), "Uptime: %02lu:%02lu:%02lu", hours, minutes, seconds);
    DATA_MGR.updateStatus(status);
    menuLog("Showing uptime: " + String(status), 3);
}

void actionShowLastSender() {
    MENU_SYS.setDisplayMode(false);
    const uint8_t* lastSender = DATA_MGR.getLastSenderMAC();
    if (lastSender) {
        String macStr = macToString(lastSender);
        DATA_MGR.updateStatus("Last: " + macStr);
        menuLog("Last sender: " + macStr, 3);
    } else {
        DATA_MGR.updateStatus("No sender yet");
        menuLog("No sender yet", 3);
    }
}

void MenuSystem::enterBitIndexConfigMode() {
    if (dynamicProvider) {
        delete dynamicProvider;
    }
    dynamicProvider = new BitIndexConfigMenuProvider();
    inMenuMode = true;
    menuLog("Entered bit index config mode", 3);
}

void MenuSystem::exitBitIndexConfigMode() {
    if (dynamicProvider) {
        delete dynamicProvider;
        dynamicProvider = nullptr;
    }
    // Directly set main menu instead of calling backToMain to avoid recursion
    setCurrentMenu(mainMenu, mainMenuSize);
    selectedIndex = 0;
    menuStackDepth = 0;
    DATA_MGR.updateStatus("Main Menu");
    menuLog("Exited bit index config mode", 3);
}

void MenuSystem::completeDeviceConfiguration() {
    inDeviceConfigMode = false;
    DATA_MGR.updateStatus("Device configured");
    menuLog("Device configuration completed", 3);
}

void MenuSystem::proceedToBitIndexConfig() {
    if (dynamicProvider) {
        delete dynamicProvider;
        dynamicProvider = nullptr;
    }
    enterBitIndexConfigMode();
    menuLog("Proceeding to bit index configuration", 3);
}

// Action functions that were missing
void actionSendBroadcast() {
    MENU_SYS.setDisplayMode(false);
    
    if (!DATA_MGR.isHIDConfigured()) {
        DATA_MGR.updateStatus("HID not configured");
        menuLog("Cannot send broadcast - HID not configured", 2);
        return;
    }
    
    // Send the legacy broadcast test
    espnowSendBroadcastTest();
    
    // Also send a tree network test message if configured
    if (DATA_MGR.isBitIndexConfigured()) {
        // Send a test command to all devices
        uint8_t testPayload[4] = {0xDE, 0xAD, 0xBE, 0xEF};
        bool success = sendTreeCommand(0xFFFF, MSG_COMMAND_SET_OUTPUTS, testPayload, sizeof(testPayload));
        
        if (success) {
            DATA_MGR.updateStatus("Broadcast + Tree sent");
            menuLog("Legacy broadcast + tree command sent", 3);
        } else {
            DATA_MGR.updateStatus("Tree command failed");
            menuLog("Tree command failed to send", 2);
        }
    } else {
        DATA_MGR.updateStatus("Legacy broadcast sent");
        menuLog("Legacy broadcast test sent", 3);
    }
}

void actionToggleContinuousBroadcast() {
    MENU_SYS.setDisplayMode(false);
    static bool enabled = false;
    enabled = !enabled;
    setContinuousBroadcast(enabled);
    DATA_MGR.updateStatus(enabled ? "Continuous ON" : "Continuous OFF");
    menuLog("Continuous broadcast: " + String(enabled ? "ON" : "OFF"), 3);
}

void actionShowHIDInfo() {
    MENU_SYS.setDisplayMode(false);
    if (DATA_MGR.isHIDConfigured()) {
        DATA_MGR.updateStatus("HID: " + String(DATA_MGR.getMyHID()));
    } else {
        DATA_MGR.updateStatus("HID not set");
    }
    menuLog("Showing HID info", 3);
}

void actionConfigureHID() {
    MENU_SYS.enterHidConfigMode();
    menuLog("Starting HID configuration", 3);
}

void actionClearHID() {
    MENU_SYS.setDisplayMode(false);
    DATA_MGR.clearHIDFromNVM();
    DATA_MGR.updateStatus("HID cleared");
    menuLog("HID configuration cleared", 3);
}

void actionShowTreeStats() {
    MENU_SYS.setDisplayMode(false);
    const NetworkStats& stats = DATA_MGR.getNetworkStats();
    DATA_MGR.updateStatus("Sent:" + String(stats.messagesSent) + " Rx:" + String(stats.messagesReceived));
    menuLog("Showing tree network stats", 3);
}

void actionToggleAutoReporting() {
    MENU_SYS.setDisplayMode(false);
    static bool enabled = false;
    enabled = !enabled;
    DATA_MGR.updateStatus(enabled ? "Auto Report ON" : "Auto Report OFF");
    menuLog("Auto reporting: " + String(enabled ? "ON" : "OFF"), 3);
}

void actionConfigureBitIndex() {
    MENU_SYS.enterBitIndexConfigMode();
    menuLog("Starting bit index configuration", 3);
}

void actionClearBitIndex() {
    MENU_SYS.setDisplayMode(false);
    DATA_MGR.clearBitIndexFromNVM();
    DATA_MGR.updateStatus("Bit index cleared");
    menuLog("Bit index configuration cleared", 3);
}

void actionToggleDebug() {
    MENU_SYS.setDisplayMode(false);
    static bool enabled = false;
    enabled = !enabled;
    DATA_MGR.updateStatus(enabled ? "Debug ON" : "Debug OFF");
    menuLog("Debug mode: " + String(enabled ? "ON" : "OFF"), 3);
}

void actionShowLRStatus() {
    MENU_SYS.setDisplayMode(false);
    bool isEnabled = isLongRangeModeEnabled();
    DATA_MGR.updateStatus(isEnabled ? "LR Mode ON" : "LR Mode OFF");
    menuLog("Long Range status: " + String(isEnabled ? "ON" : "OFF"), 3);
}

void actionResetStats() {
    MENU_SYS.setDisplayMode(false);
    DATA_MGR.resetNetworkStats();
    DATA_MGR.updateStatus("Stats reset");
    menuLog("Network statistics reset", 3);
}

void actionShowStats() {
    MENU_SYS.setDisplayMode(false);
    const NetworkStats& stats = DATA_MGR.getNetworkStats();
    DATA_MGR.updateStatus("TX:" + String(stats.messagesSent) + " RX:" + String(stats.messagesReceived));
    menuLog("Showing statistics", 3);
}

void actionShowRSSI() {
    MENU_SYS.setDisplayMode(false);
    const NetworkStats& stats = DATA_MGR.getNetworkStats();
    DATA_MGR.updateStatus("RSSI: " + String(stats.signalStrength) + "dBm");
    menuLog("Showing RSSI", 3);
}

void actionDisplayInfo() {
    MENU_SYS.setDisplayMode(false);
    DATA_MGR.updateStatus("Display Info");
    menuLog("Showing display info", 3);
}

void actionShowNodeInfo() {
    MENU_SYS.setDisplayMode(false);
    uint8_t mac[6];
    DATA_MGR.getNodeMAC(mac);
    String macStr = macToString(mac);
    DATA_MGR.updateStatus("Node: " + macStr.substring(12));
    menuLog("Showing node info", 3);
}

void actionBackToMain() {
    MENU_SYS.backToMain();
    menuLog("Returned to main menu", 3);
}

 