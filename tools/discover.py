#!/usr/bin/env python3
"""
TCP-based thermostat discovery tool.
Scans the local subnet for devices responding to WHOAMI command.
"""

import socket
import ipaddress
import concurrent.futures
import sys

TCP_PORT = 8266
TIMEOUT = 0.5
MAX_WORKERS = 50


def get_local_subnet():
    """Auto-detect the local subnet by finding the default gateway interface."""
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        local_ip = s.getsockname()[0]
    finally:
        s.close()

    network = ipaddress.ip_network(f"{local_ip}/24", strict=False)
    return network, local_ip


def probe_device(ip: str) -> tuple:
    """
    Attempt to connect to IP on TCP_PORT and send WHOAMI command.
    Returns (ip, response) if successful, (ip, None) if failed.
    """
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(TIMEOUT)
        sock.connect((ip, TCP_PORT))
        sock.sendall(b"WHOAMI\n")
        response = sock.recv(256).decode().strip()
        sock.close()
        return (ip, response)
    except (socket.timeout, ConnectionRefusedError, OSError):
        return (ip, None)


def main():
    try:
        network, local_ip = get_local_subnet()
    except Exception as e:
        print(f"Error detecting network: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"Local IP: {local_ip}")
    print(f"Scanning subnet: {network} on port {TCP_PORT}...")
    print()

    hosts = [str(ip) for ip in network.hosts()]

    found_devices = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_WORKERS) as executor:
        futures = {executor.submit(probe_device, ip): ip for ip in hosts}
        for future in concurrent.futures.as_completed(futures):
            ip, response = future.result()
            if response and response.startswith("OK THERMOSTAT"):
                found_devices.append((ip, response))
                print(f"Found: {ip} - {response}")

    print()
    if found_devices:
        print(f"Discovered {len(found_devices)} device(s):")
        for ip, resp in found_devices:
            print(f"  {ip}: {resp}")
    else:
        print("No thermostat devices found.")
        print("Ensure devices are powered on and connected to the same network.")

    return 0 if found_devices else 1


if __name__ == "__main__":
    sys.exit(main())
