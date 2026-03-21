#include "serial_config.h"
#include "config_store.h"
#include <WiFi.h>

SerialSetup serialSetup;

void SerialSetup::begin() {
    Serial.begin(115200);
    delay(2000);  // Wait for USB CDC to enumerate
    Serial.println();
    Serial.println("Thermostat v1.0");

    if (!configStore.hasWifiCredentials()) {
        WiFi.mode(WIFI_OFF);  // Prevent ESP-IDF auto-connect with stale NVS data
        Serial.println("No WiFi credentials configured.");
        Serial.println("Entering config mode (also available via BLE).");
        config_mode = true;
        Serial.println("\n=== Configuration Mode ===");
        showMenu();
        return;
    }

    Serial.println("Press 'c' within 3 seconds to enter config mode...");

    unsigned long start = millis();
    while (millis() - start < 3000) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == 'c' || c == 'C') {
                config_mode = true;
                Serial.println("\n=== Configuration Mode ===");
                showMenu();
                return;
            }
        }
        delay(100);
    }
    Serial.println("Starting normal operation...");
}

void SerialSetup::loop() {
    if (!config_mode) return;

    if (Serial.available()) {
        String input = readLine();
        handleInput(input);
    }
}

bool SerialSetup::isConfigMode() {
    return config_mode;
}

void SerialSetup::showMenu() {
    Serial.println();
    Serial.println("Configuration Menu:");
    Serial.println("  1. Configure WiFi");
    Serial.println("  2. Configure Timezone");
    Serial.println("  3. Show Current Config");
    Serial.println("  4. Reset All Settings");
    Serial.println("  5. Exit Config Mode");
    Serial.println();
    Serial.print("> ");
}

void SerialSetup::handleInput(const String& input) {
    if (input == "1") {
        configureWifi();
    } else if (input == "2") {
        configureTimezone();
    } else if (input == "3") {
        showCurrentConfig();
    } else if (input == "4") {
        resetConfig();
    } else if (input == "5") {
        Serial.println("Exiting config mode. Restarting...");
        delay(1000);
        ESP.restart();
    } else {
        Serial.println("Invalid option.");
    }
    showMenu();
}

void SerialSetup::configureWifi() {
    Serial.println();
    Serial.print("Enter WiFi SSID: ");
    String ssid = readLine();

    Serial.print("Enter WiFi Password: ");
    String pass = readLine();

    configStore.setWifiCredentials(ssid.c_str(), pass.c_str());
    Serial.println("WiFi credentials saved.");
}

void SerialSetup::configureTimezone() {
    Serial.println();
    Serial.print("Enter timezone offset (e.g., -5 for EST, +3 for MSK): ");
    String tz_str = readLine();

    int tz = tz_str.toInt();
    if (tz >= -12 && tz <= 14) {
        configStore.setTimezone(tz);
        Serial.print("Timezone set to UTC");
        if (tz >= 0) Serial.print("+");
        Serial.println(tz);
    } else {
        Serial.println("Invalid timezone. Must be between -12 and +14.");
    }
}

void SerialSetup::showCurrentConfig() {
    Serial.println();
    Serial.println("Current Configuration:");
    Serial.print("  WiFi SSID: ");
    Serial.println(configStore.getWifiSsid());
    Serial.print("  WiFi Pass: ");
    Serial.println(strlen(configStore.getWifiPass()) > 0 ? "********" : "(not set)");
    Serial.print("  Timezone: UTC");
    int8_t tz = configStore.getTimezone();
    if (tz >= 0) Serial.print("+");
    Serial.println(tz);
    Serial.print("  Schedules: ");
    Serial.println(configStore.getScheduleCount());

    for (int i = 0; i < MAX_SCHEDULES; i++) {
        Schedule* s = configStore.getSchedule(i);
        if (s && s->active) {
            Serial.print("    [");
            Serial.print(i);
            Serial.print("] ");
            Serial.print(s->time_from_hour);
            Serial.print(":");
            if (s->time_from_min < 10) Serial.print("0");
            Serial.print(s->time_from_min);
            Serial.print(" - ");
            Serial.print(s->time_to_hour);
            Serial.print(":");
            if (s->time_to_min < 10) Serial.print("0");
            Serial.print(s->time_to_min);
            Serial.print(" | ");
            Serial.print(s->open_temp, 1);
            Serial.print("C - ");
            Serial.print(s->close_temp, 1);
            Serial.println("C");
        }
    }
}

void SerialSetup::resetConfig() {
    Serial.println();
    Serial.print("Are you sure? Type 'YES' to confirm: ");
    String confirm = readLine();
    if (confirm == "YES") {
        configStore.reset();
        Serial.println("All settings reset to defaults.");
    } else {
        Serial.println("Reset cancelled.");
    }
}

String SerialSetup::readLine() {
    String result = "";
    while (true) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                Serial.println();
                break;
            }
            result += c;
            Serial.print(c);
        }
        yield();
    }
    result.trim();
    return result;
}
