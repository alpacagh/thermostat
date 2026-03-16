#include "display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Display display;

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

bool Display::begin() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        return false;
    }

    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    initialized = true;
    return true;
}

void Display::showBootMessage(const char* message) {
    if (!initialized) return;

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setCursor(0, 28);
    oled.print(message);
    oled.display();
}

void Display::showStatus(float temp, float humidity, bool relay_on, bool overridden,
                         uint8_t hour, uint8_t min, float open_temp, float close_temp,
                         bool wifi_connected, const char* ip_address, bool sensor_valid,
                         unsigned long override_remaining_sec) {
    if (!initialized) return;

    oled.clearDisplay();
    oled.setTextSize(1);

    // Row 1: Time and relay status
    oled.setCursor(0, 0);
    char time_buf[8];
    snprintf(time_buf, sizeof(time_buf), "%02u:%02u", (unsigned)hour, (unsigned)min);
    oled.print(time_buf);

    oled.setCursor(48, 0);
    if (relay_on) {
        oled.print("HEAT ON");
        if (overridden) oled.print("*");
    } else {
        oled.print("HEAT OFF");
        if (overridden) oled.print("*");
    }

    // Row 2: Temperature (larger)
    oled.setTextSize(2);
    oled.setCursor(0, 14);
    if (sensor_valid) {
        char temp_buf[10];
        dtostrf(temp, 4, 1, temp_buf);
        oled.print(temp_buf);
        oled.setTextSize(1);
        oled.print(" C");
    } else {
        oled.print("ERR");
    }

    // Row 3: Humidity
    oled.setTextSize(1);
    oled.setCursor(80, 14);
    if (sensor_valid) {
        char hum_buf[8];
        dtostrf(humidity, 4, 1, hum_buf);
        oled.print(hum_buf);
        oled.print("%");
    }

    // Row 4: Temperature margins or override countdown
    oled.setCursor(0, 36);
    if (overridden) {
        oled.print("OVR: ");
        if (override_remaining_sec > 0) {
            unsigned long h = override_remaining_sec / 3600;
            unsigned long m = (override_remaining_sec % 3600) / 60;
            unsigned long s = override_remaining_sec % 60;
            char ovr_buf[16];
            snprintf(ovr_buf, sizeof(ovr_buf), "%lu:%02lu:%02lu", h, m, s);
            oled.print(ovr_buf);
        } else {
            oled.print("indefinite");
        }
    } else {
        oled.print("Set: ");
        if (open_temp > 0 || close_temp > 0) {
            char margin_buf[16];
            dtostrf(open_temp, 3, 1, margin_buf);
            oled.print(margin_buf);
            oled.print("-");
            dtostrf(close_temp, 3, 1, margin_buf);
            oled.print(margin_buf);
        } else {
            oled.print("--");
        }
    }

    // Row 5: Connection status
    oled.setCursor(0, 50);
    if (wifi_connected) {
        oled.print("WiFi: ");
        oled.print(ip_address);
    } else {
        oled.print("WiFi: disconnected");
    }

    oled.display();
}
