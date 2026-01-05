#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>
#include "config.h"

struct Schedule {
    uint8_t time_from_hour;
    uint8_t time_from_min;
    uint8_t time_to_hour;
    uint8_t time_to_min;
    float open_temp;
    float close_temp;
    bool active;
};

struct StoredConfig {
    uint32_t magic;  // Validation marker
    char wifi_ssid[WIFI_SSID_MAX_LEN];
    char wifi_pass[WIFI_PASS_MAX_LEN];
    int8_t timezone;  // UTC offset in hours
    uint8_t schedule_count;
    Schedule schedules[MAX_SCHEDULES];
};

class ConfigStore {
public:
    void begin();
    void load();
    void save();
    void reset();

    // WiFi
    const char* getWifiSsid();
    const char* getWifiPass();
    void setWifiCredentials(const char* ssid, const char* pass);
    bool hasWifiCredentials();

    // Timezone
    int8_t getTimezone();
    void setTimezone(int8_t tz);

    // Schedules
    uint8_t getScheduleCount();
    Schedule* getSchedule(uint8_t index);
    bool setSchedule(uint8_t index, uint8_t from_h, uint8_t from_m,
                     uint8_t to_h, uint8_t to_m, float open_t, float close_t);
    bool deleteSchedule(uint8_t index);

private:
    StoredConfig config;
    static const uint32_t MAGIC_NUMBER = 0x54484552;  // "THER"
};

extern ConfigStore configStore;

#endif
