#include <Arduino.h>
#include "config.h"
#include "config_store.h"
#include "display.h"
#include "sensor.h"
#include "relay.h"
#include "network.h"
#include "scheduler.h"
#include "serial_config.h"
#include "webserver.h"

// Timing
static unsigned long last_sensor_read = 0;
static unsigned long last_schedule_check = 0;
static unsigned long last_display_update = 0;

void setup() {
    // Initialize EEPROM config
    configStore.begin();

    // Serial config mode check
    serialSetup.begin();
    if (serialSetup.isConfigMode()) {
        return;  // Stay in config mode
    }

    // Initialize display
    if (!display.begin()) {
        Serial.println("OLED init failed!");
    }
    display.showBootMessage("Booting...");

    // Initialize sensor
    tempSensor.begin();

    // Initialize relay (starts OFF)
    relayControl.begin();

    // Initialize network
    network.begin();
    network.setTimezone(configStore.getTimezone());

    // Initialize web server
    webServer.begin();

    // Connect to WiFi
    if (configStore.hasWifiCredentials()) {
        display.showBootMessage("Connecting WiFi...");
        if (network.connectWifi(configStore.getWifiSsid(), configStore.getWifiPass())) {
            Serial.print("WiFi connected: ");
            Serial.println(network.getLocalIP());

            // Sync time
            display.showBootMessage("Syncing time...");
            if (network.syncTime()) {
                Serial.println("Time synced");
            } else {
                Serial.println("Time sync failed");
            }
        } else {
            Serial.println("WiFi connection failed");
            display.showBootMessage("WiFi failed!");
            delay(2000);
        }
    } else {
        Serial.println("No WiFi credentials. Use serial config.");
        display.showBootMessage("No WiFi config");
        delay(2000);
    }

    display.showBootMessage("Ready");
    delay(500);

    Serial.println("Thermostat ready");
}

void loop() {
    // Handle serial config mode
    if (serialSetup.isConfigMode()) {
        serialSetup.loop();
        return;
    }

    unsigned long now = millis();

    // Read sensor periodically
    if (now - last_sensor_read >= SENSOR_READ_INTERVAL) {
        last_sensor_read = now;
        tempSensor.read();
    }

    // Check schedule periodically
    if (now - last_schedule_check >= SCHEDULE_CHECK_INTERVAL) {
        last_schedule_check = now;
        scheduler.update(network.getHour(), network.getMinute());
    }

    // Update relay control
    relayControl.update(
        tempSensor.getTemperature(),
        scheduler.getOpenTemp(),
        scheduler.getCloseTemp(),
        scheduler.hasActiveSchedule(),
        tempSensor.isValid()
    );

    // Update display periodically
    if (now - last_display_update >= DISPLAY_UPDATE_INTERVAL) {
        last_display_update = now;

        display.showStatus(
            tempSensor.getTemperature(),
            tempSensor.getHumidity(),
            relayControl.isOn(),
            relayControl.isOverridden(),
            network.getHour(),
            network.getMinute(),
            scheduler.getOpenTemp(),
            scheduler.getCloseTemp(),
            network.isWifiConnected(),
            network.getLocalIP().c_str(),
            tempSensor.isValid()
        );
    }

    // Handle network (UDP discovery, TCP commands)
    network.loop();

    // Handle web server
    webServer.loop();

    yield();
}
