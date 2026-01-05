#include "webserver.h"
#include "webpage.h"
#include "config.h"
#include "config_store.h"
#include "sensor.h"
#include "relay.h"
#include "scheduler.h"
#include "network.h"
#include <ESP8266WebServer.h>

WebServer webServer;

static ESP8266WebServer httpServer(80);

void WebServer::begin() {
    httpServer.on("/", HTTP_GET, [this]() { handleRoot(); });
    httpServer.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
    httpServer.on("/api/schedules", HTTP_GET, [this]() { handleApiSchedules(); });
    httpServer.on("/api/schedule", HTTP_POST, [this]() { handleApiSchedulePost(); });
    httpServer.on("/api/schedule", HTTP_DELETE, [this]() { handleApiScheduleDelete(); });
    httpServer.on("/api/override", HTTP_POST, [this]() { handleApiOverride(); });
    httpServer.on("/api/config", HTTP_GET, [this]() { handleApiConfigGet(); });
    httpServer.on("/api/config", HTTP_POST, [this]() { handleApiConfigPost(); });
    httpServer.onNotFound([this]() { handleNotFound(); });

    httpServer.begin();
    Serial.println("HTTP server started on port 80");
}

void WebServer::loop() {
    httpServer.handleClient();
}

void WebServer::handleRoot() {
    httpServer.send_P(200, "text/html", WEBPAGE_HTML);
}

void WebServer::handleApiStatus() {
    char json[256];
    snprintf(json, sizeof(json),
        "{\"temperature\":%.1f,\"humidity\":%.1f,\"relay\":%s,\"override\":\"%s\","
        "\"schedule\":%d,\"valid\":%s,\"time\":\"%02d:%02d\"}",
        tempSensor.getTemperature(),
        tempSensor.getHumidity(),
        relayControl.isOn() ? "true" : "false",
        relayControl.getOverride() == OverrideState::FORCE_ON ? "on" :
            (relayControl.getOverride() == OverrideState::FORCE_OFF ? "off" : "none"),
        scheduler.getActiveScheduleIndex(),
        tempSensor.isValid() ? "true" : "false",
        network.getHour(),
        network.getMinute()
    );
    httpServer.send(200, "application/json", json);
}

void WebServer::handleApiSchedules() {
    String json = "[";
    bool first = true;
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        Schedule* s = configStore.getSchedule(i);
        if (s && s->active) {
            if (!first) json += ",";
            first = false;
            char buf[128];
            snprintf(buf, sizeof(buf),
                "{\"index\":%d,\"from\":\"%02d:%02d\",\"to\":\"%02d:%02d\","
                "\"open\":%.1f,\"close\":%.1f}",
                i, s->time_from_hour, s->time_from_min,
                s->time_to_hour, s->time_to_min,
                s->open_temp, s->close_temp);
            json += buf;
        }
    }
    json += "]";
    httpServer.send(200, "application/json", json);
}

void WebServer::handleApiSchedulePost() {
    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    String body = httpServer.arg("plain");

    // Simple JSON parsing for: {"index":0,"from":"08:00","to":"22:00","open":20.0,"close":22.0}
    int idx = -1;
    int from_h = 0, from_m = 0, to_h = 0, to_m = 0;
    float open_t = 0, close_t = 0;

    // Parse index
    int pos = body.indexOf("\"index\":");
    if (pos >= 0) idx = body.substring(pos + 8).toInt();

    // Parse from time
    pos = body.indexOf("\"from\":\"");
    if (pos >= 0) {
        String fromStr = body.substring(pos + 8, pos + 13);
        sscanf(fromStr.c_str(), "%d:%d", &from_h, &from_m);
    }

    // Parse to time
    pos = body.indexOf("\"to\":\"");
    if (pos >= 0) {
        String toStr = body.substring(pos + 6, pos + 11);
        sscanf(toStr.c_str(), "%d:%d", &to_h, &to_m);
    }

    // Parse open temp
    pos = body.indexOf("\"open\":");
    if (pos >= 0) open_t = body.substring(pos + 7).toFloat();

    // Parse close temp
    pos = body.indexOf("\"close\":");
    if (pos >= 0) close_t = body.substring(pos + 8).toFloat();

    if (idx < 0 || idx >= MAX_SCHEDULES) {
        httpServer.send(400, "application/json", "{\"error\":\"invalid index\"}");
        return;
    }

    if (configStore.setSchedule(idx, from_h, from_m, to_h, to_m, open_t, close_t)) {
        httpServer.send(200, "application/json", "{\"ok\":true}");
    } else {
        httpServer.send(400, "application/json", "{\"error\":\"invalid parameters\"}");
    }
}

void WebServer::handleApiScheduleDelete() {
    if (!httpServer.hasArg("index")) {
        httpServer.send(400, "application/json", "{\"error\":\"missing index\"}");
        return;
    }

    int idx = httpServer.arg("index").toInt();
    if (configStore.deleteSchedule(idx)) {
        httpServer.send(200, "application/json", "{\"ok\":true}");
    } else {
        httpServer.send(400, "application/json", "{\"error\":\"invalid index\"}");
    }
}

void WebServer::handleApiOverride() {
    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    String body = httpServer.arg("plain");
    body.toLowerCase();

    if (body.indexOf("\"on\"") >= 0 || body.indexOf(":\"on\"") >= 0) {
        relayControl.setOverride(OverrideState::FORCE_ON);
        httpServer.send(200, "application/json", "{\"ok\":true}");
    } else if (body.indexOf("\"off\"") >= 0 || body.indexOf(":\"off\"") >= 0) {
        relayControl.setOverride(OverrideState::FORCE_OFF);
        httpServer.send(200, "application/json", "{\"ok\":true}");
    } else if (body.indexOf("\"clear\"") >= 0 || body.indexOf("\"none\"") >= 0) {
        relayControl.clearOverride();
        httpServer.send(200, "application/json", "{\"ok\":true}");
    } else {
        httpServer.send(400, "application/json", "{\"error\":\"invalid state\"}");
    }
}

void WebServer::handleApiConfigGet() {
    char json[128];
    snprintf(json, sizeof(json),
        "{\"timezone\":%d,\"ip\":\"%s\"}",
        configStore.getTimezone(),
        network.getLocalIP().c_str()
    );
    httpServer.send(200, "application/json", json);
}

void WebServer::handleApiConfigPost() {
    if (!httpServer.hasArg("plain")) {
        httpServer.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    String body = httpServer.arg("plain");

    int pos = body.indexOf("\"timezone\":");
    if (pos >= 0) {
        int tz = body.substring(pos + 11).toInt();
        if (tz >= -12 && tz <= 14) {
            configStore.setTimezone(tz);
            network.setTimezone(tz);
            httpServer.send(200, "application/json", "{\"ok\":true}");
            return;
        }
    }

    httpServer.send(400, "application/json", "{\"error\":\"invalid timezone\"}");
}

void WebServer::handleNotFound() {
    httpServer.send(404, "text/plain", "Not Found");
}
