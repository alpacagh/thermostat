#include "webserver.h"
#include "webpage.h"
#include "config.h"
#include "config_store.h"
#include "sensor.h"
#include "relay.h"
#include "relay_log.h"
#include "scheduler.h"
#include "network.h"
#include <ESP8266WebServer.h>
#include <time.h>

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
    httpServer.on("/api/stats", HTTP_GET, [this]() { handleApiStats(); });
    httpServer.on("/api/log", HTTP_GET, [this]() { handleApiLog(); });
    httpServer.on("/api/log/plantuml", HTTP_GET, [this]() { handleApiLogPlantUML(); });
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

void WebServer::handleApiStats() {
    uint32_t on_1h = relayLog.getOnSeconds(1);
    uint32_t on_6h = relayLog.getOnSeconds(6);
    uint32_t on_12h = relayLog.getOnSeconds(12);
    uint32_t on_24h = relayLog.getOnSeconds(24);

    char json[256];
    snprintf(json, sizeof(json),
        "{\"on_1h\":%u,\"on_6h\":%u,\"on_12h\":%u,\"on_24h\":%u,"
        "\"on_1h_pct\":%.1f,\"on_6h_pct\":%.1f,\"on_12h_pct\":%.1f,\"on_24h_pct\":%.1f}",
        on_1h, on_6h, on_12h, on_24h,
        on_1h * 100.0 / 3600.0,
        on_6h * 100.0 / 21600.0,
        on_12h * 100.0 / 43200.0,
        on_24h * 100.0 / 86400.0
    );
    httpServer.send(200, "application/json", json);
}

void WebServer::handleApiLog() {
    int count = 100;  // Default
    if (httpServer.hasArg("count")) {
        count = httpServer.arg("count").toInt();
        if (count > 1000) count = 1000;
        if (count < 1) count = 1;
    }

    String json = "[";
    uint16_t total = relayLog.getLogCount();
    uint16_t start = (total > count) ? (total - count) : 0;
    bool first = true;

    for (uint16_t i = start; i < total; i++) {
        RelayLogEntry entry;
        if (relayLog.getLogEntry(i, entry)) {
            if (!first) json += ",";
            first = false;
            char buf[64];
            snprintf(buf, sizeof(buf), "{\"t\":%u,\"temp\":%.1f,\"s\":%d}",
                     entry.timestamp,
                     entry.temp_x10 / 10.0f,
                     entry.state);
            json += buf;
        }
    }
    json += "]";
    httpServer.send(200, "application/json", json);
}

// Helper to convert a byte to two hex chars
static void byteToHex(uint8_t b, char* out) {
    const char hex[] = "0123456789abcdef";
    out[0] = hex[b >> 4];
    out[1] = hex[b & 0x0f];
}

void WebServer::handleApiLogPlantUML() {
    // Get configurable PlantUML server URL
    String baseUrl = PLANTUML_DEFAULT_SERVER;
    if (httpServer.hasArg("server")) {
        baseUrl = httpServer.arg("server");
    }

    // Build PlantUML timing diagram source
    String puml = "@startuml\n";
    puml += "title Relay Activity Log\n";
    puml += "analog \"Temp\" between 10 and 30 as T\n";
    puml += "T ticks num on multiple 2\n";
    puml += "binary \"Relay\" as R\n\n";

    uint16_t total = relayLog.getLogCount();
    uint16_t count = total > 100 ? 100 : total;  // Limit for URL length
    uint16_t start = total > count ? total - count : 0;

    for (uint16_t i = start; i < total; i++) {
        RelayLogEntry entry;
        if (relayLog.getLogEntry(i, entry)) {
            // Format timestamp as HH:MM:SS
            time_t ts = entry.timestamp;
            struct tm* tm_info = localtime(&ts);
            char timeStr[16];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                     tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);

            puml += "@";
            puml += timeStr;
            puml += "\n";
            puml += "T is ";
            char tempStr[8];
            snprintf(tempStr, sizeof(tempStr), "%.1f", entry.temp_x10 / 10.0f);
            puml += tempStr;
            puml += "\n";
            puml += "R is ";
            puml += entry.state ? "high" : "low";
            puml += "\n\n";
        }
    }

    puml += "@enduml";

    // Build hex-encoded URL using ~h prefix
    String hexEncoded = "";
    for (size_t i = 0; i < puml.length(); i++) {
        char hex[3];
        byteToHex((uint8_t)puml[i], hex);
        hex[2] = '\0';
        hexEncoded += hex;
    }

    String imgUrl = baseUrl + "~h" + hexEncoded;

    // Return HTML page with embedded image
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset=\"UTF-8\">";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<title>Relay Timeline</title>";
    html += "<style>body{font-family:sans-serif;margin:20px;background:#f5f5f5;}";
    html += ".container{max-width:100%;overflow-x:auto;background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}";
    html += "h1{color:#333;margin-bottom:20px;}";
    html += "img{max-width:100%;height:auto;}";
    html += "pre{background:#f0f0f0;padding:10px;overflow-x:auto;font-size:12px;}</style>";
    html += "</head><body>";
    html += "<div class=\"container\">";
    html += "<h1>Relay Activity Timeline</h1>";
    html += "<p>Last " + String(count) + " events</p>";
    html += "<img src=\"" + imgUrl + "\" alt=\"Timing Diagram\">";
    html += "<h2>PlantUML Source</h2>";
    html += "<pre>" + puml + "</pre>";
    html += "<p><a href=\"/\">Back to Dashboard</a></p>";
    html += "</div></body></html>";

    httpServer.send(200, "text/html", html);
}

void WebServer::handleNotFound() {
    httpServer.send(404, "text/plain", "Not Found");
}
