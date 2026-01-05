#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include "config_store.h"

class Scheduler {
public:
    void update(uint8_t current_hour, uint8_t current_min);

    bool hasActiveSchedule();
    float getOpenTemp();
    float getCloseTemp();
    int8_t getActiveScheduleIndex();

private:
    bool active_schedule_found = false;
    int8_t active_index = -1;
    float current_open_temp = 0;
    float current_close_temp = 0;

    bool isTimeInRange(uint8_t hour, uint8_t min,
                       uint8_t from_h, uint8_t from_m,
                       uint8_t to_h, uint8_t to_m);
};

extern Scheduler scheduler;

#endif
