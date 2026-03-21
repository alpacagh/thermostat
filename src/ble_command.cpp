#include "ble_command.h"
#include "config.h"
#include "config_store.h"
#include "network.h"
#include "relay.h"
#include "scheduler.h"
#include "sensor.h"

BleCommand bleCommand;

void BleCommand::begin() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setMTU(BLE_MTU_SIZE);

    NimBLEServer* server = NimBLEDevice::createServer();
    server->setCallbacks(new BleServerCallbacks(this));

    NimBLEService* service = server->createService(BLE_NUS_SERVICE_UUID);

    txCharacteristic = service->createCharacteristic(
        BLE_NUS_TX_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );

    NimBLECharacteristic* rxCharacteristic = service->createCharacteristic(
        BLE_NUS_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    rxCharacteristic->setCallbacks(this);

    service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_NUS_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    Serial.println("BLE command server started");
}

void BleCommand::loop() {
    if (has_pending) {
        has_pending = false;
        String cmd = pending_command;
        String response = processCommand(cmd);
        sendResponse(response);
    }

    if (wifi_reset_pending) {
        wifi_reset_pending = false;
        network.reconnectWifi();
    }
}

void BleCommand::onWrite(NimBLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
        pending_command = String(value.c_str());
        pending_command.trim();
        if (pending_command.length() > 0) {
            has_pending = true;
        }
    }
}

String BleCommand::processCommand(const String& cmd) {
    String command = cmd;
    command.toUpperCase();

    if (command.startsWith("STATUS")) {
        return handleStatus();
    }
    if (command.startsWith("SET_WIFI")) {
        return handleSetWifi(cmd);  // Use original case for credentials
    }
    if (command.startsWith("SET_TIMEZONE")) {
        return handleSetTimezone(cmd);
    }
    if (command.startsWith("WIFI_RESET")) {
        return handleWifiReset();
    }

    return "ERR unknown command";
}

void BleCommand::sendResponse(const String& response) {
    if (!client_connected || !txCharacteristic) return;

    uint16_t mtu = NimBLEDevice::getMTU() - 3;
    if (mtu < 20) mtu = 20;

    if ((uint16_t)response.length() <= mtu) {
        txCharacteristic->setValue((const uint8_t*)response.c_str(), response.length());
        txCharacteristic->notify();
        return;
    }

    // Chunked send for long responses
    uint16_t offset = 0;
    while (offset < response.length()) {
        uint16_t chunk = response.length() - offset;
        if (chunk > mtu) chunk = mtu;
        txCharacteristic->setValue((const uint8_t*)(response.c_str() + offset), chunk);
        txCharacteristic->notify();
        offset += chunk;
        if (offset < response.length()) {
            delay(20);
        }
    }
}

String BleCommand::handleStatus() {
    char buf[256];
    const char* ip = network.isWifiConnected() ? network.getLocalIP().c_str() : "disconnected";

    if (network.isWifiConnected()) {
        char ip_buf[16];
        strncpy(ip_buf, network.getLocalIP().c_str(), sizeof(ip_buf) - 1);
        ip_buf[sizeof(ip_buf) - 1] = '\0';
        snprintf(buf, sizeof(buf),
                 "OK relay=%s override=%s override_remaining=%lu schedule=%d wifi=%s http=http://%s/",
                 relayControl.isOn() ? "ON" : "OFF",
                 relayControl.isOverridden() ? "YES" : "NO",
                 relayControl.getOverrideRemaining() / 1000,
                 scheduler.getActiveScheduleIndex(),
                 ip_buf, ip_buf);
    } else {
        snprintf(buf, sizeof(buf),
                 "OK relay=%s override=%s override_remaining=%lu schedule=%d wifi=disconnected",
                 relayControl.isOn() ? "ON" : "OFF",
                 relayControl.isOverridden() ? "YES" : "NO",
                 relayControl.getOverrideRemaining() / 1000,
                 scheduler.getActiveScheduleIndex());
    }
    return String(buf);
}

String BleCommand::handleSetWifi(const String& cmd) {
    // Format: SET_WIFI <ssid> <password>
    // Find start of SSID (after "SET_WIFI ")
    int ssid_start = cmd.indexOf(' ');
    if (ssid_start < 0) return "ERR missing ssid and password";
    ssid_start++;  // skip space

    // Skip extra spaces
    while (ssid_start < (int)cmd.length() && cmd.charAt(ssid_start) == ' ') ssid_start++;
    if (ssid_start >= (int)cmd.length()) return "ERR missing ssid and password";

    // Find end of SSID (next space)
    int ssid_end = cmd.indexOf(' ', ssid_start);
    if (ssid_end < 0) return "ERR missing password";

    String ssid = cmd.substring(ssid_start, ssid_end);

    // Password is everything after SSID space
    int pass_start = ssid_end + 1;
    while (pass_start < (int)cmd.length() && cmd.charAt(pass_start) == ' ') pass_start++;
    if (pass_start >= (int)cmd.length()) return "ERR missing password";

    String pass = cmd.substring(pass_start);

    if (ssid.length() == 0 || ssid.length() >= WIFI_SSID_MAX_LEN) return "ERR invalid ssid";
    if (pass.length() == 0 || pass.length() >= WIFI_PASS_MAX_LEN) return "ERR invalid password";

    configStore.setWifiCredentials(ssid.c_str(), pass.c_str());
    return "OK wifi_set";
}

String BleCommand::handleSetTimezone(const String& cmd) {
    // Format: SET_TIMEZONE <offset>
    int tz;
    if (sscanf(cmd.c_str(), "%*s %d", &tz) != 1) {
        return "ERR missing timezone offset";
    }
    if (tz < -12 || tz > 14) {
        return "ERR invalid timezone";
    }
    configStore.setTimezone(tz);
    network.setTimezone(tz);

    char buf[16];
    snprintf(buf, sizeof(buf), "OK tz=%d", tz);
    return String(buf);
}

String BleCommand::handleWifiReset() {
    if (!configStore.hasWifiCredentials()) {
        return "ERR no wifi credentials";
    }
    wifi_reset_pending = true;
    return "OK wifi_resetting";
}

// Server callbacks

void BleServerCallbacks::onConnect(NimBLEServer* pServer) {
    owner->client_connected = true;
    Serial.println("BLE client connected");
}

void BleServerCallbacks::onDisconnect(NimBLEServer* pServer) {
    owner->client_connected = false;
    Serial.println("BLE client disconnected");
    NimBLEDevice::startAdvertising();
}
