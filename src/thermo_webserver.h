#ifndef THERMO_WEBSERVER_H
#define THERMO_WEBSERVER_H

#include <Arduino.h>

class ThermoWebServer {
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

extern ThermoWebServer webServer;

#endif
