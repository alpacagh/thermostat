# Thermostat Wiring Diagram

## Components
- ESP32-C3 nano board
- GM009805V4.2 OLED Display (SSD1306, I2C)
- Aosong AM2302 Temperature/Humidity Sensor
- Solid State Relay (SSR) 3-32VDC / 220VAC
- 10KΩ Resistor (pull-up for AM2302)

---

## Board Pinout

| Pin label  | Role     | Alt label |
| ---------- | -------- | --------- |
| Left side  |          |           |
| V5         | 5V input |           |
| GD         | GND      |           |
| V3         | 3V3      |           |
| RX         | GPIO20   | RX        |
| TX         | GPIO21   | TX        |
| 2          | GPIO2    | A2        |
| 1          | GPIO1    | A1        |
| 0          | GPIO0    | A0        |
| Right side |          |           |
| 10         | GPIO10   |           |
| 9          | GPIO9    | SCL       |
| 8          | GPIO8    | SDA       |
| 7          | GPIO7    | SS        |
| 6          | GPIO6    | MOSI      |
| 5          | GPIO5    | MISO      |
| 4          | GPIO4    | A4        |
| 3          | GPIO3    | A3        |

---

## Wiring Diagram

### DC Side — MCU, Sensor, Display, Relay Input

```plantuml
@startuml
title DC Wiring — ESP32-C3 Thermostat
left to right direction

map "ESP32-C3 nano" as mcu {
  V5 => 5V bus
  GD => GND bus
  GPIO7 => Relay DATA
  GPIO8 => OLED SDA
  GPIO9 => OLED SCL
  GPIO10 => DHT DATA
  USB => Power Input
}

map "OLED Display\n(GM009805V4.2)" as oled {
  VCC => 5V bus
  GND => GND bus
  SCL => ESP GPIO9
  SDA => ESP GPIO8
}

map "AM2302\n(DHT22 Sensor)" as dht {
  VCC => 5V bus
  DATA => ESP GPIO10
  NC => not connected
  GND => GND bus
}

map "10K Resistor\n(Pull-up)" as pullup {
  A => 5V bus
  B => DHT DATA
}

map "Coil Relay\nModule" as relay {
  VCC => 5V bus
  GND => GND bus
  DATA => ESP GPIO7
}

mcu::GPIO9 --> oled::SCL
mcu::GPIO8 --> oled::SDA
mcu::V5 --> oled::VCC
mcu::GD --> oled::GND

mcu::GPIO10 --> dht::DATA
mcu::V5 --> dht::VCC
mcu::GD --> dht::GND

pullup::A --> mcu::V5
pullup::B --> dht::DATA

mcu::GPIO7 --> relay::DATA
mcu::V5 --> relay::VCC
mcu::GD --> relay::GND

@enduml
```

### AC Side — Relay Output to Heater (220VAC)

```plantuml
@startuml
title AC Wiring — SSR to Heater (220VAC)
left to right direction

map "Mains\n220VAC" as mains {
  L => Live
  N => Neutral
}

map "Coil Relay\n(AC Side)" as relay {
  COM => from Live
  NO => to Heater
}

map "Heater" as heater {
  HOT => from Relay
  NEUTRAL => to Mains N
}

map "AC-DC Module\n(5V PSU)" as acdc {
  AC_L => from Live
  AC_N => from Neutral
  DC_5V => to ESP32 V5
  DC_GND => to ESP32 GD
}

mains::L --> relay::COM
relay::NO --> heater::HOT
heater::NEUTRAL --> mains::N
mains::L --> acdc::AC_L
mains::N --> acdc::AC_N

note bottom of mains
  DANGER: 220VAC MAINS VOLTAGE
  Disconnect power before wiring
end note

@enduml
```

---

## Connection Tables

### OLED Display (I2C)
| OLED Pin | ESP32-C3 Pin | GPIO  |
|----------|--------------|-------|
| GND      | GD           | -     |
| VCC      | V5 (5V)      | -     |
| SCL      | 9            | GPIO9 |
| SDA      | 8            | GPIO8 |

### AM2302 Temperature Sensor
| AM2302 Pin  | ESP32-C3 Pin | GPIO   | Notes |
|-------------|--------------|--------|-------|
| Pin 1 (VCC) | V5 (5V)      | -      | Power |
| Pin 2 (DATA)| 10           | GPIO10 | + 10KΩ pull-up to 5V |
| Pin 3 (NC)  | -            | -      | Not connected |
| Pin 4 (GND) | GD           | -      | Ground |

### Solid State Relay (SSR)
| SSR Terminal | Connection     |
|--------------|----------------|
| DC+ (input)  | 7 (GPIO7)     |
| DC- (input)  | GD (GND)      |
| AC Load      | Heater in series with mains |

---

## Pin Summary

| Board Pin | GPIO   | Function        |
|-----------|--------|-----------------|
| 9         | GPIO9  | OLED SCL (I2C)  |
| 8         | GPIO8  | OLED SDA (I2C)  |
| 10        | GPIO10 | AM2302 Data     |
| 7         | GPIO7  | SSR Control     |
| V5        | -      | Power (OLED, AM2302) |
| GD        | -      | Common Ground   |

---

## Notes

1. **Pull-up Resistor**: The 10KΩ resistor between AM2302 DATA pin and 5V is required for reliable communication.

2. **SSR Input**: The SSR accepts 3-32VDC, so the 3.3V GPIO output from ESP32-C3 is sufficient to trigger it.

3. **Power**: ESP32-C3 nano can be powered via USB.

4. **I2C Address**: The OLED typically uses address `0x3C`. Run an I2C scanner if display doesn't work.

5. **Pin grouping**: All signal pins (GPIO7-10) are on the right side of the board, minimizing wiring complexity.

---

## Safety Warnings

⚠️ **MAINS VOLTAGE (220VAC)**
- Disconnect power before wiring the AC side
- Use properly rated wires for AC current
- Ensure proper insulation and enclosure
- The SSR should be rated for your heater's current draw
- Consider adding a fuse on the AC side
