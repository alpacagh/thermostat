#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

class Display {
public:
    bool begin();

    void showBootMessage(const char* message);
    void showStatus(float temp, float humidity, bool relay_on, bool overridden,
                    uint8_t hour, uint8_t min, float open_temp, float close_temp,
                    bool wifi_connected, const char* ip_address, bool sensor_valid,
                    unsigned long override_remaining_sec);

private:
    bool initialized = false;
};

extern Display display;

#endif
