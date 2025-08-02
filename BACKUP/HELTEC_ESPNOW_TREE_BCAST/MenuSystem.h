#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include <Arduino.h>
#include <functional>
#include "TreeNetwork.h"
#include "debug.h"

// ============================================================================
// DYNAMIC MENU PROVIDER INTERFACE
// ============================================================================

class IDynamicMenuProvider {
public:
    virtual ~IDynamicMenuProvider() = default;
    virtual int getItemCount() = 0;
    virtual const char* getItemText(int index) = 0;
    virtual void selectItem(int index) = 0;
    virtual void back() = 0;
};

class HidConfigMenuProvider : public IDynamicMenuProvider {
private:
    uint32_t currentHid;
    int hidConfigDepth;
    static char textBuffers[6][20]; // Static buffers to avoid stack overflow

public:
    HidConfigMenuProvider();

    int getItemCount() override {
        if (hidConfigDepth == 4) return 3; // Set, Up, Back
        if (hidConfigDepth == 1) return 6; // Set, 4 children, Back
        return 7; // Set, Up, 4 children, Back
    }

    const char* getItemText(int index) override;
    void selectItem(int index) override;
    void back() override;
};

class BitIndexConfigMenuProvider : public IDynamicMenuProvider {
private:
    uint8_t currentPage;  // 0-3 for pages 0-7, 8-15, 16-23, 24-31
    static char textBuffers[10][25]; // Static buffers to avoid stack overflow
    
    static const uint8_t BITS_PER_PAGE = 8;
    static const uint8_t MAX_PAGES = 4; // 32 bits / 8 bits per page = 4 pages

public:
    BitIndexConfigMenuProvider();

    int getItemCount() override;
    const char* getItemText(int index) override;
    void selectItem(int index) override;
    void back() override;
    
private:
    uint8_t getPageStartBit() const { return currentPage * BITS_PER_PAGE; }
    uint8_t getPageEndBit() const { return getPageStartBit() + BITS_PER_PAGE - 1; }
    bool hasPrevPage() const { return currentPage > 0; }
    bool hasNextPage() const { return currentPage < (MAX_PAGES - 1); }
};


// ============================================================================
// CONSOLE MESSAGE FUNCTIONS
// ============================================================================

// Monitor shared data for button press/release events
void consoleLogSharedDataChange(uint32_t oldSharedData, uint32_t newSharedData);

// ============================================================================
// CONSOLE DISPLAY SYSTEM
// ============================================================================

// Console message buffer for data flow messages
struct ConsoleMessage {
    String message;
    unsigned long timestamp;
};

class ConsoleDisplay {
private:
    static const int MAX_MESSAGES = 20;
    ConsoleMessage messages[MAX_MESSAGES];
    int messageCount;
    int oldestIndex;
    
public:
    ConsoleDisplay();
    void addMessage(const String& msg);
    void clear();
    int getMessageCount() const { return messageCount; }
    const ConsoleMessage& getMessage(int index) const;
};

// ============================================================================
// MENU SYSTEM STRUCTURES
// ============================================================================

struct MenuItem {
    const char* text;
    std::function<void()> action;
    const MenuItem* submenu;
    int submenuSize;
};

// ============================================================================
// MENU NAVIGATION AND DISPLAY
// ============================================================================

/**
 * @brief Menu system management class
 */
class MenuSystem {
public:
    // Singleton pattern
    static MenuSystem& getInstance();
    
    // ========================================================================
    // INITIALIZATION
    // ========================================================================
    void initialize();
    
    // ========================================================================
    // MENU NAVIGATION
    // ========================================================================
    void navigateUp();
    void navigateDown();
    void selectCurrentItem();
    void backToMain();
    void backToPreviousMenu();
    
    // ========================================================================
    // MENU STATE
    // ========================================================================
    void setCurrentMenu(const MenuItem* menu, int menuSize);
    const MenuItem* getCurrentMenu() const { return currentMenu; }
    int getCurrentMenuSize() const;
    int getSelectedIndex() const { return selectedIndex; }
    const char* getCurrentItemText() const;
    const char* getCurrentItemText(int index) const;
    
    // ========================================================================
    // DISPLAY UPDATE
    // ========================================================================
    void updateDisplay();
    void showStatusDisplay();
    void showMenuDisplay();
    void showConsoleDisplay();  // New console display method
    void setDisplayMode(bool menuMode);
    void setConsoleMode(bool consoleMode);  // New console mode setter
    bool isInMenuMode() const { return inMenuMode; }
    bool isInConsoleMode() const { return inConsoleMode; }  // New console mode getter

    // Public interface for HID configuration
    void enterHidConfigMode();
    void exitHidConfigMode();
    
    // Public interface for Bit Index configuration
    void enterBitIndexConfigMode();
    void exitBitIndexConfigMode();
    
    // Public interface for Device configuration (sequential HID + Bit Index)
    void startDeviceConfiguration();
    void completeDeviceConfiguration();
    bool isInDeviceConfigMode() const { return inDeviceConfigMode; }
    
    // Internal helper for sequential configuration
    void proceedToBitIndexConfig();
    
    // Console message management
    void addConsoleMessage(const String& msg);
    void clearConsoleMessages();

private:
    MenuSystem();
    ~MenuSystem() = default;
    MenuSystem(const MenuSystem&) = delete;
    MenuSystem& operator=(const MenuSystem&) = delete;
    
    // Menu state
    const MenuItem* currentMenu;
    int currentMenuSize;
    int selectedIndex;
    
    // Menu navigation stack for proper back navigation
    static const int MAX_MENU_DEPTH = 4;
    struct MenuLevel {
        const MenuItem* menu;
        int menuSize;
        int selectedIndex;
    };
    MenuLevel menuStack[MAX_MENU_DEPTH];
    int menuStackDepth;
    
    // Display helpers
    void drawStatusOLED();
    void drawMenuOLED();
    void displaySerial();
    
    // Display mode
    bool inMenuMode;
    bool inConsoleMode;  // New console display mode
    
    // Device configuration state
    bool inDeviceConfigMode;
    
    // Console display
    ConsoleDisplay consoleDisplay;
    
    // Status display helpers
    void displayStatusOLED();
    void displayStatusSerial();
    void drawConsoleOLED();  // New console display method
    void displayConsoleSerial();  // Console serial display method

    // Dynamic menu provider
    IDynamicMenuProvider* dynamicProvider;
};

// ============================================================================
// MENU ACTION FUNCTIONS
// ============================================================================

// Legacy ESP-NOW actions
void actionSendBroadcast();
void actionToggleContinuousBroadcast();

// Tree Network actions
void actionShowHIDInfo();
void actionConfigureHID();
void actionClearHID();
void actionSendDataReport();
void actionShowTreeStats();
void actionToggleAutoReporting();

// Device Configuration actions
void actionConfigureBitIndex();
void actionConfigureDevice();
void actionClearBitIndex();
void actionClearAllConfig();
void actionShowDeviceStatus();

// Settings actions
void actionToggleDebug();
void actionToggleLongRange();
void actionShowLRStatus();
void actionResetStats();

// Info actions
void actionShowStats();
void actionShowRSSI();
void actionShowLastSender();
void actionDisplayInfo();
void actionShowNodeInfo();
void actionShowUptime();
void actionShowNetworkInfo();

// Navigation actions
void actionBackToMain();
void actionBackToPrevious();
void actionExitMenu();
void actionToggleConsole();

// ============================================================================
// MENU STRUCTURES
// ============================================================================

extern const MenuItem mainMenu[];
extern const int mainMenuSize;

extern const MenuItem treeNetworkMenu[];
extern const int treeNetworkMenuSize;

extern const MenuItem settingsMenu[];
extern const int settingsMenuSize;

extern const MenuItem infoMenu[];
extern const int infoMenuSize;

extern const MenuItem advancedMenu[];
extern const int advancedMenuSize;

extern const MenuItem debugMenu[];
extern const int debugMenuSize;

// Global access macro
#define MENU_SYS MenuSystem::getInstance()

void actionSendTestCommand();
void actionShowTreeStats();
void actionToggleAutoReporting();
void actionShowAggregatedDevices();
void actionClearAggregatedData();

// I/O Device actions
void actionShowIOStatus();
void actionShowPinConfig();
void actionToggleIOAutoReport();
void actionSetSharedData();
void actionTestOutputs();

#endif // MENUSYSTEM_H 