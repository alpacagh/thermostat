# Thermostat Project - Hardware Components

## MCU Development Board
- **Board**: ESP32-C3 nano
- **MCU**: ESP32-C3
- **Connectivity**: WiFi + Bluetooth 5.0 (BLE)

## Display
- **Board**: GM009805V4.2
- **Type**: OLED display

## Temperature Sensor
- **Model**: Aosong AM2302
- **Type**: Digital temperature and humidity sensor (DHT22 compatible)

## Relay Module
- **Type**: Solid State Relay (SSR)
- **Input**: 3-32VDC
- **Output**: 220VAC

## Power Source
- **Type**: USB power supply for ESP and relay

---

# Project Goals

## Purpose
Create a room thermostat system that automatically controls a heater relay to maintain a set air temperature.

## Deliverables
1. **Wiring diagram** - See [WIRING.md](WIRING.md)
2. **Firmware requirements** - See [REQUIREMENTS.md](REQUIREMENTS.md)
3. **Firmware** - PlatformIO project in `src/`

---

# Documentation

| File | Description |
|------|-------------|
| [PROJECT_INFO.md](PROJECT_INFO.md) | Hardware components and project overview |
| [WIRING.md](WIRING.md) | Connection diagram and pin mappings |
| [REQUIREMENTS.md](REQUIREMENTS.md) | Firmware requirements specification |
