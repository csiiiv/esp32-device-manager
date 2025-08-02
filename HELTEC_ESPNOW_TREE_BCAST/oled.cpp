#include "oled.h"
#include <Arduino.h>

// Logging macros for the OLED module
#define MODULE_TITLE       "OLED"
#define MODULE_DEBUG_LEVEL 3
#define oledLog(msg,lvl) debugPrint(msg, MODULE_TITLE, lvl, MODULE_DEBUG_LEVEL)

#if ENABLE_OLED
// Create a global U8G2 display object

/*
U8G2_R0	No rotation, landscape
U8G2_R1	90 degree clockwise rotation
U8G2_R2	180 degree clockwise rotation
U8G2_R3	270 degree clockwise rotation
U8G2_MIRROR	No rotation, landscape, display content is mirrored (v2.6.x)
U8G2_MIRROR_VERTICAL	Display content is vertically mirrored (v2.29.x)

*/

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(
    U8G2_R0,
    /* clock=*/SCL_OLED, /* data=*/SDA_OLED, /* reset=*/RST_OLED);

void VextON(){
    // If your board uses a transistor to power the display externally:
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, LOW);
    oledLog("VextON => external power ON",3); // INFO
}

void VextOFF(){
    pinMode(VEXT_PIN,OUTPUT);
    digitalWrite(VEXT_PIN,HIGH);
    oledLog("VextOFF => external power OFF",3); // INFO
}

void setupDisplay(){
    oledLog("setupDisplay => initializing OLED",3); // INFO
    VextON(); // if you do external power
    display.begin();
    display.setFont(u8g2_font_ncenB08_tr);
    display.setCursor(0,0);
    oledLog("OLED display ready",3); // INFO
}

bool isOLEDEnabled() {
    return true;
}

#else
// OLED disabled - provide stub functions

void VextON(){
    oledLog("VextON => OLED disabled, no-op",4); // DEBUG
}

void VextOFF(){
    oledLog("VextOFF => OLED disabled, no-op",4); // DEBUG
}

void setupDisplay(){
    oledLog("setupDisplay => OLED disabled, no-op",3); // INFO
}

bool isOLEDEnabled() {
    return false;
}

#endif // ENABLE_OLED
