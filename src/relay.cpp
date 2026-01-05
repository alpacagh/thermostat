#include "relay.h"
#include "config.h"

RelayControl relayControl;

void RelayControl::begin() {
    pinMode(PIN_RELAY, OUTPUT);
    setRelay(false);  // Start with relay off
}

void RelayControl::update(float temperature, float open_temp, float close_temp,
                          bool schedule_active, bool sensor_valid) {
    // Handle override first
    if (override_state == OverrideState::FORCE_ON) {
        if (!relay_on && canChangeState()) {
            setRelay(true);
        }
        return;
    }
    if (override_state == OverrideState::FORCE_OFF) {
        if (relay_on && canChangeState()) {
            setRelay(false);
        }
        return;
    }

    // Fail-safe: turn off if sensor invalid or no schedule
    if (!sensor_valid || !schedule_active) {
        if (relay_on && canChangeState()) {
            setRelay(false);
        }
        return;
    }

    // Normal hysteresis control
    bool should_be_on = relay_on;

    if (temperature < open_temp) {
        should_be_on = true;
    } else if (temperature > close_temp) {
        should_be_on = false;
    }
    // Between open_temp and close_temp: maintain current state

    if (should_be_on != relay_on && canChangeState()) {
        setRelay(should_be_on);
    }
}

void RelayControl::setOverride(OverrideState state) {
    override_state = state;
}

void RelayControl::clearOverride() {
    override_state = OverrideState::NONE;
}

OverrideState RelayControl::getOverride() {
    return override_state;
}

bool RelayControl::isOn() {
    return relay_on;
}

bool RelayControl::isOverridden() {
    return override_state != OverrideState::NONE;
}

unsigned long RelayControl::getLastChangeTime() {
    return last_change_time;
}

void RelayControl::setRelay(bool on) {
    relay_on = on;
    digitalWrite(PIN_RELAY, on ? HIGH : LOW);
    last_change_time = millis();
}

bool RelayControl::canChangeState() {
    if (last_change_time == 0) return true;  // First change always allowed
    return (millis() - last_change_time) >= MIN_RELAY_CYCLE_MS;
}
