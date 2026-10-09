# ziggo-harmony

Ziggo Mediabox Next Mini bedienen met een Logitech Harmony Hub.

De Next Mini heeft geen IR-ontvanger; de originele afstandsbediening werkt via Bluetooth LE (HID).
Een ESP32-S3 vangt de IR-signalen van de Harmony Hub op en doet zich via BLE voor als de
originele afstandsbediening.

- `docs/remote-ble-profiel.md` — uitgelezen BLE/HID-profiel van de originele afstandsbediening
- `tools/hid_dump.py` — script (macOS) om de HID-reports per knop uit te lezen
- `packages/ziggo_harmony.yaml` — alternatieve route via Home Assistant (emulated Roku + LG Horizon)
