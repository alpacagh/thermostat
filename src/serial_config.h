#ifndef SERIAL_CONFIG_H
#define SERIAL_CONFIG_H

#include <Arduino.h>

class SerialSetup {
public:
    void begin();
    void loop();
    bool isConfigMode();

private:
    bool config_mode = false;

    void showMenu();
    void handleInput(const String& input);
    void configureWifi();
    void configureTimezone();
    void showCurrentConfig();
    void resetConfig();
    String readLine();
};

extern SerialSetup serialSetup;

#endif
