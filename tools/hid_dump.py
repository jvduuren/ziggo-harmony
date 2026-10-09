#!/usr/bin/env python3
"""Leest de ruwe HID-reports van de Ziggo Next Mini-afstandsbediening ("RemoteUnit").

Koppel de afstandsbediening eerst via Systeeminstellingen > Bluetooth met de Mac.
Vereist:  pip3 install hidapi
Gebruik:  python3 hid_dump.py
Per knop: typ de naam, Enter, en druk dan één keer op die knop.
Het resultaat komt ook in knoppen.csv naast dit script.
"""
import csv
import ctypes
import sys
import termios
import time
from pathlib import Path

import hid

NAME = "RemoteUnit"
VID, PID = 0x06E7, 0x8140  # uit de PnP ID van de afstandsbediening


def hexs(data):
    return " ".join(f"{b:02X}" for b in data)


def is_remote(d):
    return NAME in (d.get("product_string") or "") or (d["vendor_id"], d["product_id"]) == (VID, PID)


def open_remote():
    all_infos = hid.enumerate()
    infos = [d for d in all_infos if is_remote(d)]
    if not infos:
        print("Gevonden HID-apparaten:")
        for d in all_infos:
            print(f"  {d['vendor_id']:04X}:{d['product_id']:04X}  {d.get('product_string')!r}")
        sys.exit(f"'{NAME}' niet gevonden. Is hij gekoppeld en verbonden (druk een knop)?")
    # macOS: hidapi opent standaard exclusief, wat bij toetsenbordachtige apparaten
    # mislukt. Pas na hid_init (eerste enumerate) uitzetten, anders wordt het overschreven.
    try:
        ctypes.CDLL(hid.__file__).hid_darwin_set_open_exclusive(0)
    except (OSError, AttributeError):
        pass
    for d in infos:
        print(f"collectie: usage_page=0x{d['usage_page']:04X} usage=0x{d['usage']:04X}")
    devices = []
    for path in dict.fromkeys(d["path"] for d in infos):
        h = hid.device()
        try:
            h.open_path(path)
        except OSError:
            sys.exit("Openen mislukt. Geef Terminal toegang via Systeeminstellingen > "
                     "Privacy en beveiliging > Invoermonitoring, sluit Terminal (Cmd+Q) "
                     "en probeer opnieuw.")
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
    """Wacht op de eerste report en verzamelt daarna nog 0,5 s (loslaten)."""
    reports, deadline = [], time.monotonic() + timeout
    while time.monotonic() < deadline:
        new = read_all(devices)
        if new and not reports:
            deadline = time.monotonic() + 0.5
        reports += new
        time.sleep(0.005)
    return reports


def ask(prompt):
    # De remote typt ook als toetsenbord in Terminal; die tekens eerst weggooien.
    termios.tcflush(sys.stdin, termios.TCIFLUSH)
    return input(prompt).strip()


def main():
    devices = open_remote()
    rows = []
    print("Typ een knopnaam + Enter en druk dan op die knop. Lege naam = stoppen.")
    while name := ask("\nknop: "):
        read_all(devices)  # oude reports weggooien
        reports = capture(devices)
        if not reports:
            print("  geen report ontvangen")
            continue
        for r in reports:
            print("  " + hexs(r))
        rows.append([name] + [hexs(r) for r in reports])
    out = Path(__file__).with_name("knoppen.csv")
    with out.open("w", newline="") as f:
        csv.writer(f).writerows(rows)
    print(f"\nopgeslagen in {out}")


if __name__ == "__main__":
    main()
