# BLE-profiel originele Ziggo Next Mini-afstandsbediening

Uitgelezen met nRF Connect (iOS) op 2026-10-09, afstandsbediening in koppelmodus
(TV-knop, niet de TV-powerknop, + 0 tien seconden ingedrukt).

## Advertentie
- Naam: `RemoteUnit`
- Device type: HID
- Manufacturer data: company ID `0x0093` (Universal Electronics, Inc.), data `00 80`
- Geadverteerde services: `0x1812` Human Interface Device, `0x180F` Battery Service
- Connectable: ja

## GATT
- `0x1800` Generic Access
- `0x1801` Generic Attribute
- `0x180A` Device Information
  - Manufacturer Name (2A29): `Universal Electronics, Inc.`
  - Model Number (2A24): `RemoteUnit`
  - Serial Number (2A25): `0000-0001`
  - Firmware Revision (2A26): `BL 196`
  - Hardware Revision (2A27): `01`
  - Software Revision (2A28): `7060.29.00_0.0`
  - System ID (2A23): `B8 C0 65 FF FE 5A 81 EF`
  - IEEE 11073 Reg. Cert. (2A2A): `33 00 00 00 00 00`
  - PnP ID (2A50): `02 E7 06 40 81 10 01`
    - Vendor ID source `0x02` (USB-IF)
    - Vendor ID `0x06E7` (niet in de openbare usb.ids; vermoedelijk UEI)
    - Product ID `0x8140`
    - Product version `0x0110`
- `0x180F` Battery Service — Battery Level (2A19): read, notify
- `0x1803` Link Loss — Alert Level (2A06): read, write
- `0x1802` Immediate Alert — Alert Level (2A06): write without response
  (vermoedelijk de "zoek mijn afstandsbediening"-piep)
- `0x1804` Tx Power — Tx Power Level (2A07): read
- `0x1812` HID — door iOS verborgen; Report Map uitgelezen via macOS `ioreg` (zie hieronder)

## HID Report Map (macOS ioreg, 2026-10-09)
```
05010907a101850195027510050719002a000115002600018100c0
050c0901a10185039502751019002aff03150026ff038100c0
05010980a10185029502751019002a000115002600018100c0
0601ff0901a101a102850709141580257f750895c58122c0c0
0601ff0901a101a102850809141580257f750895c59122c0c0
0601ff0904a101a102851f09141580257f750895a08122c0c0
```
(regels zijn alleen voor de leesbaarheid gesplitst; het is één aaneengesloten descriptor)

| Report ID | Collectie | Richting | Formaat |
|---|---|---|---|
| 1 | Generic Desktop / Keypad (0x01/0x07), usage page Keyboard 0x07 | input | 2 × uint16 LE usage (0..0x100), array |
| 3 | Consumer / Consumer Control (0x0C/0x01) | input | 2 × uint16 LE usage (0..0x3FF), array |
| 2 | Generic Desktop / System Control (0x01/0x80) | input | 2 × uint16 LE usage (0..0x100), array |
| 7 | Vendor 0xFF01 / 0x01, usage 0x14 | input | 197 bytes |
| 8 | Vendor 0xFF01 / 0x01, usage 0x14 | output | 197 bytes |
| 0x1F | Vendor 0xFF01 / 0x04, usage 0x14 | input | 160 bytes |

Een knopdruk is dus: report met één usage in het eerste 16-bit-slot, loslaten = report met nullen.
Reports 7/8/0x1F zijn vermoedelijk spraak en het UEI/QuickSet-configuratiekanaal; de box kan via
report 8 data naar de remote sturen — nagaan of de box op een antwoord wacht.

## Knoppen (tools/hid_dump.py op macOS, 2026-10-09)
Elke knop: report met de usage in het eerste slot (uint16 LE), daarna een report met nullen.

| Knop | Report | Usage | HID-naam |
|---|---|---|---|
| standby (box) | 3 | `0x003F` | — (geen gangbare standaardnaam) |
| home | 3 | `0x0223` | AC Home |
| terug | 3 | `0x0224` | AC Back |
| gids | 3 | `0x008D` | Media Select Program Guide |
| tv (naar live-tv) | 3 | `0x0089` | Media Select TV |
| pijl omhoog | 3 | `0x0042` | Menu Up |
| pijl omlaag | 3 | `0x0043` | Menu Down |
| pijl links | 3 | `0x0044` | Menu Left |
| pijl rechts | 3 | `0x0045` | Menu Right |
| ok | 3 | `0x0041` | Menu Pick |
| programma + | 3 | `0x009C` | Channel Increment |
| programma − | 3 | `0x009D` | Channel Decrement |
| opname | 3 | `0x00B2` | Record |
| terugspoelen | 3 | `0x00B4` | Rewind |
| start/pauze | 3 | `0x00CD` | Play/Pause |
| vooruitspoelen | 3 | `0x00B3` | Fast Forward |
| spraak | 3 | `0x0221` | AC Search (+ report 7, zie onder) |
| profiel (zijknop) | 3 | `0x018D` | AL-reeks (Application Launch), hier gebruikt voor profielwissel |
| 1 … 9 | 1 | `0x001E` … `0x0026` | Keyboard 1 … 9 |
| 0 | 1 | `0x0027` | Keyboard 0 |

Geen BLE-report (gaan via IR rechtstreeks naar de tv): tv-power, volume +/−, mute, source.

Spraakknop stuurt naast AC Search ook report 7 (eerste 64 bytes, rest nullen):
`07 04 00 78 36 43 00 …` — vermoedelijk start van een spraaksessie; niet nodig voor de brug.

Opmerking: de eerste knopdruk na een tijd niets doen gaf soms geen report (remote moest
eerst opnieuw verbinden).
