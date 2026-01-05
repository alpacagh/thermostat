#include "config_store.h"
#include <EEPROM.h>

ConfigStore configStore;

void ConfigStore::begin() {
    EEPROM.begin(sizeof(StoredConfig));
    load();
}

void ConfigStore::load() {
    EEPROM.get(0, config);
    if (config.magic != MAGIC_NUMBER) {
        reset();
    }
}

void ConfigStore::save() {
    config.magic = MAGIC_NUMBER;
    EEPROM.put(0, config);
    EEPROM.commit();
}

void ConfigStore::reset() {
    memset(&config, 0, sizeof(StoredConfig));
    config.magic = MAGIC_NUMBER;
    config.timezone = 0;
    config.schedule_count = 0;
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        config.schedules[i].active = false;
    }
    save();
}

const char* ConfigStore::getWifiSsid() {
    return config.wifi_ssid;
}

const char* ConfigStore::getWifiPass() {
    return config.wifi_pass;
}

void ConfigStore::setWifiCredentials(const char* ssid, const char* pass) {
    strncpy(config.wifi_ssid, ssid, WIFI_SSID_MAX_LEN - 1);
    config.wifi_ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
    strncpy(config.wifi_pass, pass, WIFI_PASS_MAX_LEN - 1);
    config.wifi_pass[WIFI_PASS_MAX_LEN - 1] = '\0';
    save();
}

bool ConfigStore::hasWifiCredentials() {
    return strlen(config.wifi_ssid) > 0;
}

int8_t ConfigStore::getTimezone() {
    return config.timezone;
}

void ConfigStore::setTimezone(int8_t tz) {
    config.timezone = tz;
    save();
}

uint8_t ConfigStore::getScheduleCount() {
    uint8_t count = 0;
    for (int i = 0; i < MAX_SCHEDULES; i++) {
        if (config.schedules[i].active) count++;
    }
    return count;
}

Schedule* ConfigStore::getSchedule(uint8_t index) {
    if (index >= MAX_SCHEDULES) return nullptr;
    return &config.schedules[index];
}

bool ConfigStore::setSchedule(uint8_t index, uint8_t from_h, uint8_t from_m,
                               uint8_t to_h, uint8_t to_m, float open_t, float close_t) {
    if (index >= MAX_SCHEDULES) return false;
    if (from_h > 23 || from_m > 59 || to_h > 23 || to_m > 59) return false;
    if (open_t >= close_t) return false;  // Invalid hysteresis

    config.schedules[index].time_from_hour = from_h;
    config.schedules[index].time_from_min = from_m;
    config.schedules[index].time_to_hour = to_h;
    config.schedules[index].time_to_min = to_m;
    config.schedules[index].open_temp = open_t;
    config.schedules[index].close_temp = close_t;
    config.schedules[index].active = true;
    config.schedule_count = getScheduleCount();
    save();
    return true;
}

bool ConfigStore::deleteSchedule(uint8_t index) {
    if (index >= MAX_SCHEDULES) return false;
    config.schedules[index].active = false;
    config.schedule_count = getScheduleCount();
    save();
    return true;
}
