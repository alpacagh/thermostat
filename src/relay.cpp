#include "relay.h"
#include "config.h"
#include "relay_log.h"
#include "sensor.h"

RelayControl relayControl;

void RelayControl::begin() {
    pinMode(PIN_RELAY, OUTPUT);
    setRelay(false);  // Start with relay off
}

void RelayControl::update(float temperature, float open_temp, float close_temp,
                          bool schedule_active, bool sensor_valid) {
    // Check timed override expiry
    if (override_duration > 0 && (millis() - override_start >= override_duration)) {
        clearOverride();
    }

    // Safety: hard upper temperature limit — overrides everything
    upper_limit_active = (sensor_valid && temperature >= UPPER_TEMP_LIMIT);
    if (upper_limit_active) {
        if (relay_on && canChangeState()) { setRelay(false); }
        return;
    }

    // Handle override
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
    override_start = 0;
    override_duration = 0;
}

void RelayControl::setOverride(OverrideState state, unsigned long duration_ms) {
    override_state = state;
    override_start = millis();
    override_duration = duration_ms;
}

void RelayControl::clearOverride() {
    override_state = OverrideState::NONE;
    override_start = 0;
    override_duration = 0;
}

unsigned long RelayControl::getOverrideRemaining() {
    if (override_state == OverrideState::NONE || override_duration == 0) return 0;
    unsigned long elapsed = millis() - override_start;
    if (elapsed >= override_duration) return 0;
    return override_duration - elapsed;
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

bool RelayControl::isUpperLimitActive() {
    return upper_limit_active;
}

unsigned long RelayControl::getLastChangeTime() {
    return last_change_time;
}

void RelayControl::setRelay(bool on) {
    if (on != relay_on) {  // Only log actual state changes
        relay_on = on;
        digitalWrite(PIN_RELAY, on ? HIGH : LOW);
        last_change_time = millis();

        // Log the relay state change with current temperature
        relayLog.logEvent(on, tempSensor.getTemperature());
    } else {
        relay_on = on;
        digitalWrite(PIN_RELAY, on ? HIGH : LOW);
        last_change_time = millis();
    }
}

bool RelayControl::canChangeState() {
    if (last_change_time == 0) return true;  // First change always allowed
    return (millis() - last_change_time) >= MIN_RELAY_CYCLE_MS;
}

unsigned long parseDurationMs(const char* str) {
    if (!str || *str == '\0') return 0;

    // Split by ':' and count parts
    int parts[3] = {0, 0, 0};
    int count = 0;
    const char* p = str;

    while (*p && count < 3) {
        parts[count] = atoi(p);
        if (parts[count] < 0) return 0;
        count++;
        while (*p && *p != ':') p++;
        if (*p == ':') p++;
    }

    if (count == 0) return 0;

    unsigned long hours = 0, minutes = 0, seconds = 0;
    if (count == 1) {
        // mm
        minutes = parts[0];
    } else if (count == 2) {
        // hh:mm
        hours = parts[0];
        minutes = parts[1];
    } else {
        // hh:mm:ss
        hours = parts[0];
        minutes = parts[1];
        seconds = parts[2];
    }

    return (hours * 3600UL + minutes * 60UL + seconds) * 1000UL;
}
