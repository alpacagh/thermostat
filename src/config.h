#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pin definitions (ESP32-C3 nano)
#define PIN_OLED_SCL    9   // GPIO9
#define PIN_OLED_SDA    8   // GPIO8
#define PIN_DHT         10  // GPIO10
#define PIN_RELAY       7   // GPIO7

// DHT sensor type
#define DHT_TYPE        DHT22

// OLED display
#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_ADDRESS    0x3C

// Timing constants (milliseconds)
#define SENSOR_READ_INTERVAL    2000
#define SCHEDULE_CHECK_INTERVAL 60000
#define NTP_SYNC_INTERVAL       3600000
#define DISPLAY_UPDATE_INTERVAL 500
#define MIN_RELAY_CYCLE_MS      5000  // 5 seconds
#define RELAY_LOG_UPDATE_INTERVAL   60000   // 1 minute - hourly stats update
#define RELAY_LOG_PERIODIC_INTERVAL 600000  // 10 minutes - periodic log entry
#define HEAP_CHECK_INTERVAL        300000  // 5 minutes - heap memory monitoring
#define WIFI_CHECK_INTERVAL        30000   // 30 seconds - WiFi reconnection check
#define WIFI_CHECK_INTERVAL_MAX    300000  // 5 minutes - max backoff for WiFi reconnection
// attempt to workaround my router problem with lan access
#define DHCP_RENEW_INTERVAL        180000 // 3 minutes - periodic DHCP lease renewal

// Safety limits
#define UPPER_TEMP_LIMIT       30.0f   // Relay OFF at or above this temperature

// Network ports
#define TCP_SERVER_PORT     8266

// EEPROM
#define MAX_SCHEDULES       8
#define WIFI_SSID_MAX_LEN   32
#define WIFI_PASS_MAX_LEN   64

// NTP
#define NTP_SERVER          "pool.ntp.org"

// PlantUML
#define PLANTUML_DEFAULT_SERVER "http://www.plantuml.com/plantuml/svg/"

// Discovery response
#define DEVICE_TYPE         "thermostat"
#define PROTOCOL_VERSION    "1.0"

#endif
