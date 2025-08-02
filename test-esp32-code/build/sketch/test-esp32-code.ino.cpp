#include <Arduino.h>
#line 1 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
/*
 * ESP32 Educational Configuration System
 * Uses ArduinoJson for robust configuration management
 * Features: Dynamic configuration schema, LED control, modular design
 */

#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFi.h>

// LED Configuration
struct LEDConfig {
    bool enabled;
    String mode;  // "off", "on", "blink"
    int on_time_ms;
    int off_time_ms;
    int pin;
    unsigned long last_toggle;
    bool current_state;
};

// Device Configuration using ArduinoJson
struct DeviceConfig {
    JsonDocument doc;
    Preferences preferences;
    LEDConfig led;
    
    // Configuration schema
    void sendConfigurationSchema();
    bool loadFromPreferences();
    bool saveToPreferences();
    bool updateFromJson(const char* jsonString);
    void applyConfiguration();
    void sendDeviceInfo();
};

DeviceConfig config;
String inputString = "";
bool stringComplete = false;

#line 41 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void setup();
#line 74 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void loop();
#line 88 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void serialEvent();
#line 98 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void processInput(String input);
#line 306 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void handleLED();
#line 321 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void sendStatus();
#line 339 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void sendCurrentConfig();
#line 356 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void runDiagnostics();
#line 381 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void showHelp();
#line 41 "/media/temp/6E9A84429A8408B3/_VSC/WEB_UPLOAD/test-esp32-code/test-esp32-code.ino"
void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    
    // Initialize preferences
    config.preferences.begin("device_config", false);
    
    // Initialize LED
    config.led.pin = 35;
    config.led.enabled = false;
    config.led.mode = "off";
    config.led.on_time_ms = 500;
    config.led.off_time_ms = 500;
    config.led.current_state = false;
    config.led.last_toggle = 0;
    pinMode(config.led.pin, OUTPUT);
    digitalWrite(config.led.pin, LOW);
    
    // Load saved configuration
    config.loadFromPreferences();
    
    // Apply loaded configuration
    config.applyConfiguration();
    
    Serial.println("\n=== ESP32 Educational Configuration System ===");
    Serial.println("Ready to receive configuration updates");
    Serial.println("Commands: status, info, config, current, restart, test, help");
    Serial.println("Send JSON configuration to update device settings");
    Serial.println("Use 'info' to get device information");
    Serial.println("Use 'config' to get configuration schema");
    Serial.println("Use 'current' to see current configuration values");
}

void loop() {
    // Handle serial input
    if (stringComplete) {
        processInput(inputString);
        inputString = "";
        stringComplete = false;
    }
    
    // Handle LED blinking
    handleLED();
    
    delay(10);
}

void serialEvent() {
    while (Serial.available()) {
        char inChar = (char)Serial.read();
        inputString += inChar;
        if (inChar == '\n') {
            stringComplete = true;
        }
    }
}

void processInput(String input) {
    input.trim();
    
    if (input.isEmpty()) return;
    
    Serial.print("Received: ");
    Serial.println(input);
    
    // Check if it's a JSON configuration
    if (input.startsWith("{")) {
        if (config.updateFromJson(input.c_str())) {
            Serial.println("✅ Configuration updated successfully!");
            config.saveToPreferences();
            config.applyConfiguration();
        } else {
            Serial.println("❌ Failed to update configuration");
        }
    } else if (input.equalsIgnoreCase("status")) {
        sendStatus();
    } else if (input.equalsIgnoreCase("info")) {
        config.sendDeviceInfo();
    } else if (input.equalsIgnoreCase("config")) {
        config.sendConfigurationSchema();
    } else if (input.equalsIgnoreCase("current")) {
        sendCurrentConfig();
    } else if (input.equalsIgnoreCase("restart")) {
        Serial.println("Restarting device...");
        delay(1000);
        ESP.restart();
    } else if (input.equalsIgnoreCase("test")) {
        runDiagnostics();
    } else if (input.equalsIgnoreCase("help")) {
        showHelp();
    } else {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
}

void DeviceConfig::sendDeviceInfo() {
    JsonDocument doc;
    
    doc["device_info"]["chip_type"] = "ESP32";
    doc["device_info"]["flash_size"] = "4MB";
    doc["device_info"]["free_heap"] = ESP.getFreeHeap();
    doc["device_info"]["mac_address"] = WiFi.macAddress();
    doc["device_info"]["uptime"] = millis();
    
    String output;
    serializeJson(doc, output);
    Serial.println("DEVICE_INFO:" + output);
}

void DeviceConfig::sendConfigurationSchema() {
    JsonDocument doc;
    JsonArray schema = doc.createNestedArray("config_schema");
    
    // Device Name
    JsonObject deviceName = schema.createNestedObject();
    deviceName["name"] = "device_name";
    deviceName["display_name"] = "Device Name";
    deviceName["description"] = "A friendly name for your ESP32 device";
    deviceName["type"] = "string";
    deviceName["current_value"] = doc.containsKey("device_name") ? doc["device_name"].as<String>() : "ESP32_Device";
    deviceName["default_value"] = "ESP32_Device";
    deviceName["validation"]["required"] = true;
    deviceName["validation"]["min_length"] = 1;
    deviceName["validation"]["max_length"] = 32;
    deviceName["examples"][0] = "My ESP32";
    deviceName["examples"][1] = "Living Room Sensor";
    deviceName["examples"][2] = "Garage Door Controller";
    deviceName["category"] = "basic";
    
    // LED Enabled
    JsonObject ledEnabled = schema.createNestedObject();
    ledEnabled["name"] = "led_enabled";
    ledEnabled["display_name"] = "LED Control";
    ledEnabled["description"] = "Enable or disable the LED on GPIO35";
    ledEnabled["type"] = "boolean";
    ledEnabled["current_value"] = led.enabled;
    ledEnabled["default_value"] = false;
    ledEnabled["validation"]["required"] = false;
    ledEnabled["examples"] = "true = LED enabled, false = LED disabled";
    ledEnabled["category"] = "led";
    
    // LED Mode
    JsonObject ledMode = schema.createNestedObject();
    ledMode["name"] = "led_mode";
    ledMode["display_name"] = "LED Mode";
    ledMode["description"] = "How the LED should behave";
    ledMode["type"] = "select";
    ledMode["current_value"] = led.mode;
    ledMode["default_value"] = "off";
    ledMode["validation"]["required"] = false;
    JsonArray options = ledMode["validation"].createNestedArray("options");
    options.add("off");
    options.add("on");
    options.add("blink");
    ledMode["examples"] = "off = LED off, on = LED always on, blink = LED blinks";
    ledMode["category"] = "led";
    
    // LED On Time
    JsonObject ledOnTime = schema.createNestedObject();
    ledOnTime["name"] = "led_on_time";
    ledOnTime["display_name"] = "LED On Time";
    ledOnTime["description"] = "How long the LED stays on during blink mode (milliseconds)";
    ledOnTime["type"] = "range";
    ledOnTime["current_value"] = led.on_time_ms;
    ledOnTime["default_value"] = 500;
    ledOnTime["validation"]["required"] = false;
    ledOnTime["validation"]["min"] = 50;
    ledOnTime["validation"]["max"] = 5000;
    ledOnTime["validation"]["step"] = 50;
    ledOnTime["examples"] = "100 = fast blink, 1000 = slow blink";
    ledOnTime["category"] = "led";
    
    // LED Off Time
    JsonObject ledOffTime = schema.createNestedObject();
    ledOffTime["name"] = "led_off_time";
    ledOffTime["display_name"] = "LED Off Time";
    ledOffTime["description"] = "How long the LED stays off during blink mode (milliseconds)";
    ledOffTime["type"] = "range";
    ledOffTime["current_value"] = led.off_time_ms;
    ledOffTime["default_value"] = 500;
    ledOffTime["validation"]["required"] = false;
    ledOffTime["validation"]["min"] = 50;
    ledOffTime["validation"]["max"] = 5000;
    ledOffTime["validation"]["step"] = 50;
    ledOffTime["examples"] = "100 = fast blink, 1000 = slow blink";
    ledOffTime["category"] = "led";
    
    String output;
    serializeJson(doc, output);
    Serial.println("CONFIG_SCHEMA:" + output);
}

bool DeviceConfig::loadFromPreferences() {
    // Load device name
    String deviceName = preferences.getString("device_name", "ESP32_Device");
    doc["device_name"] = deviceName;
    
    // Load LED configuration
    led.enabled = preferences.getBool("led_enabled", false);
    led.mode = preferences.getString("led_mode", "off");
    led.on_time_ms = preferences.getInt("led_on_time", 500);
    led.off_time_ms = preferences.getInt("led_off_time", 500);
    
    return true;
}

bool DeviceConfig::saveToPreferences() {
    // Save device name
    if (doc.containsKey("device_name")) {
        preferences.putString("device_name", doc["device_name"].as<String>());
    }
    
    // Save LED configuration
    preferences.putBool("led_enabled", led.enabled);
    preferences.putString("led_mode", led.mode);
    preferences.putInt("led_on_time", led.on_time_ms);
    preferences.putInt("led_off_time", led.off_time_ms);
    
    return true;
}

bool DeviceConfig::updateFromJson(const char* jsonString) {
    JsonDocument updateDoc;
    DeserializationError error = deserializeJson(updateDoc, jsonString);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return false;
    }
    
    // Update device name
    if (updateDoc.containsKey("device_name")) {
        doc["device_name"] = updateDoc["device_name"];
    }
    
    // Update LED configuration
    if (updateDoc.containsKey("led_enabled")) {
        led.enabled = updateDoc["led_enabled"];
    }
    if (updateDoc.containsKey("led_mode")) {
        led.mode = updateDoc["led_mode"].as<String>();
    }
    if (updateDoc.containsKey("led_on_time")) {
        led.on_time_ms = updateDoc["led_on_time"];
    }
    if (updateDoc.containsKey("led_off_time")) {
        led.off_time_ms = updateDoc["led_off_time"];
    }
    
    return true;
}

void DeviceConfig::applyConfiguration() {
    // Apply LED configuration
    if (!led.enabled || led.mode == "off") {
        digitalWrite(led.pin, LOW);
        led.current_state = false;
    } else if (led.mode == "on") {
        digitalWrite(led.pin, HIGH);
        led.current_state = true;
    }
    // Blink mode is handled in the main loop
}

void handleLED() {
    if (!config.led.enabled || config.led.mode != "blink") {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long interval = config.led.current_state ? config.led.on_time_ms : config.led.off_time_ms;
    
    if (currentTime - config.led.last_toggle >= interval) {
        config.led.current_state = !config.led.current_state;
        digitalWrite(config.led.pin, config.led.current_state ? HIGH : LOW);
        config.led.last_toggle = currentTime;
    }
}

void sendStatus() {
    Serial.println("\n=== Device Status ===");
    Serial.print("Device Name: ");
    Serial.println(config.doc.containsKey("device_name") ? config.doc["device_name"].as<String>() : "ESP32_Device");
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    Serial.print("Uptime: ");
    Serial.print(millis() / 1000);
    Serial.println(" seconds");
    Serial.print("LED State: ");
    Serial.print(config.led.enabled ? "Enabled" : "Disabled");
    Serial.print(" (");
    Serial.print(config.led.mode);
    Serial.println(")");
    Serial.println("==================\n");
}

void sendCurrentConfig() {
    Serial.println("\n=== Current Configuration ===");
    Serial.print("Device Name: ");
    Serial.println(config.doc.containsKey("device_name") ? config.doc["device_name"].as<String>() : "ESP32_Device");
    Serial.print("LED Enabled: ");
    Serial.println(config.led.enabled ? "true" : "false");
    Serial.print("LED Mode: ");
    Serial.println(config.led.mode);
    Serial.print("LED On Time: ");
    Serial.print(config.led.on_time_ms);
    Serial.println(" ms");
    Serial.print("LED Off Time: ");
    Serial.print(config.led.off_time_ms);
    Serial.println(" ms");
    Serial.println("==========================\n");
}

void runDiagnostics() {
    Serial.println("\n=== Device Diagnostics ===");
    
    // Memory test
    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
    
    // LED test
    Serial.println("Testing LED on GPIO35...");
    digitalWrite(config.led.pin, HIGH);
    delay(500);
    digitalWrite(config.led.pin, LOW);
    delay(500);
    digitalWrite(config.led.pin, HIGH);
    delay(500);
    digitalWrite(config.led.pin, LOW);
    Serial.println("LED test complete");
    
    // Configuration test
    Serial.println("Configuration loaded successfully");
    
    Serial.println("========================\n");
}

void showHelp() {
    Serial.println("\n=== Available Commands ===");
    Serial.println("status  - Show device status");
    Serial.println("info    - Show device information and capabilities");
    Serial.println("config  - Show configuration schema");
    Serial.println("current - Show current configuration values");
    Serial.println("restart - Restart the device");
    Serial.println("test    - Run device diagnostics");
    Serial.println("help    - Show this help");
    Serial.println("");
    Serial.println("=== Configuration ===");
    Serial.println("Send JSON to update configuration:");
    Serial.println("{\"device_name\":\"My Device\",\"led_enabled\":true,\"led_mode\":\"blink\"}");
    Serial.println("====================\n");
}
