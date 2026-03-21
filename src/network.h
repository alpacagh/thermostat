#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

class Network {
public:
    void begin();
    void loop();

    bool connectWifi(const char* ssid, const char* pass);
    void disconnectWifi();
    bool reconnectWifi();
    bool isWifiConnected();
    String getLocalIP();

    bool syncTime();
    uint8_t getHour();
    uint8_t getMinute();
    uint8_t getSecond();

    void setTimezone(int8_t tz);

    // Connection monitoring
    int getReconnectAttempts() const { return reconnect_attempts; }

private:
    bool wifi_connected = false;
    int8_t timezone_offset = 0;
    unsigned long last_ntp_sync = 0;
    unsigned long last_dhcp_renew = 0;
    int reconnect_attempts = 0;

    void handleTcpClients();
    String processCommand(const String& cmd);
};

extern Network network;

#endif
