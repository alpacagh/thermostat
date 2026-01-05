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
    void clearOverride();
    OverrideState getOverride();

    bool isOn();
    bool isOverridden();
    unsigned long getLastChangeTime();

private:
    bool relay_on = false;
    OverrideState override_state = OverrideState::NONE;
    unsigned long last_change_time = 0;

    void setRelay(bool on);
    bool canChangeState();
};

extern RelayControl relayControl;

#endif
