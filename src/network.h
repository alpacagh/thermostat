#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

class Network {
public:
    void begin();
    void loop();

    bool connectWifi(const char* ssid, const char* pass);
    bool isWifiConnected();
    String getLocalIP();

    bool syncTime();
    uint8_t getHour();
    uint8_t getMinute();
    uint8_t getSecond();

    void setTimezone(int8_t tz);

private:
    bool wifi_connected = false;
    int8_t timezone_offset = 0;
    unsigned long last_ntp_sync = 0;

    void handleTcpClients();
    String processCommand(const String& cmd);
};

extern Network network;

#endif
