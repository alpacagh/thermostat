#include "network.h"
#include "config.h"
#include "config_store.h"
#include "sensor.h"
#include "relay.h"
#include "scheduler.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <time.h>

Network network;

static WiFiServer tcpServer(TCP_SERVER_PORT);
static WiFiUDP udp;

void Network::begin() {
    WiFi.mode(WIFI_STA);
    udp.begin(UDP_DISCOVERY_PORT);
    tcpServer.begin();
}

void Network::loop() {
    handleUdpDiscovery();
    handleTcpClients();

    // Periodic NTP sync
    if (wifi_connected && (millis() - last_ntp_sync > NTP_SYNC_INTERVAL)) {
        syncTime();
    }
}

bool Network::connectWifi(const char* ssid, const char* pass) {
    WiFi.begin(ssid, pass);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        attempts++;
    }

    wifi_connected = (WiFi.status() == WL_CONNECTED);
    return wifi_connected;
}

bool Network::isWifiConnected() {
    wifi_connected = (WiFi.status() == WL_CONNECTED);
    return wifi_connected;
}

String Network::getLocalIP() {
    if (!wifi_connected) return "0.0.0.0";
    return WiFi.localIP().toString();
}

bool Network::syncTime() {
    configTime(timezone_offset * 3600, 0, NTP_SERVER);

    time_t now = 0;
    int attempts = 0;
    while (now < 1000000000 && attempts < 20) {
        delay(500);
        time(&now);
        attempts++;
    }

    if (now > 1000000000) {
        last_ntp_sync = millis();
        return true;
    }
    return false;
}

uint8_t Network::getHour() {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_hour;
}

uint8_t Network::getMinute() {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_min;
}

uint8_t Network::getSecond() {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    return timeinfo->tm_sec;
}

void Network::setTimezone(int8_t tz) {
    timezone_offset = tz;
}

void Network::handleUdpDiscovery() {
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        char buffer[64];
        int len = udp.read(buffer, sizeof(buffer) - 1);
        if (len > 0) {
            buffer[len] = '\0';
            if (strstr(buffer, "DISCOVER") != nullptr) {
                // Send discovery response
                String response = String(DEVICE_TYPE) + "," +
                                  getLocalIP() + "," +
                                  String(TCP_SERVER_PORT) + "," +
                                  PROTOCOL_VERSION;
                udp.beginPacket(udp.remoteIP(), udp.remotePort());
                udp.write(response.c_str());
                udp.endPacket();
            }
        }
    }
}

void Network::handleTcpClients() {
    WiFiClient client = tcpServer.accept();
    if (!client) return;

    client.setTimeout(5000);
    while (client.connected()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line.length() > 0) {
                String response = processCommand(line);
                client.println(response);
                if (line.startsWith("CLOSE")) {
                    break;
                }
            }
        }
        yield();
    }
    client.stop();
}

String Network::processCommand(const String& cmd) {
    String command = cmd;
    command.toUpperCase();

    if (command.startsWith("STATUS")) {
        char buf[128];
        snprintf(buf, sizeof(buf),
                 "OK temp=%.1f humidity=%.1f relay=%s override=%s schedule=%d",
                 tempSensor.getTemperature(),
                 tempSensor.getHumidity(),
                 relayControl.isOn() ? "ON" : "OFF",
                 relayControl.isOverridden() ? "YES" : "NO",
                 scheduler.getActiveScheduleIndex());
        return String(buf);
    }

    if (command.startsWith("GET_SCHEDULES")) {
        String result = "OK\n";
        for (int i = 0; i < MAX_SCHEDULES; i++) {
            Schedule* s = configStore.getSchedule(i);
            if (s && s->active) {
                char buf[64];
                snprintf(buf, sizeof(buf), "%d %02d:%02d %02d:%02d %.1f %.1f\n",
                         i, s->time_from_hour, s->time_from_min,
                         s->time_to_hour, s->time_to_min,
                         s->open_temp, s->close_temp);
                result += buf;
            }
        }
        return result;
    }

    if (command.startsWith("SET_SCHEDULE")) {
        // Format: SET_SCHEDULE idx HH:MM HH:MM open close
        int idx;
        int from_h, from_m, to_h, to_m;
        float open_t, close_t;

        // Parse: SET_SCHEDULE 0 08:00 22:00 20.0 22.0
        if (sscanf(cmd.c_str(), "%*s %d %d:%d %d:%d %f %f",
                   &idx, &from_h, &from_m, &to_h, &to_m, &open_t, &close_t) == 7) {
            if (configStore.setSchedule(idx, from_h, from_m, to_h, to_m, open_t, close_t)) {
                return "OK";
            }
            return "ERR invalid parameters";
        }
        return "ERR parse error";
    }

    if (command.startsWith("DEL_SCHEDULE")) {
        int idx;
        if (sscanf(cmd.c_str(), "%*s %d", &idx) == 1) {
            if (configStore.deleteSchedule(idx)) {
                return "OK";
            }
            return "ERR invalid index";
        }
        return "ERR parse error";
    }

    if (command.startsWith("OVERRIDE")) {
        if (command.indexOf("ON") > 0) {
            relayControl.setOverride(OverrideState::FORCE_ON);
            return "OK";
        }
        if (command.indexOf("OFF") > 0) {
            relayControl.setOverride(OverrideState::FORCE_OFF);
            return "OK";
        }
        if (command.indexOf("CLEAR") > 0) {
            relayControl.clearOverride();
            return "OK";
        }
        return "ERR invalid override state";
    }

    if (command.startsWith("GET_CONFIG")) {
        char buf[128];
        snprintf(buf, sizeof(buf), "OK ssid=%s timezone=%d ip=%s",
                 configStore.getWifiSsid(),
                 configStore.getTimezone(),
                 getLocalIP().c_str());
        return String(buf);
    }

    if (command.startsWith("CLOSE")) {
        return "OK closing connection";
    }

    return "ERR unknown command";
}
