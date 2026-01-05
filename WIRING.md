# Thermostat Wiring Diagram

## Components
- NodeMCU Amica (ESP8266MOD)
- GM009805V4.2 OLED Display (SSD1306, I2C)
- Aosong AM2302 Temperature/Humidity Sensor
- Solid State Relay (SSR) 3-32VDC / 220VAC
- 10KΩ Resistor (pull-up for AM2302)

---

## Wiring Diagram

```
                                    +---------------------+
                                    |    OLED DISPLAY     |
                                    |   (GM009805V4.2)    |
                                    |                     |
                                    | GND VCC SCL SDA     |
                                    +--+---+---+---+------+
                                       |   |   |   |
                                       |   |   |   |
        +------------------------------+   |   |   +------------------+
        |   +--------------------------+   |   +---------------+      |
        |   |                              |                   |      |
        |   |      +3.3V +-----------------+                   |      |
        |   |        |                                         |      |
        |   |        +----------------+------------------------+------+----+
        |   |        |                |                        |      |    |
        |   |        |   +------------+----+                   |      |    |
        |   |        |   |            |    |                   |      |    |
      +-+---+--------+---+------------+----+-------------------+------+----+--+
      | GND 3V3     D8  D7  D6  D5   D4  D3  D2  D1  D0  A0   RSV   3V3  GND |
      |                      |   |        |   |                              |
      |      NodeMCU Amica   |   |        |   |                              |
      |      (ESP8266MOD)    |   |        |   |                              |
      |                      |   |        |   |                              |
      | Vin GND RST EN  3V3 ... USB ...  TX  RX  GND  3V3                    |
      +----------------------+---+--------+---+------------------------------+
                             |   |        |   |
                             |   |        |   +---- SCL (GPIO5) --> OLED
                             |   |        +-------- SDA (GPIO4) --> OLED
                             |   |
                             |   +--- DATA (GPIO14) <-- AM2302
                             |                  |
                             |                 +-+
                             |                 | | 10KΩ
                             |                 | | Pull-up
                             |                 +-+
                             |                  |
                             |           +------+
                             |           |
                             +--- SSR DC+ (GPIO12)


     +---------------+                +--------------+
     |   AM2302      |                |     SSR      |
     | (DHT22 Sensor)|                | Solid State  |
     |               |                |    Relay     |
     | 1   2   3   4 |                |              |
     +--+---+---+--+-+                | DC+  DC-     |
        |   |   |  |                  +--+----+------+
        |   |   NC |                     |    |
      3.3V  |      GND                   |   GND
            |                            |
            +--- to D5 (GPIO14)          +--- from D6 (GPIO12)
            |
           +-+
           | | 10KΩ Pull-up
           +-+
            |
          3.3V


     SSR AC OUTPUT SIDE (220VAC):
     +------------------------------------------+
     |                                          |
     |  L (Live) ----+                          |
     |               |     +--------+           |
     |              SSR    | HEATER |           |
     |               |     +---+----+           |
     |               +---------|                |
     |                         |                |
     |  N (Neutral) -----------+                |
     |                                          |
     +------------------------------------------+
     ⚠️  DANGER: 220VAC MAINS VOLTAGE
```

---

## Connection Tables

### OLED Display (I2C)
| OLED Pin | NodeMCU Pin | GPIO |
|----------|-------------|------|
| GND      | GND         | -    |
| VCC      | 3.3V        | -    |
| SCL      | D1          | GPIO5|
| SDA      | D2          | GPIO4|

### AM2302 Temperature Sensor
| AM2302 Pin | NodeMCU Pin | GPIO   | Notes |
|------------|-------------|--------|-------|
| Pin 1 (VCC)| 3.3V        | -      | Power |
| Pin 2 (DATA)| D5         | GPIO14 | + 10KΩ pull-up to 3.3V |
| Pin 3 (NC) | -           | -      | Not connected |
| Pin 4 (GND)| GND         | -      | Ground |

### Solid State Relay (SSR)
| SSR Terminal | Connection |
|--------------|------------|
| DC+ (input)  | D6 (GPIO12)|
| DC- (input)  | GND        |
| AC Load      | Heater in series with mains |

---

## Pin Summary

| NodeMCU | GPIO   | Function        |
|---------|--------|-----------------|
| D1      | GPIO5  | OLED SCL (I2C)  |
| D2      | GPIO4  | OLED SDA (I2C)  |
| D5      | GPIO14 | AM2302 Data     |
| D6      | GPIO12 | SSR Control     |
| 3.3V    | -      | Power (OLED, AM2302) |
| GND     | -      | Common Ground   |

---

## Notes

1. **Pull-up Resistor**: The 10KΩ resistor between AM2302 DATA pin and 3.3V is required for reliable communication.

2. **SSR Input**: The SSR accepts 3-32VDC, so the 3.3V GPIO output from ESP8266 is sufficient to trigger it.

3. **Power**: NodeMCU can be powered via USB or 5V to Vin pin.

4. **I2C Address**: The OLED typically uses address `0x3C`. Run an I2C scanner if display doesn't work.

---

## Safety Warnings

⚠️ **MAINS VOLTAGE (220VAC)**
- Disconnect power before wiring the AC side
- Use properly rated wires for AC current
- Ensure proper insulation and enclosure
- The SSR should be rated for your heater's current draw
- Consider adding a fuse on the AC side
