# BLE profile of the original Ziggo Next Mini remote

Reference for the remote that ships with the Ziggo Mediabox Next Mini (Liberty Global "4K Mini TV
Box", remote made by Universal Electronics Inc.). The firmware in this repository reproduces this
profile so the box accepts the ESP32 as its remote.

## How it was captured

| What | Tool |
|---|---|
| Advertisement, GATT services, Device Information | nRF Connect (iOS), remote in pairing mode |
| HID report map | macOS: pair the remote, then `ioreg -r -l -c IOHIDDevice` |
| Report per button | macOS: [`tools/hid_dump.py`](../tools/hid_dump.py) |

Pairing mode on the remote: hold **TV** (the live-TV button, not TV power) **+ 0** for 10 seconds.
iOS hides the HID service from apps; macOS shows it.

## Advertisement

- Complete local name: `RemoteUnit`
- Manufacturer data: company ID `0x0093` (Universal Electronics, Inc.), data `00 80`
- 16-bit service UUIDs: `0x1812` Human Interface Device, `0x180F` Battery Service
- Connectable

## GATT services

- `0x1800` Generic Access, `0x1801` Generic Attribute
- `0x180A` Device Information
  - Manufacturer Name (2A29): `Universal Electronics, Inc.`
  - Model Number (2A24): `RemoteUnit`
  - Serial Number (2A25): `0000-0001` (same on every unit)
  - Firmware Revision (2A26): `BL 196`
  - Hardware Revision (2A27): `01`
  - Software Revision (2A28): `7060.29.00_0.0`
  - System ID (2A23): 8 bytes derived from the remote's own BLE address (`.. .. .. FF FE .. .. ..`)
  - IEEE 11073-20601 Regulatory Certification (2A2A): `33 00 00 00 00 00`
  - PnP ID (2A50): `02 E7 06 40 81 10 01`
    - vendor ID source `0x02` (USB-IF), vendor ID `0x06E7`, product ID `0x8140`, version `0x0110`
- `0x180F` Battery Service: Battery Level (2A19), read/notify
- `0x1803` Link Loss: Alert Level (2A06), read/write
- `0x1802` Immediate Alert: Alert Level (2A06), write without response (probably "find my remote")
- `0x1804` Tx Power: Tx Power Level (2A07), read
- `0x1812` Human Interface Device (report map below)

No vendor-specific GATT services are exposed.

## HID report map

```
05010907a101850195027510050719002a000115002600018100c0
050c0901a10185039502751019002aff03150026ff038100c0
05010980a10185029502751019002a000115002600018100c0
0601ff0901a101a102850709141580257f750895c58122c0c0
0601ff0901a101a102850809141580257f750895c59122c0c0
0601ff0904a101a102851f09141580257f750895a08122c0c0
```
(one continuous descriptor, split per collection for readability)

| Report ID | Collection | Direction | Format |
|---|---|---|---|
| 1 | Generic Desktop / Keypad, usage page Keyboard (0x07) | input | 2 × uint16 LE usage (0..0x100), array |
| 3 | Consumer / Consumer Control (0x0C) | input | 2 × uint16 LE usage (0..0x3FF), array |
| 2 | Generic Desktop / System Control | input | 2 × uint16 LE usage (0..0x100), array |
| 7 | Vendor 0xFF01 / 0x01 | input | 197 bytes |
| 8 | Vendor 0xFF01 / 0x01 | output | 197 bytes |
| 0x1F | Vendor 0xFF01 / 0x04 | input | 160 bytes |

A key press is one report with the usage in the first 16-bit slot; release is the same report with
all zeros. Reports 7, 8 and 0x1F are most likely voice and the UEI QuickSet configuration channel
(the box can send data to the remote through report 8, probably to program the TV IR codes).

## Buttons

| Button | Report | Usage | HID name |
|---|---|---|---|
| Standby (box) | 3 | `0x003F` | (no common standard name) |
| Home | 3 | `0x0223` | AC Home |
| Back | 3 | `0x0224` | AC Back |
| Guide | 3 | `0x008D` | Media Select Program Guide |
| TV (live TV) | 3 | `0x0089` | Media Select TV |
| Up / Down / Left / Right | 3 | `0x0042` / `0x0043` / `0x0044` / `0x0045` | Menu Up / Down / Left / Right |
| OK | 3 | `0x0041` | Menu Pick |
| Channel + / − | 3 | `0x009C` / `0x009D` | Channel Increment / Decrement |
| Record | 3 | `0x00B2` | Record |
| Rewind | 3 | `0x00B4` | Rewind |
| Play/Pause | 3 | `0x00CD` | Play/Pause |
| Fast forward | 3 | `0x00B3` | Fast Forward |
| Voice | 3 | `0x0221` | AC Search, plus vendor report 7 (`07 04 00 78 36 43 00 …`) |
| Profile (side button) | 3 | `0x018D` | Application Launch range |
| 1 … 9 | 1 | `0x001E` … `0x0026` | Keyboard 1 … 9 |
| 0 | 1 | `0x0027` | Keyboard 0 |

TV power, volume +/−, mute and source send no BLE report: the remote sends those as IR straight to
the TV.

The first button press after the remote has been idle for a while may be lost while it reconnects.
