#include "relay_log.h"
#include <LittleFS.h>
#include <time.h>

RelayLog relayLog;

void RelayLog::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed");
        return;
    }

    if (LittleFS.exists(RELAY_LOG_FILE)) {
        loadHeader();
        if (header.magic != RELAY_LOG_MAGIC) {
            Serial.println("Invalid relay log, reinitializing");
            initializeFile();
        }
    } else {
        Serial.println("Creating new relay log file");
        initializeFile();
    }

    initialized = true;
    Serial.println("Relay log initialized");
}

void RelayLog::initializeFile() {
    memset(&header, 0, sizeof(header));
    header.magic = RELAY_LOG_MAGIC;
    header.head = 0;
    header.count = 0;
    header.current_hour = 255;  // Invalid, will be set on first update

    // Create file and write header
    File f = LittleFS.open(RELAY_LOG_FILE, "w");
    if (f) {
        f.write((uint8_t*)&header, sizeof(header));
        // Pre-allocate space for entries
        RelayLogEntry empty;
        memset(&empty, 0, sizeof(empty));
        for (uint16_t i = 0; i < MAX_LOG_ENTRIES; i++) {
            f.write((uint8_t*)&empty, sizeof(empty));
        }
        f.close();
    }
}

void RelayLog::loadHeader() {
    File f = LittleFS.open(RELAY_LOG_FILE, "r");
    if (f) {
        f.read((uint8_t*)&header, sizeof(header));
        f.close();
    }
}

void RelayLog::saveHeader() {
    File f = LittleFS.open(RELAY_LOG_FILE, "r+");
    if (f) {
        f.seek(0, SeekSet);
        f.write((uint8_t*)&header, sizeof(header));
        f.close();
    }
}

bool RelayLog::loadEntry(uint16_t file_index, RelayLogEntry& entry) {
    if (file_index >= MAX_LOG_ENTRIES) return false;

    File f = LittleFS.open(RELAY_LOG_FILE, "r");
    if (!f) return false;

    size_t offset = sizeof(header) + file_index * sizeof(RelayLogEntry);
    f.seek(offset, SeekSet);
    size_t read = f.read((uint8_t*)&entry, sizeof(entry));
    f.close();

    return read == sizeof(entry);
}

void RelayLog::saveEntry(uint16_t file_index, const RelayLogEntry& entry) {
    if (file_index >= MAX_LOG_ENTRIES) return;

    File f = LittleFS.open(RELAY_LOG_FILE, "r+");
    if (!f) return;

    size_t offset = sizeof(header) + file_index * sizeof(RelayLogEntry);
    f.seek(offset, SeekSet);
    f.write((uint8_t*)&entry, sizeof(entry));
    f.close();
}

void RelayLog::addEntry(uint32_t timestamp, float temp, bool state) {
    if (!initialized) return;

    RelayLogEntry entry;
    entry.timestamp = timestamp;
    entry.temp_x10 = (int16_t)(temp * 10.0f);
    entry.state = state ? 1 : 0;
    entry._pad = 0;

    // Save entry at head position
    saveEntry(header.head, entry);

    // Advance head (circular)
    header.head = (header.head + 1) % MAX_LOG_ENTRIES;

    // Increase count until full
    if (header.count < MAX_LOG_ENTRIES) {
        header.count++;
    }

    saveHeader();
}

void RelayLog::logEvent(bool state, float temp) {
    if (!initialized) return;

    time_t now;
    time(&now);

    // Update hourly stats for state change
    if (state) {
        // Turning ON - record start time
        header.current_hour_on_start = now;
    } else {
        // Turning OFF - calculate duration and add to bucket
        if (header.current_hour_on_start > 0) {
            uint32_t duration = now - header.current_hour_on_start;
            struct tm* tm_info = localtime(&now);
            uint8_t hour = tm_info->tm_hour;

            // Add duration to current hour bucket (capped at 3600)
            uint32_t new_total = header.hourly_buckets[hour].on_seconds + duration;
            if (new_total > 3600) new_total = 3600;
            header.hourly_buckets[hour].on_seconds = new_total;

            header.current_hour_on_start = 0;
        }
    }

    addEntry(now, temp, state);
}

void RelayLog::logPeriodic(bool current_state, float temp) {
    if (!initialized) return;

    time_t now;
    time(&now);

    addEntry(now, temp, current_state);
}

void RelayLog::updateHourlyStats(bool current_relay_state) {
    if (!initialized) return;

    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    uint8_t current_h = tm_info->tm_hour;

    // Check for hour rollover
    if (current_h != header.current_hour) {
        // If relay was ON, finalize the previous hour's duration
        if (header.current_hour_on_start > 0 && header.current_hour != 255) {
            // Calculate time from start to end of previous hour
            struct tm prev_end = *tm_info;
            prev_end.tm_min = 0;
            prev_end.tm_sec = 0;
            time_t hour_boundary = mktime(&prev_end);

            uint32_t duration = hour_boundary - header.current_hour_on_start;
            if (duration > 3600) duration = 3600;

            uint32_t new_total = header.hourly_buckets[header.current_hour].on_seconds + duration;
            if (new_total > 3600) new_total = 3600;
            header.hourly_buckets[header.current_hour].on_seconds = new_total;

            // Reset start time to beginning of new hour
            header.current_hour_on_start = hour_boundary;
        }

        // Clear the new hour bucket (it contains stale data from 24h ago)
        header.hourly_buckets[current_h].on_seconds = 0;
        header.current_hour = current_h;
    }

    // If relay is ON, update running duration in current bucket
    if (current_relay_state && header.current_hour_on_start > 0) {
        uint32_t elapsed = now - header.last_bucket_update;
        if (header.last_bucket_update > 0 && elapsed < 120) {  // Sanity check
            uint32_t new_total = header.hourly_buckets[current_h].on_seconds + elapsed;
            if (new_total > 3600) new_total = 3600;
            header.hourly_buckets[current_h].on_seconds = new_total;
        }
    }

    header.last_bucket_update = now;
    saveHeader();
}

uint32_t RelayLog::getOnSeconds(uint8_t hours) {
    if (!initialized) return 0;
    if (hours > 24) hours = 24;
    if (hours == 0) return 0;

    time_t now;
    time(&now);
    struct tm* tm_info = localtime(&now);
    uint8_t current_h = tm_info->tm_hour;

    uint32_t total = 0;
    for (uint8_t i = 0; i < hours; i++) {
        uint8_t bucket_idx = (current_h - i + 24) % 24;
        total += header.hourly_buckets[bucket_idx].on_seconds;
    }

    return total;
}

uint16_t RelayLog::getLogCount() {
    return initialized ? header.count : 0;
}

bool RelayLog::getLogEntry(uint16_t index, RelayLogEntry& entry) {
    if (!initialized || index >= header.count) return false;

    // Calculate actual file position (oldest entry first)
    uint16_t start;
    if (header.count < MAX_LOG_ENTRIES) {
        start = 0;  // Buffer not full yet
    } else {
        start = header.head;  // Oldest is at head (about to be overwritten)
    }
    uint16_t pos = (start + index) % MAX_LOG_ENTRIES;

    return loadEntry(pos, entry);
}

bool RelayLog::isRelayTrackedOn() {
    return header.current_hour_on_start > 0;
}
