#!/usr/bin/env python3
"""Print the raw HID reports of the Ziggo Next Mini remote ("RemoteUnit"), per button.

Used to capture docs/ble-profile.md; handy for checking other remote/firmware versions.

macOS:
  1. Put the remote in pairing mode (TV + 0 for 10 s) and connect it in
     System Settings > Bluetooth.
  2. Allow Terminal under Privacy & Security > Input Monitoring, then restart Terminal.
  3. python3 -m venv ~/hidenv && ~/hidenv/bin/pip install hidapi
     ~/hidenv/bin/python hid_dump.py
Per button: type a name, press Enter, then press that button once on the remote.
Results are also written to buttons.csv next to this script.
Afterwards, forget the remote on the Mac and pair it with the box again.
"""
import csv
import ctypes
import sys
import termios
import time
from pathlib import Path

import hid

NAME = "RemoteUnit"
VID, PID = 0x06E7, 0x8140  # from the remote's PnP ID


def hexs(data):
    return " ".join(f"{b:02X}" for b in data)


def is_remote(d):
    return NAME in (d.get("product_string") or "") or (d["vendor_id"], d["product_id"]) == (VID, PID)


def open_remote():
    all_infos = hid.enumerate()
    infos = [d for d in all_infos if is_remote(d)]
    if not infos:
        print("HID devices found:")
        for d in all_infos:
            print(f"  {d['vendor_id']:04X}:{d['product_id']:04X}  {d.get('product_string')!r}")
        sys.exit(f"'{NAME}' not found. Is it paired and connected (press a button)?")
    # macOS: hidapi opens devices exclusively by default, which fails for keyboard-like
    # devices. Disable it after hid_init (first enumerate), otherwise it is reset.
    try:
        ctypes.CDLL(hid.__file__).hid_darwin_set_open_exclusive(0)
    except (OSError, AttributeError):
        pass
    for d in infos:
        print(f"collection: usage_page=0x{d['usage_page']:04X} usage=0x{d['usage']:04X}")
    devices = []
    for path in dict.fromkeys(d["path"] for d in infos):
        h = hid.device()
        try:
            h.open_path(path)
        except OSError:
            sys.exit("Open failed. Allow Terminal under System Settings > Privacy & Security > "
                     "Input Monitoring, quit Terminal (Cmd+Q) and try again.")
        h.set_nonblocking(True)
        devices.append(h)
    return devices


def read_all(devices):
    reports = []
    for h in devices:
        while data := h.read(64):
            reports.append(data)
    return reports


def capture(devices, timeout=5.0):
    """Wait for the first report, then keep collecting for 0.5 s (key release)."""
    reports, deadline = [], time.monotonic() + timeout
    while time.monotonic() < deadline:
        new = read_all(devices)
        if new and not reports:
            deadline = time.monotonic() + 0.5
        reports += new
        time.sleep(0.005)
    return reports


def ask(prompt):
    # The remote also types into Terminal as a keyboard; discard those characters first.
    termios.tcflush(sys.stdin, termios.TCIFLUSH)
    return input(prompt).strip()


def main():
    devices = open_remote()
    rows = []
    print("Type a button name + Enter, then press that button. Empty name = stop.")
    while name := ask("\nbutton: "):
        read_all(devices)  # drop stale reports
        reports = capture(devices)
        if not reports:
            print("  no report received (press it again: the first press may wake the remote)")
            continue
        for r in reports:
            print("  " + hexs(r))
        rows.append([name] + [hexs(r) for r in reports])
    out = Path(__file__).with_name("buttons.csv")
    with out.open("w", newline="") as f:
        csv.writer(f).writerows(rows)
    print(f"\nsaved to {out}")


if __name__ == "__main__":
    main()
