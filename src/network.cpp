#include "network.h"
#include "config.h"
#include "config_store.h"
#include "sensor.h"
#include "relay.h"
#include "relay_log.h"
#include "scheduler.h"
#include <ESP8266WiFi.h>
#include <time.h>

extern "C" {
    #include <lwip/dhcp.h>
    struct netif* eagle_lwip_getif(int netif_index);
}

Network network;

static WiFiServer tcpServer(TCP_SERVER_PORT);

void Network::begin() {
    WiFi.mode(WIFI_STA);
    tcpServer.begin();
}

void Network::loop() {
    handleTcpClients();

    unsigned long now = millis();

    // WiFi reconnection logic with exponential backoff
    static unsigned long last_wifi_check = 0;
    static unsigned long next_check_interval = WIFI_CHECK_INTERVAL;

    if (now - last_wifi_check > next_check_interval) {
        last_wifi_check = now;
        if (!isWifiConnected()) {
            Serial.println("WiFi disconnected, attempting reconnection...");
            if (connectWifi(configStore.getWifiSsid(), configStore.getWifiPass())) {
                Serial.println("WiFi reconnected successfully");
                next_check_interval = WIFI_CHECK_INTERVAL;
                reconnect_attempts = 0;

                // Resync time after reconnection (periodic NTP sync will retry if this fails)
                if (syncTime()) {
                    Serial.println("Time synced after reconnection");
                } else {
                    Serial.println("Time sync failed after reconnection, will retry on next NTP interval");
                }
            } else {
                reconnect_attempts++;
                Serial.printf("WiFi reconnection failed, attempt %d\n", reconnect_attempts);

                // Exponential backoff: 30s, 60s, 120s, 240s, 300s (max)
                if (next_check_interval < WIFI_CHECK_INTERVAL_MAX) {
                    next_check_interval = next_check_interval * 2;
                    if (next_check_interval > WIFI_CHECK_INTERVAL_MAX) {
                        next_check_interval = WIFI_CHECK_INTERVAL_MAX;
                    }
                }
                Serial.printf("Next reconnection attempt in %ld seconds\n", next_check_interval / 1000);

                // Reset reconnection attempts after many failures
                if (reconnect_attempts > 10) {
                    reconnect_attempts = 0;
                    next_check_interval = WIFI_CHECK_INTERVAL;
                    Serial.println("Resetting WiFi reconnection attempts");
                }
            }
        } else {
            // Reset to normal check interval when connected
            next_check_interval = WIFI_CHECK_INTERVAL;
        }
    }

    // Periodic NTP sync
    if (wifi_connected && (now - last_ntp_sync > NTP_SYNC_INTERVAL)) {
        syncTime();
    }

    // Periodic DHCP lease renewal
    if (wifi_connected && (now - last_dhcp_renew > DHCP_RENEW_INTERVAL)) {
        last_dhcp_renew = now;
        struct netif* nif = eagle_lwip_getif(0);
        if (nif && netif_dhcp_data(nif)) {
            dhcp_renew(nif);
            Serial.println("DHCP lease renewed");
        }
    }
}

bool Network::connectWifi(const char* ssid, const char* pass) {
    WiFi.begin(ssid, pass);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        attempts++;
    }

    wifi_connected = (WiFi.status() == WL_CONNECTED);
    if (wifi_connected) {
        reconnect_attempts = 0;
    }
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

    if (command.startsWith("WHOAMI")) {
        return "OK THERMOSTAT,v1.0";
    }

    if (command.startsWith("STATUS")) {
        char buf[192];
        snprintf(buf, sizeof(buf),
                 "OK temp=%.1f humidity=%.1f relay=%s override=%s override_remaining=%lu upper_limit=%s schedule=%d",
                 tempSensor.getTemperature(),
                 tempSensor.getHumidity(),
                 relayControl.isOn() ? "ON" : "OFF",
                 relayControl.isOverridden() ? "YES" : "NO",
                 relayControl.getOverrideRemaining() / 1000,
                 relayControl.isUpperLimitActive() ? "YES" : "NO",
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
            // Check for optional duration after "ON"
            int pos = command.indexOf("ON") + 2;
            String rest = cmd.substring(pos);
            rest.trim();
            if (rest.length() > 0) {
                unsigned long dur = parseDurationMs(rest.c_str());
                if (dur > 0) {
                    relayControl.setOverride(OverrideState::FORCE_ON, dur);
                    return "OK";
                }
                return "ERR invalid duration";
            }
            relayControl.setOverride(OverrideState::FORCE_ON);
            return "OK";
        }
        if (command.indexOf("OFF") > 0) {
            int pos = command.indexOf("OFF") + 3;
            String rest = cmd.substring(pos);
            rest.trim();
            if (rest.length() > 0) {
                unsigned long dur = parseDurationMs(rest.c_str());
                if (dur > 0) {
                    relayControl.setOverride(OverrideState::FORCE_OFF, dur);
                    return "OK";
                }
                return "ERR invalid duration";
            }
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

    if (command.startsWith("STATS")) {
        char buf[128];
        snprintf(buf, sizeof(buf), "OK on_1h=%u on_6h=%u on_12h=%u on_24h=%u",
                 relayLog.getOnSeconds(1),
                 relayLog.getOnSeconds(6),
                 relayLog.getOnSeconds(12),
                 relayLog.getOnSeconds(24));
        return String(buf);
    }

    if (command.startsWith("LOG")) {
        int count = 20;  // Default
        sscanf(cmd.c_str(), "%*s %d", &count);
        if (count > 1000) count = 1000;
        if (count < 1) count = 1;

        String result = "OK\n";
        uint16_t total = relayLog.getLogCount();
        uint16_t start = (total > count) ? (total - count) : 0;

        for (uint16_t i = start; i < total; i++) {
            RelayLogEntry entry;
            if (relayLog.getLogEntry(i, entry)) {
                char buf[48];
                snprintf(buf, sizeof(buf), "%u %.1f %s\n",
                         entry.timestamp,
                         entry.temp_x10 / 10.0f,
                         entry.state ? "ON" : "OFF");
                result += buf;
            }
        }
        return result;
    }

    if (command.startsWith("CLOSE")) {
        return "OK closing connection";
    }

    return "ERR unknown command";
}
