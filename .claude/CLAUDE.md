# Thermostat Project

ESP8266 (NodeMCU Amica) thermostat with WiFi connectivity, temperature-based relay control, OLED display, web dashboard, and TCP command interface. Built with PlatformIO + Arduino framework.

## Architecture

The main loop (`src/main.cpp`) orchestrates all components via periodic polling using `millis()`. No RTOS — cooperative multitasking with `yield()` at end of each loop iteration.

**Priority hierarchy for relay control:**
1. Upper temperature limit — relay OFF if temperature >= 30°C (`UPPER_TEMP_LIMIT`), cannot be bypassed
2. Override (timed or indefinite) — bypasses schedule and hysteresis logic
3. Fail-safe — relay OFF if sensor invalid or no active schedule
4. Hysteresis control — based on temperature thresholds from active schedule

## Components & Files

| Component | Files | Description |
|-----------|-------|-------------|
| **Main loop** | `src/main.cpp` | Orchestrates all components, timing intervals |
| **Config** | `src/config.h` | Pin definitions, timing constants (`SENSOR_READ_INTERVAL`, etc.) |
| **Config Store** | `src/config_store.h/cpp` | EEPROM persistence for WiFi credentials, schedules, timezone |
| **Sensor** | `src/sensor.h/cpp` | DHT22 temperature/humidity reading |
| **Relay** | `src/relay.h/cpp` | Relay control with hysteresis, override (timed/indefinite), `parseDurationMs()` duration parser |
| **Scheduler** | `src/scheduler.h/cpp` | Time-based schedule evaluation (up to 8 slots) |
| **Display** | `src/display.h/cpp` | SSD1306 128x64 OLED status display |
| **Network** | `src/network.h/cpp` | WiFi management (auto-reconnect with exponential backoff), NTP time sync, TCP command server on port 8266 |
| **Web Server** | `src/webserver.h/cpp` | HTTP REST API on port 80 |
| **Web UI** | `src/webpage.h` | Single-page HTML/JS dashboard stored in PROGMEM |
| **Relay Log** | `src/relay_log.h/cpp` | LittleFS relay state logging with hourly statistics |
| **Serial Config** | `src/serial_config.h/cpp` | Serial-based initial WiFi/timezone setup (hold during boot) |

## Build & Flash

```bash
pio run                  # Build for ESP8266
pio run -t upload        # Build and flash (auto-detects USB serial)
pio test -e native       # Run unit tests on host (no device needed)
pio device monitor       # Serial monitor at 115200 baud
```

The `native` env in `platformio.ini` is test-only (Unity framework). It cannot build the full firmware (no Arduino.h on host).

## Testing

### Unit Tests (no device needed)

```bash
pio test -e native
```

Tests live in `test/` directory. Currently covers `parseDurationMs()` duration parser.

### On-Device Testing via TCP (port 8266)

Pattern: `{ printf "COMMAND\n"; sleep 1; } | nc -w3 <DEVICE_IP> 8266`

```bash
# Identify device
{ printf "WHOAMI\n"; sleep 1; } | nc -w3 192.168.1.17 8266
# → OK THERMOSTAT,v1.0

# Get status
{ printf "STATUS\n"; sleep 1; } | nc -w3 192.168.1.17 8266
# → OK temp=20.9 humidity=38.0 relay=OFF override=NO override_remaining=0 upper_limit=NO schedule=0

# Override relay (indefinite)
{ printf "OVERRIDE ON\n"; sleep 1; } | nc -w3 192.168.1.17 8266

# Override relay (timed: 30 min)
{ printf "OVERRIDE ON 30\n"; sleep 1; } | nc -w3 192.168.1.17 8266

# Override relay (timed: 1h30m)
{ printf "OVERRIDE ON 1:30\n"; sleep 1; } | nc -w3 192.168.1.17 8266

# Clear override
{ printf "OVERRIDE CLEAR\n"; sleep 1; } | nc -w3 192.168.1.17 8266

# List schedules
{ printf "GET_SCHEDULES\n"; sleep 1; } | nc -w3 192.168.1.17 8266

# Set schedule: slot 0, 08:00-22:00, heat ON below 20.0, OFF above 22.0
{ printf "SET_SCHEDULE 0 08:00 22:00 20.0 22.0\n"; sleep 1; } | nc -w3 192.168.1.17 8266

# Get relay statistics
{ printf "STATS\n"; sleep 1; } | nc -w3 192.168.1.17 8266
```

### On-Device Testing via HTTP API (port 80)

```bash
# Status
curl -s http://192.168.1.17/api/status | python3 -m json.tool

# Set timed override
curl -s -X POST http://192.168.1.17/api/override \
  -d '{"state":"on","duration":"30"}'

# Clear override
curl -s -X POST http://192.168.1.17/api/override \
  -d '{"state":"clear"}'

# Get schedules
curl -s http://192.168.1.17/api/schedules

# Set schedule
curl -s -X POST http://192.168.1.17/api/schedule \
  -d '{"index":0,"from":"08:00","to":"22:00","open":20.0,"close":22.0}'

# Get relay statistics
curl -s http://192.168.1.17/api/stats
```

### Web UI

Open `http://<DEVICE_IP>/` in a browser. Dashboard auto-refreshes every 5 seconds.

## Code Conventions

- **JSON responses**: Use stack-allocated `char[]` + `snprintf`, not `String` concatenation (prevents heap fragmentation on ESP8266's ~40KB heap)
- **Timing**: All interval constants in `src/config.h`. Use local `unsigned long now = millis()` variable, not repeated `millis()` calls
- **Main loop**: Ends with `yield()` for ESP8266 cooperative multitasking. Do NOT use `ESP.wdtFeed()` — `yield()` already feeds the software WDT
- **WiFi reconnection**: Exponential backoff (30s → 5min cap), non-blocking time sync after reconnect (periodic NTP handles retries)
- **Override duration format**: `[[hh:]mm[:ss]]` — single value = minutes, two parts = hh:mm, three parts = hh:mm:ss
