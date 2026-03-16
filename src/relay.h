#ifndef RELAY_H
#define RELAY_H

#include <Arduino.h>

enum class OverrideState {
    NONE,
    FORCE_ON,
    FORCE_OFF
};

class RelayControl {
public:
    void begin();
    void update(float temperature, float open_temp, float close_temp, bool schedule_active, bool sensor_valid);

    void setOverride(OverrideState state);
    void setOverride(OverrideState state, unsigned long duration_ms);
    void clearOverride();
    OverrideState getOverride();
    unsigned long getOverrideRemaining();

    bool isOn();
    bool isOverridden();
    unsigned long getLastChangeTime();

private:
    bool relay_on = false;
    OverrideState override_state = OverrideState::NONE;
    unsigned long last_change_time = 0;
    unsigned long override_start = 0;
    unsigned long override_duration = 0;  // 0 = indefinite

    void setRelay(bool on);
    bool canChangeState();
};

// Parses "[[hh:]mm[:ss]]" duration string to milliseconds. Returns 0 on failure.
unsigned long parseDurationMs(const char* str);

extern RelayControl relayControl;

#endif
