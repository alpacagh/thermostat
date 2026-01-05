#include "sensor.h"
#include "config.h"
#include <DHT.h>

TempSensor tempSensor;

static DHT dht(PIN_DHT, DHT_TYPE);

void TempSensor::begin() {
    dht.begin();
}

bool TempSensor::read() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        valid = false;
        return false;
    }

    temperature = t;
    humidity = h;
    valid = true;
    last_read_time = millis();
    return true;
}

float TempSensor::getTemperature() {
    return temperature;
}

float TempSensor::getHumidity() {
    return humidity;
}

bool TempSensor::isValid() {
    return valid;
}

unsigned long TempSensor::getLastReadTime() {
    return last_read_time;
}
