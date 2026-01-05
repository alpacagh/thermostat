#include "scheduler.h"

Scheduler scheduler;

void Scheduler::update(uint8_t current_hour, uint8_t current_min) {
    active_schedule_found = false;
    active_index = -1;

    // Find first matching schedule
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        Schedule* sched = configStore.getSchedule(i);
        if (sched == nullptr || !sched->active) continue;

        if (isTimeInRange(current_hour, current_min,
                          sched->time_from_hour, sched->time_from_min,
                          sched->time_to_hour, sched->time_to_min)) {
            active_schedule_found = true;
            active_index = i;
            current_open_temp = sched->open_temp;
            current_close_temp = sched->close_temp;
            break;  // First matching wins
        }
    }
}

bool Scheduler::hasActiveSchedule() {
    return active_schedule_found;
}

float Scheduler::getOpenTemp() {
    return current_open_temp;
}

float Scheduler::getCloseTemp() {
    return current_close_temp;
}

int8_t Scheduler::getActiveScheduleIndex() {
    return active_index;
}

bool Scheduler::isTimeInRange(uint8_t hour, uint8_t min,
                               uint8_t from_h, uint8_t from_m,
                               uint8_t to_h, uint8_t to_m) {
    uint16_t current = hour * 60 + min;
    uint16_t from = from_h * 60 + from_m;
    uint16_t to = to_h * 60 + to_m;

    if (from <= to) {
        // Normal range (e.g., 08:00 - 22:00)
        return current >= from && current < to;
    } else {
        // Overnight range (e.g., 22:00 - 06:00)
        return current >= from || current < to;
    }
}
