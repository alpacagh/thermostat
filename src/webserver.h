#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>

class WebServer {
public:
    void begin();
    void loop();

private:
    void handleRoot();
    void handleApiStatus();
    void handleApiSchedules();
    void handleApiSchedulePost();
    void handleApiScheduleDelete();
    void handleApiOverride();
    void handleApiConfigGet();
    void handleApiConfigPost();
    void handleApiStats();
    void handleApiLog();
    void handleApiLogPlantUML();
    void handleNotFound();
};

extern WebServer webServer;

#endif
