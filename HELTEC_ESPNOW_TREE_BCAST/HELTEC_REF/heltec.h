#ifndef HELTEC_H
#define HELTEC_H

#include <SPI.h>
#include "SSD1306Wire.h"

// ======================================================
// Device Identification
// ------------------------------------------------------
// Define the device ID. Master should be ID 0 and slaves 1..15.
// Change this definition for each device.

    // Set to 0 for master, 1–15 for slaves.
#define DEVICE_ID 2
#define IS_MASTER (DEVICE_ID == 0)
// ======================================================

#define BUTTON    GPIO_NUM_0
#define LED_PIN   GPIO_NUM_35

// External power control
#define VEXT      GPIO_NUM_36

// Battery voltage measurement
#define VBAT_CTRL GPIO_NUM_37
#define VBAT_ADC  GPIO_NUM_1

// SPI pins
#define SS        GPIO_NUM_8
#define MOSI      GPIO_NUM_10
#define MISO      GPIO_NUM_11
#define SCK       GPIO_NUM_9

// Radio pins
#define DIO1      GPIO_NUM_14
#define RST_LoRa  GPIO_NUM_12
#define BUSY_LoRa GPIO_NUM_13

// Display pins
#define SDA_OLED  GPIO_NUM_17
#define SCL_OLED  GPIO_NUM_18
#define RST_OLED  GPIO_NUM_21

#define RF_FREQUENCY          915.0
#define TX_OUTPUT_POWER       14
#define LORA_BANDWIDTH        125.0
#define LORA_SPREADING_FACTOR 12
#define LORA_CODINGRATE       8
#define LORA_PREAMBLE_LENGTH  16
#define LORA_SYNC_WORD  0x34

SPIClass* hspi = new SPIClass(HSPI);
SX1262 radio = new Module(SS, DIO1, RST_LoRa, BUSY_LoRa, *hspi);

SSD1306Wire factory_display(0x3c, SDA_OLED, SCL_OLED, GEOMETRY_128_64);

// ======================================================
// OLED CONSOLE: Use 5 lines; reserve the last line for the 16-bit state bitmap.
// ------------------------------------------------------
#define MAX_CONSOLE_LINES 5
#define MAX_CONSOLE_CHARS 22   // 20 characters + null terminator

// We'll store only 4 lines of console messages; the 5th line is reserved.
char consoleBuffer[MAX_CONSOLE_LINES - 1][MAX_CONSOLE_CHARS];
int consoleLineCount = 0;
bool needConsoleRefresh = false;

// Global state bitmap variable (each bit corresponds to a device ID)
volatile uint16_t stateBitmap = 0;

void consoleClearBuffer() {
  for (int i = 0; i < MAX_CONSOLE_LINES - 1; i++) {
    consoleBuffer[i][0] = '\0';
  }
  consoleLineCount = 0;
  needConsoleRefresh = true;
}

void consoleWrite(const String &msg) {
  char line[MAX_CONSOLE_CHARS];
  msg.toCharArray(line, MAX_CONSOLE_CHARS);  // Truncate if needed

  Serial.println("OLED: " + msg);

  if (consoleLineCount >= (MAX_CONSOLE_LINES - 1)) {
    for (int i = 1; i < (MAX_CONSOLE_LINES - 1); i++) {
      strncpy(consoleBuffer[i - 1], consoleBuffer[i], MAX_CONSOLE_CHARS);
      consoleBuffer[i - 1][MAX_CONSOLE_CHARS - 1] = '\0';
    }
    strncpy(consoleBuffer[(MAX_CONSOLE_LINES - 1) - 1], line, MAX_CONSOLE_CHARS - 1);
    consoleBuffer[(MAX_CONSOLE_LINES - 1) - 1][MAX_CONSOLE_CHARS - 1] = '\0';
  } else {
    strncpy(consoleBuffer[consoleLineCount], line, MAX_CONSOLE_CHARS - 1);
    consoleBuffer[consoleLineCount][MAX_CONSOLE_CHARS - 1] = '\0';
    consoleLineCount++;
  }
  needConsoleRefresh = true;
}

// Helper function to convert a 16-bit value to a binary string.
// The resulting string will be exactly 16 characters long.
void uint16ToBinaryString(uint16_t value, char *buffer, int bufferSize) {
  if (bufferSize < 17) return; // Need space for 16 chars + null terminator.
  for (int i = 15; i >= 0; i--) {
    buffer[15 - i] = (value & (1 << i)) ? '1' : '0';
  }
  buffer[16] = '\0';
}

void consoleUpdate() {
  if (!needConsoleRefresh) return;
  factory_display.clear();
  // Draw the buffered console lines on lines 0 to 3.
  for (int i = 0; i < (MAX_CONSOLE_LINES - 1); i++) {
    if (i < consoleLineCount) {
      factory_display.drawString(0, i * 10, consoleBuffer[i]);
    }
  }
  // Reserve the last line for the binary state bitmap.
  // Format: "BM:1010101010101010" (total 19 characters)
  char binaryStr[17];
  uint16ToBinaryString(stateBitmap, binaryStr, sizeof(binaryStr));
  char stateLine[MAX_CONSOLE_CHARS];
  snprintf(stateLine, MAX_CONSOLE_CHARS, "BM:%s", binaryStr);
  factory_display.drawString(0, (MAX_CONSOLE_LINES - 1) * 10, stateLine);
  factory_display.display();
  needConsoleRefresh = false;
}

unsigned long lastConsoleUpdate = 0;
const unsigned long UPDATE_INTERVAL_MS = 100;

void attemptConsoleUpdate() {
  unsigned long now = millis();
  if ((now - lastConsoleUpdate >= UPDATE_INTERVAL_MS) && needConsoleRefresh) {
    consoleUpdate();
    lastConsoleUpdate = now;
  }
}

// ======================================================
// Original Radio and Hardware Initialization Code
// ------------------------------------------------------
bool setup_heltec() {
  hspi->begin(SCK, MISO, MOSI, SS);

  Serial.print(F("[SX1262] Initializing ... "));
  bool state = radio.begin(RF_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                           LORA_CODINGRATE, LORA_SYNC_WORD, TX_OUTPUT_POWER,
                           LORA_PREAMBLE_LENGTH);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.print(F("success! code: "));
    Serial.println(state);
    state = true;
  } else {
    Serial.print(F("failed, code: "));
    Serial.println(state);
    while (true) { delay(10); }
  }

  if (radio.setFrequency(RF_FREQUENCY) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setBandwidth(LORA_BANDWIDTH) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setSpreadingFactor(LORA_SPREADING_FACTOR) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setCodingRate(LORA_CODINGRATE) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setSyncWord(0xAB) != RADIOLIB_ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
    while (true) { delay(10); }
  }

  if (radio.setOutputPower(TX_OUTPUT_POWER) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setCurrentLimit(80) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Selected current limit is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setPreambleLength(LORA_PREAMBLE_LENGTH) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
    Serial.println(F("Selected CRC is invalid for this module!"));
    while (true) { delay(10); }
  }

  if (radio.setTCXO(2.4) == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE) {
    Serial.println(F("Selected TCXO voltage is invalid for this module!"));
    while (true) { delay(10); }
  }

  Serial.println(F("All settings succesfully changed!"));
  return state;
}

#endif
