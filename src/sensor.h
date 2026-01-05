#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>

class TempSensor {
public:
    void begin();
    bool read();

    float getTemperature();
    float getHumidity();
    bool isValid();
    unsigned long getLastReadTime();

private:
    float temperature = 0;
    float humidity = 0;
    bool valid = false;
    unsigned long last_read_time = 0;
};

extern TempSensor tempSensor;

#endif
