# Thermostat Firmware Makefile

# Configuration
DEVICE_PORT ?= /dev/cu.usbserial-0001
BAUD_RATE ?= 115200
TCP_PORT ?= 8266

# Device IP (set after discovery, or override: make telnet DEVICE_IP=192.168.1.100)
DEVICE_IP ?=

.PHONY: build upload monitor discover telnet clean help

## Build firmware
build:
	pio run -e esp32c3

## Upload firmware to device
upload:
	pio run -t upload -e esp32c3

## Connect to serial TTY (LF line endings)
monitor:
	pio device monitor --eol LF -e esp32c3

## Alternative: minicom with LF mode
minicom:
	@echo "Connecting with minicom..."
	minicom -D $(DEVICE_PORT) -b $(BAUD_RATE)

## Discover device on LAN (TCP scan)
discover:
	@python3 tools/discover.py

## Connect to device control port (set DEVICE_IP first)
telnet:
ifndef DEVICE_IP
	@echo "Error: DEVICE_IP not set"
	@echo "Usage: make telnet DEVICE_IP=192.168.x.x"
	@echo "Or run 'make discover' first to find the IP"
	@exit 1
endif
	@echo "Connecting to $(DEVICE_IP):$(TCP_PORT)..."
	@echo "Commands: STATUS, GET_SCHEDULES, SET_SCHEDULE, OVERRIDE ON|OFF|CLEAR"
	@echo "Press Ctrl+C to exit"
	nc $(DEVICE_IP) $(TCP_PORT)

## Interactive session: discover then connect
connect:
	@echo "Discovering device..."
	@IP=$$(python3 tools/discover.py 2>/dev/null | grep -m1 "^Found:" | awk '{print $$2}') && \
	if [ -n "$$IP" ]; then \
		echo "Found device at $$IP"; \
		echo "Connecting..."; \
		nc $$IP $(TCP_PORT); \
	else \
		echo "No device found"; \
	fi

## Send a single command (use: make cmd DEVICE_IP=x.x.x.x CMD="STATUS")
cmd:
ifndef DEVICE_IP
	@echo "Usage: make cmd DEVICE_IP=192.168.x.x CMD=\"STATUS\""
	@exit 1
endif
ifndef CMD
	@echo "Usage: make cmd DEVICE_IP=192.168.x.x CMD=\"STATUS\""
	@exit 1
endif
	@(echo "$(CMD)"; sleep 1) | nc $(DEVICE_IP) $(TCP_PORT)

## Clean build files
clean:
	pio run -t clean

## Show help
help:
	@echo "Thermostat Makefile Commands:"
	@echo ""
	@echo "  make build      - Build firmware"
	@echo "  make upload     - Upload firmware to device"
	@echo "  make monitor    - Connect to serial TTY (LF mode)"
	@echo "  make discover   - Find device IP on LAN"
	@echo "  make telnet DEVICE_IP=x.x.x.x  - Connect to control port"
	@echo "  make connect    - Discover and connect automatically"
	@echo "  make cmd DEVICE_IP=x.x.x.x CMD=\"STATUS\" - Send single command"
	@echo "  make clean      - Clean build files"
	@echo ""
	@echo "Configuration:"
	@echo "  DEVICE_PORT=$(DEVICE_PORT)"
	@echo "  TCP_PORT=$(TCP_PORT)"
