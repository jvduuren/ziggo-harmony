# ziggo-harmony

Control the **Ziggo Mediabox Next Mini** with a **Logitech Harmony** (or any other infrared remote),
using a small ESP32 board of about €15.

🇳🇱 [Nederlandse versie](README.nl.md)

> **Status: working.** Tested with a Waveshare ESP32-S3-Zero, a Harmony Smart Control (Hub) and a
> Ziggo Next Mini: pairing, every button except voice, and standby following the Harmony activity.
> The XIAO ESP32S3 is untested so far. Please report your results in the issues.

## Why

The Next Mini has no infrared receiver. Its remote talks to the box over Bluetooth Low Energy, so a
Harmony, or any other universal remote, cannot control it. Ziggo has not shared the codes with
remote manufacturers.

This project reproduces the original remote in an ESP32. The ESP32 receives infrared from your
Harmony and sends the matching key over Bluetooth, exactly the way the original remote does. The box
sees it as its own remote.

```
Harmony remote ─▶ Harmony Hub ──IR──▶ ESP32 ──Bluetooth LE──▶ Ziggo Next Mini
                                     (presents itself as the
                                      original Ziggo remote)
```

- Fully local: no cloud, no Home Assistant, Wi-Fi only needed for setup.
- Instant response. Holding a button repeats, just like the original remote.
- All box buttons: navigation, channels, digits, playback, record, guide, profile.
- Works with any IR remote: the bridge learns the codes.

## What you need

| Part | Notes |
|---|---|
| **Waveshare ESP32-S3-Zero** or **Seeed Studio XIAO ESP32S3** | Must be an ESP32-**S3** (BLE + Wi-Fi). Not the S2, which has no Bluetooth. |
| **38 kHz IR receiver**, VS1838B or TSOP38238 | A bare 3-pin part keeps the case small |
| USB-C data cable and a USB power supply | |
| Optional: a 3D-printed case | Design follows |

With a **Hub**-based Harmony the IR comes from the Hub, so the bridge needs to see the Hub (or an IR
mini blaster), not the remote.

### Wiring

```
IR receiver        ESP32-S3
  OUT  ──────────── GPIO1   (pad "1" on the S3-Zero, "D0" on the XIAO)
  GND  ──────────── GND
  VCC  ──────────── 3V3     (not 5V)
```

The common VS1838B pin order, looking at the dome with the legs down, is **OUT, GND, VCC**. Clones
differ, so check the datasheet of yours. An optional 100 nF capacitor between VCC and GND, close to
the receiver, makes reception more robust.

**Receiver on a cable (recommended with a Hub).** A ready-made IR "eye" on a 3.5 mm cable (an "IR
extender cable" for set-top boxes) lets you put the receiver right next to the Hub while the ESP32
sits elsewhere. Mount a 3.5 mm panel jack in the case. The wiring of these cables differs per brand,
so measure it first with a multimeter in diode mode (red in VΩ):

- **GND** is the contact that, with red on it, shows about 0.6–0.8 V to both other contacts (OL the
  other way round);
- between the other two it only conducts from **OUT** (red) to **VCC** (black).

For the tested eye (Amazon B07KM1Q48Y) that is: tip = VCC, ring = OUT, sleeve = GND. Such eyes often
have no pull-up of their own on OUT, so the firmware enables GPIO1's internal pull-up.

## Flashing

Every build produces two files per board:

- `factory.bin`: the complete image, flashed once over USB at offset `0x0`.
- `firmware.bin`: the application only, for updates over Wi-Fi.

### First time, from a browser (no installation)

1. Download `factory.bin` for your board from the latest
   [release](https://github.com/jvduuren/ziggo-harmony/releases), or build it yourself (see below).
2. Hold **BOOT** on the board while plugging it into your computer's USB port, then release it.
3. Open [esptool-js](https://espressif.github.io/esptool-js/) in **Chrome or Edge** (Safari and
   Firefox lack Web Serial). Click **Connect** and choose the board's serial port.
4. Set the flash address to `0x0`, select `factory.bin` and click **Program**.
5. Unplug and replug the board. The LED shows red, green and blue, then blinks amber.

### With PlatformIO

1. Install [VS Code](https://code.visualstudio.com/) with the **PlatformIO IDE** extension.
2. Clone this repository and open the folder in VS Code.
3. Pick the environment for your board in the PlatformIO toolbar: `waveshare-esp32-s3-zero` or
   `seeed-xiao-esp32s3`.
4. First time only: hold **BOOT** while plugging in the USB cable, then release it.
5. Run **Upload**, then **Monitor** to see the log (115200 baud).

### Updates over Wi-Fi

Upload `firmware.bin` (not `factory.bin`) in the web UI under Maintenance, or from a command line:

```
curl -F "firmware=@firmware.bin" http://<bridge-ip>/update
```

The log is also available without USB: in the web UI under Maintenance, or at `/api/log`.

## Setup

### 1. Wi-Fi (optional, recommended)

On first start the bridge opens a Wi-Fi network called **ZiggoRemote-Setup**. Connect to it with
your phone, pick your home network and enter the password. The web UI is then at the bridge's IP
address, which you find in your router's client list or in the serial log. Give it a fixed address in
your router (DHCP reservation). **http://ziggo-remote.local** sometimes works too, but not reliably:
with Wi-Fi and Bluetooth both active the Wi-Fi radio sleeps and misses most mDNS queries.

Missed the setup network? If the board already holds Wi-Fi credentials, the setup network closes
after 3 minutes. Restart the board, or forget the Wi-Fi settings (BOOT 10 s, or `wifi-reset` on the
serial console).

Without Wi-Fi everything still works. You then learn codes through the serial console instead (see
below).

### 2. Pair with the box

1. Make sure the original remote is **not** in pairing mode.
2. Hold the **standby button on the box** for 5 seconds, until the pairing screen appears on the TV.
3. The bridge advertises continuously as `RemoteUnit`, and the box should pair with it by itself.
   The LED turns **blue** once it is connected.

The box most likely accepts only one Bluetooth remote at a time. Pairing the bridge may therefore
disconnect the original remote. To switch back, pair the original remote again: standby button on
the box for 5 s, then **TV + 0** on the remote for 10 s.

### 3. Set up the Harmony

Add a device in the Harmony app whose IR codes **no other device in the room responds to**.
Recommended: **Microsoft "Windows Media Center"** (sometimes listed as "Media Center PC"). It uses
the RC6 protocol, has every button you need, and there is a ready-made mapping for it (see step 4).

1. Add the device to your "Watch TV" activity and assign the control buttons to it: arrows, OK,
   back, menu, guide, channels, digits and playback. Volume and TV power keep going to your TV or
   receiver, as before.
2. Sync. With a Hub-based remote (Smart Control, Companion, Elite) the buttons only work inside an
   activity, and the bridge must be able to see the **Hub** or its IR blaster.

The Harmony sends the Media Center **Power** command when an activity starts and when it stops.
Mapped to Standby, the box follows the activity on and off. Standby is a toggle: if the box gets out
of step, use **Help** in the Harmony app.

### 4. Map the buttons

**With the Media Center profile:** import [extras/harmony/media-center.txt](extras/harmony/media-center.txt)
in the web UI under "Share or restore mapping" (paste the text and click **Import**). Done.

**With another device:** in the web UI, click **Learn** next to a button and press the matching
button on your Harmony. Repeat for every button. Use **Test** to send a key to the box directly.

- Clicking **Learn** again **adds** another code to a key, up to 3, for example both Play and Pause
  for Play/Pause. **×** forgets all codes of that key.
- A good code has a protocol name such as **RC6**, **NEC** or **SAMSUNG**. Signals the firmware
  cannot decode ("UNKNOWN") are skipped while learning, because they often differ on every press. If
  a button only ever arrives as UNKNOWN, pick another Harmony device.
- **Maintenance → Log** in the web UI shows which codes arrive.

Mappings can be exported and imported as text ("Share or restore mapping"). A key that appears in an
import gets the codes from that import. If you made a mapping for a specific Harmony device profile,
please share it so others can skip this step.

## Using it

### LED

| LED | Meaning |
|---|---|
| Red, green, blue at power-on | Colour check |
| Amber blink | Waiting for the box (not paired or not connected) |
| Blue | Connected to the box |
| Purple blink | Learning: press a button on your IR remote |
| White flash | IR code received and sent to the box |
| Red flash | Unknown IR code |
| Green flash | IR code learned |
| Fast white blink | The box is looking for its remote ("find my remote") |

If amber looks green on your board, flip `LED_SWAP_RG` in `platformio.ini`.

### BOOT button

- Hold 5 s: unpair the box.
- Hold 10 s: unpair the box and forget the Wi-Fi settings.

### Serial console (USB, 115200 baud)

`keys`, `send <key>`, `learn <key>`, `clear <key>`, `map`, `status`, `unpair`, `wifi-reset`,
`raw <report> <usage>`, `scan [seconds]`

`scan` logs the Bluetooth advertisements of nearby remotes byte for byte. That is how the
advertisement the box expects was found.

### HTTP API

| Request | Effect |
|---|---|
| `POST /api/send?key=ok` | Press a key (ids as in the web UI and `keys`) |
| `POST /api/send?report=3&usage=0x40` | Send any HID usage (for discovering keys the box understands) |
| `GET /api/status` | Status as JSON |
| `GET /api/log` | Recent log lines |
| `POST /update` (multipart, field `firmware`) | Firmware update with `firmware.bin` |
| `GET /api/map`, `POST /api/map` (text) | Export or import the IR mapping |
| `POST /api/scan?seconds=60` | Log the Bluetooth advertisements of nearby remotes |

For Home Assistant, see [extras/home-assistant/local-rest-command.yaml](extras/home-assistant/local-rest-command.yaml).

## Limitations

- Most likely **one remote at a time**: the original remote and the bridge cannot both be paired.
  A pass-through mode, where the original remote connects through the bridge, is a possible future
  feature.
- **Voice search** does not work: the box starts a voice session and waits for audio from a
  microphone. Search through the box's menu instead.
- **ziggo-remote.local** (mDNS) is unreliable. Use the IP address.
- In **eco energy mode** the box falls into deep standby and only wakes up with the button on the
  box itself. Set the box's energy mode to fast/high if you want to switch it on remotely.
- The web UI and API have **no password**. Anyone on your local network can send keys or upload
  firmware. Never expose the bridge to the internet.
- A software update from Ziggo could change the remote protocol and break the bridge.

## Background

- [docs/ble-profile.md](docs/ble-profile.md): the complete BLE/HID profile of the original remote and
  how it was captured.
- [tools/hid_dump.py](tools/hid_dump.py): the script used to capture the key codes.
- [extras/home-assistant/cloud-route-package.yaml](extras/home-assistant/cloud-route-package.yaml):
  an alternative without extra hardware, via Home Assistant and the Ziggo cloud. It is slower and
  more limited.

## Contributing

Useful contributions:

- Test reports, especially from other Liberty Global countries that use the same "4K Mini TV Box".
- IR mapping files for Harmony device profiles.
- A case design.

## Disclaimer

This is an independent hobby project. It is not affiliated with, endorsed by or supported by Ziggo,
VodafoneZiggo, Liberty Global, Universal Electronics or Logitech. All trademarks belong to their
respective owners. Use at your own risk.

## License

[MIT](LICENSE)
