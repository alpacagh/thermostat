#ifndef BLE_COMMAND_H
#define BLE_COMMAND_H

#include <Arduino.h>
#include <NimBLEDevice.h>

class BleCommand : public NimBLECharacteristicCallbacks {
public:
    void begin();
    void loop();

    bool isClientConnected() const { return client_connected; }

private:
    NimBLECharacteristic* txCharacteristic = nullptr;
    volatile bool client_connected = false;
    volatile bool has_pending = false;
    volatile bool wifi_reset_pending = false;
    String pending_command;

    // NimBLE characteristic callback
    void onWrite(NimBLECharacteristic* pCharacteristic) override;

    // Command processing
    String processCommand(const String& cmd);
    void sendResponse(const String& response);

    // Command handlers
    String handleStatus();
    String handleSetWifi(const String& cmd);
    String handleSetTimezone(const String& cmd);
    String handleWifiReset();

    friend class BleServerCallbacks;
};

class BleServerCallbacks : public NimBLEServerCallbacks {
public:
    BleServerCallbacks(BleCommand* owner) : owner(owner) {}
    void onConnect(NimBLEServer* pServer) override;
    void onDisconnect(NimBLEServer* pServer) override;

private:
    BleCommand* owner;
};

extern BleCommand bleCommand;

#endif
