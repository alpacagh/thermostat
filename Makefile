# Thermostat Firmware Makefile

# Configuration
DEVICE_PORT ?= /dev/cu.usbserial-0001
BAUD_RATE ?= 115200
TCP_PORT ?= 8266
UDP_PORT ?= 8267
BROADCAST ?= 255.255.255.255

# Device IP (set after discovery, or override: make telnet DEVICE_IP=192.168.1.100)
DEVICE_IP ?=

.PHONY: build upload monitor discover telnet clean help

## Build firmware
build:
	pio run

## Upload firmware to device
upload:
	pio run -t upload

## Connect to serial TTY (LF line endings)
monitor:
	pio device monitor --eol LF

## Alternative: minicom with LF mode
minicom:
	@echo "Connecting with minicom..."
	minicom -D $(DEVICE_PORT) -b $(BAUD_RATE)

## Discover device on LAN (UDP broadcast)
discover:
	@echo "Sending discovery broadcast to $(BROADCAST):$(UDP_PORT)..."
	@python3 -c "\
import socket; \
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); \
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1); \
s.settimeout(3); \
s.sendto(b'DISCOVER', ('$(BROADCAST)', $(UDP_PORT))); \
print('Waiting for response...'); \
resp, addr = s.recvfrom(1024); \
print(f'Found: {resp.decode()} from {addr[0]}'); \
" 2>/dev/null || echo "No device found. Check WiFi config or try: make discover BROADCAST=192.168.1.255"

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
	@IP=$$(python3 -c "\
import socket; \
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); \
s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1); \
s.settimeout(3); \
s.sendto(b'DISCOVER', ('$(BROADCAST)', $(UDP_PORT))); \
resp, addr = s.recvfrom(1024); \
print(resp.decode().split(',')[1]); \
" 2>/dev/null) && \
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
	@echo "  BROADCAST=$(BROADCAST)"
	@echo "  TCP_PORT=$(TCP_PORT)"
