#include <RadioLib.h>
#include "heltec.h"

// =================== Dynamic Timing Definitions ===================
const unsigned long TX_TIME_MS      = 2800;               // Known transmission time (ms)
const unsigned long ACK_TIMEOUT_MS  = TX_TIME_MS + 500;     // e.g., 3300 ms
const int           BACKOFF_MIN_MS  = TX_TIME_MS / 2;         // e.g., 1400 ms
const int           BACKOFF_MAX_MS  = TX_TIME_MS;             // e.g., 2800 ms

const unsigned long SCAN_TIMEOUT_MS         = 10000; // 10 sec timeout for channel scan
const unsigned long BROADCAST_COOLDOWN_MS   = 1000;  // 1 sec cooldown for master broadcasts

const int MAX_RETRIES = 3;  // Maximum number of retransmissions

// =================== Global Variables ===================
// (Assume volatile uint16_t stateBitmap is declared in heltec.h)

volatile bool operationDone = false;  // Set by radio tx/rx interrupt
volatile bool scanFlag      = false;  // Set by channel scan interrupt
bool buttonPressed          = false;  // For master manual refresh

#if !IS_MASTER
// Slave-specific globals
bool lastButtonState = false;  // Local copy of desired button state
bool pendingAck      = false;  // True when waiting for master confirmation
unsigned long ackStartTime = 0;
String pendingMessage = "";    // The message pending to be resent on timeout
int currentRetries   = 0;
#else
// Master-specific global for throttling
unsigned long lastBroadcastTime = 0;
#endif

// =================== Helper Functions ===================

// Create BTN message: "BTN:<deviceID>:<state>"
String makeBtnMessage(uint8_t id, uint8_t state) {
  return "BTN:" + String(id) + ":" + String(state);
}

// Create STATE message: "STATE:<hex>" where hex is 4-digit uppercase hex.
String makeStateMessage(uint16_t state) {
  char hexStr[5];
  sprintf(hexStr, "%04X", state);
  return "STATE:" + String(hexStr);
}

// =================== Callback Functions ===================
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR void setFlag() { operationDone = true; }
ICACHE_RAM_ATTR void scanCallback() { scanFlag = true; }
#else
void setFlag() { operationDone = true; }
void scanCallback() { scanFlag = true; }
#endif

// =================== Debug Output ===================
void debugPrint(const String &msg) {
  Serial.println("DEBUG [" + String(millis()) + "]: " + msg);
}

// =================== Radio Utility Functions ===================

// Wait for a clear channel (returns true even if timeout) and resumes RX.
bool waitForClearChannel() {
  debugPrint("Starting channel scan");
  consoleWrite("Scan chan");
  unsigned long startTime = millis();
  radio.setDio1Action(scanCallback);
  
  while (millis() - startTime < SCAN_TIMEOUT_MS) {
    scanFlag = false;
    int ret = radio.startChannelScan();
    if (ret != RADIOLIB_ERR_NONE) {
      debugPrint("Scan init error: " + String(ret));
      consoleWrite("Scan err");
      delay(random(BACKOFF_MIN_MS, BACKOFF_MAX_MS));
      continue;
    }
    while (!scanFlag && (millis() - startTime < SCAN_TIMEOUT_MS))
      delay(10);
    int result = radio.getChannelScanResult();
    if (result == RADIOLIB_CHANNEL_FREE) {
      debugPrint("Channel free");
      consoleWrite("Chan free");
      break;
    } else {
      debugPrint("Channel busy, result: " + String(result));
      delay(random(BACKOFF_MIN_MS, BACKOFF_MAX_MS));
    }
  }
  radio.setDio1Action(setFlag);
  radio.startReceive();
  return true;
}

// Transmit a message string with retry logic.
void sendMessage(const String &msg) {
  int retries = 0;
  bool sent = false;
  
  while (retries < MAX_RETRIES && !sent) {
    operationDone = false;
    String msg_str = msg;
    radio.startTransmit(msg_str);
    unsigned long startTime = millis();
    while (!operationDone && (millis() - startTime < 5000))
      delay(10);
    
    if (operationDone) {
      sent = true;
      debugPrint("Message sent: " + msg);
      Serial.println("Message sent: " + msg);
      debugPrint("StateBitmap (binary): " + String(stateBitmap, BIN));
    } else {
      retries++;
      debugPrint("Transmit failed, retrying (" + String(retries) + ")");
      delay(random(BACKOFF_MIN_MS, BACKOFF_MAX_MS));
    }
  }
  if (!sent) {
    debugPrint("Failed to send message: " + msg);
    consoleWrite("Send fail");
  }
  radio.startReceive();
}

// =================== Packet Processing ===================
// Process an incoming message string.
void processIncomingMessage() {
  String inMsg = radio.readData();
  debugPrint("Received raw message: " + inMsg);
  // Expect either "BTN:<id>:<state>" or "STATE:<hex>"
  if (inMsg.startsWith("BTN:")) {
    // Parse slave button update.
    int firstColon = inMsg.indexOf(':');
    int secondColon = inMsg.indexOf(':', firstColon + 1);
    if (firstColon == -1 || secondColon == -1) {
      debugPrint("Malformed BTN message: " + inMsg);
      return;
    }
    int id = inMsg.substring(firstColon + 1, secondColon).toInt();
    int btnState = inMsg.substring(secondColon + 1).toInt();
#if IS_MASTER
    // Master updates its stateBitmap accordingly.
    if (btnState == 1)
      stateBitmap |= (1 << id);
    else
      stateBitmap &= ~(1 << id);
    debugPrint("Master updated stateBitmap: " + String(stateBitmap, BIN));
    
    // Broadcast collated state.
    if (millis() - lastBroadcastTime >= BROADCAST_COOLDOWN_MS) {
      String stateMsg = makeStateMessage(stateBitmap);
      lastBroadcastTime = millis();
      debugPrint("Master broadcasting state: " + stateMsg);
      waitForClearChannel();
      sendMessage(stateMsg);
    }
#endif
  } else if (inMsg.startsWith("STATE:")) {
    // Parse the state broadcast.
    String hexVal = inMsg.substring(6);
    uint16_t newState = (uint16_t)strtol(hexVal.c_str(), NULL, 16);
    stateBitmap = newState;
    debugPrint("Received state from master: " + inMsg + " | Binary: " + String(newState, BIN));
#if !IS_MASTER
    // For slaves, verify that our own bit matches our desired state.
    uint8_t recBit = (newState >> DEVICE_ID) & 0x01;
    uint8_t desired = lastButtonState ? 1 : 0;
    if (recBit != desired) {
      debugPrint("ACK mismatch on slave: desired " + String(desired) +
                 " vs received " + String(recBit) +
                 " | State: " + String(newState, BIN) +
                 "; will retry");
      // Leave pendingAck true so retry logic triggers.
    } else if (pendingAck) {
      pendingAck = false;
      currentRetries = 0;
      debugPrint("ACK confirmed on slave; StateBitmap: " + String(newState, BIN));
    }
#endif
  } else {
    debugPrint("Unknown message format: " + inMsg);
  }
  consoleWrite("Rx: " + inMsg);
  Serial.println("Received message: " + inMsg);
}

// =================== Button Handling ===================
bool isButtonPressed() {
  if (digitalRead(BUTTON) == LOW) {
    delay(50);
    return (digitalRead(BUTTON) == LOW);
  }
  return false;
}

void handleButtonPress() {
#if !IS_MASTER
  // Slave: on button state change, send a BTN update.
  bool current = isButtonPressed();
  if (current != lastButtonState) {
    lastButtonState = current;
    if (current)
      stateBitmap |= (1 << DEVICE_ID);
    else
      stateBitmap &= ~(1 << DEVICE_ID);
    String btnMsg = makeBtnMessage(DEVICE_ID, current ? 1 : 0);
    pendingMessage = btnMsg;
    pendingAck = true;
    ackStartTime = millis();
    currentRetries = 0;
    debugPrint("Slave: Button changed. Sending BTN message: " + btnMsg +
               " | New stateBitmap: " + String(stateBitmap, BIN));
    waitForClearChannel();
    sendMessage(btnMsg);
  }
#else
  // Master: manual refresh on button press.
  if (isButtonPressed() && !buttonPressed) {
    buttonPressed = true;
    debugPrint("Master button pressed: Manual refresh | StateBitmap: " + String(stateBitmap, BIN));
    String stateMsg = makeStateMessage(stateBitmap);
    waitForClearChannel();
    sendMessage(stateMsg);
  } else if (!isButtonPressed()) {
    buttonPressed = false;
  }
#endif
}

#if !IS_MASTER
void handleAckTimeout() {
  if (pendingAck && (millis() - ackStartTime >= ACK_TIMEOUT_MS)) {
    if (currentRetries < MAX_RETRIES) {
      currentRetries++;
      debugPrint("Slave: ACK timeout. Retrying BTN message: " + pendingMessage +
                 " | StateBitmap: " + String(stateBitmap, BIN));
      waitForClearChannel();
      sendMessage(pendingMessage);
      ackStartTime = millis();
    } else {
      debugPrint("Slave: Max retries reached. Giving up on BTN update.");
      consoleWrite("Retry fail");
      pendingAck = false;
    }
  }
}
#endif

// =================== Setup and Main Loop ===================
void initializeSystem() {
  Serial.begin(115200);
  delay(100);
  
  randomSeed(analogRead(0));
  
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  delay(100);
  
  pinMode(RST_OLED, OUTPUT);
  digitalWrite(RST_OLED, HIGH);
  delay(100);
  
  factory_display.init();
  factory_display.clear();
  factory_display.display();
  delay(300);
  
  consoleClearBuffer();
  consoleWrite("Start Test");
  debugPrint("Setup start");
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  pinMode(BUTTON, INPUT_PULLUP);
  
  bool ok = setup_heltec();
  if (ok) {
    debugPrint("Heltec OK");
    consoleWrite("Heltec OK");
  } else {
    debugPrint("Heltec FAIL");
    consoleWrite("Heltec FAIL");
  }
  
  Serial.println("========== Device Parameters ==========");
  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);
  Serial.print("Mode: ");
  Serial.println(IS_MASTER ? "Master" : "Slave");
  Serial.print("TX_TIME_MS: ");
  Serial.println(TX_TIME_MS);
  Serial.print("ACK_TIMEOUT_MS: ");
  Serial.println(ACK_TIMEOUT_MS);
  Serial.print("BACKOFF_MIN_MS: ");
  Serial.println(BACKOFF_MIN_MS);
  Serial.print("BACKOFF_MAX_MS: ");
  Serial.println(BACKOFF_MAX_MS);
  Serial.println("=======================================");
  
  consoleWrite("ID:" + String(DEVICE_ID) + " " + (IS_MASTER ? "MSTR" : "SLV"));
  
  radio.setDio1Action(setFlag);
  
  int ret = radio.startReceive();
  if (ret == RADIOLIB_ERR_NONE) {
    debugPrint("Radio RX mode activated");
    consoleWrite("RX mode");
  } else {
    debugPrint("RX error: " + String(ret));
    consoleWrite("RX err:" + String(ret));
    while (true) { delay(10); }
  }
}

void setup() {
  initializeSystem();
}

void loop() {
  attemptConsoleUpdate();
  handleButtonPress();
  
#if !IS_MASTER
  handleAckTimeout();
#endif
  
  if (operationDone) {
    operationDone = false;
    processIncomingMessage();
  }
}
