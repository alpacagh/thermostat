#ifndef RELAY_LOG_H
#define RELAY_LOG_H

#include <Arduino.h>

#define MAX_LOG_ENTRIES 1000
#define RELAY_LOG_MAGIC 0x524C4F47  // "RLOG"
#define RELAY_LOG_FILE "/relay_log.dat"

// Log entry: 8 bytes each
struct RelayLogEntry {
    uint32_t timestamp;  // Unix epoch
    int16_t temp_x10;    // Temperature * 10 (e.g., 235 = 23.5C)
    uint8_t state;       // 0=OFF, 1=ON
    uint8_t _pad;        // Alignment padding
};

// Hourly bucket for statistics: 2 bytes each
struct HourlyBucket {
    uint16_t on_seconds;  // 0-3600
};

// Header stored at beginning of SPIFFS file
struct RelayLogHeader {
    uint32_t magic;
    uint16_t head;                       // Circular buffer head index
    uint16_t count;                      // Number of entries (0-1000)
    uint8_t current_hour;                // Last processed hour (0-23)
    uint8_t _pad[3];                     // Alignment
    HourlyBucket hourly_buckets[24];     // Rolling 24h stats
    uint32_t current_hour_on_start;      // Epoch when relay turned ON in current hour (0 if OFF)
    uint32_t last_bucket_update;         // Last time buckets were updated
};

class RelayLog {
public:
    void begin();

    // Log a relay state change with temperature
    void logEvent(bool state, float temp);

    // Log periodic entry (every 10 min) with current state
    void logPeriodic(bool current_state, float temp);

    // Update hourly statistics buckets (call every minute)
    void updateHourlyStats(bool current_relay_state);

    // Get ON duration in seconds for last N hours (1, 6, 12, or 24)
    uint32_t getOnSeconds(uint8_t hours);

    // Get log entry count
    uint16_t getLogCount();

    // Get log entry by index (0 = oldest)
    bool getLogEntry(uint16_t index, RelayLogEntry& entry);

    // Check if relay is currently tracked as ON
    bool isRelayTrackedOn();

private:
    RelayLogHeader header;
    bool initialized = false;

    void loadHeader();
    void saveHeader();
    void addEntry(uint32_t timestamp, float temp, bool state);
    bool loadEntry(uint16_t file_index, RelayLogEntry& entry);
    void saveEntry(uint16_t file_index, const RelayLogEntry& entry);
    void initializeFile();
};

extern RelayLog relayLog;

#endif
