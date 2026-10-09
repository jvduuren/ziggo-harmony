# ziggo-harmony

Bedien de **Ziggo Mediabox Next Mini** met een **Logitech Harmony** (of een andere infrarood-
afstandsbediening), met een klein ESP32-bordje van ongeveer €15.

🇬🇧 [English version](README.md)

> **Status: in ontwikkeling.** De firmware compileert en bevat alles wat hieronder staat, maar de
> eerste test op echte hardware moet nog gebeuren. Meld je resultaten graag via de issues.

## Waarom

De Next Mini heeft geen infraroodontvanger. De afstandsbediening praat met de box via Bluetooth Low
Energy, dus een Harmony of andere universele afstandsbediening kan hem niet bedienen. Ziggo heeft de
codes niet aan fabrikanten van afstandsbedieningen gegeven.

Dit project bootst de originele afstandsbediening na in een ESP32. De ESP32 ontvangt infrarood van
je Harmony en stuurt de bijbehorende toets via Bluetooth door, precies zoals de originele
afstandsbediening dat doet. De box ziet hem als zijn eigen afstandsbediening.

```
Harmony-afstandsbediening ─▶ Harmony Hub ──IR──▶ ESP32 ──Bluetooth LE──▶ Ziggo Next Mini
                                                (doet zich voor als de
                                                 originele afstandsbediening)
```

- Volledig lokaal: geen cloud, geen Home Assistant, wifi is alleen nodig voor het instellen.
- Directe reactie. Ingedrukt houden herhaalt, net als bij de originele afstandsbediening.
- Alle knoppen van de box: navigatie, zenders, cijfers, afspelen, opnemen, gids en profiel.
- Werkt met elke IR-afstandsbediening: de brug leert de codes.

## Wat heb je nodig

| Onderdeel | Opmerking |
|---|---|
| **Waveshare ESP32-S3-Zero** of **Seeed Studio XIAO ESP32S3** | Moet een ESP32-**S3** zijn (Bluetooth LE en wifi). Geen S2, die heeft geen Bluetooth. |
| **38 kHz IR-ontvanger**, VS1838B of TSOP38238 | Een los onderdeel met 3 pootjes houdt het kastje klein |
| USB-C-datakabel en een USB-lader | |
| Optioneel: een 3D-geprint kastje | Ontwerp volgt |

### Aansluiten

```
IR-ontvanger       ESP32-S3
  OUT  ──────────── GPIO1   (pad "1" op de S3-Zero, "D0" op de XIAO)
  GND  ──────────── GND
  VCC  ──────────── 3V3     (niet 5V)
```

De gangbare pinvolgorde van de VS1838B, kijkend op het bolletje met de pootjes naar beneden, is
**OUT, GND, VCC**. Klonen wijken soms af, dus controleer de datasheet van jouw exemplaar. Een
condensator van 100 nF tussen VCC en GND, vlak bij de ontvanger, is optioneel en maakt de ontvangst
stabieler.

## Flashen

1. Installeer [VS Code](https://code.visualstudio.com/) met de extensie **PlatformIO IDE**.
2. Clone deze repository en open de map in VS Code.
3. Kies de omgeving voor je bordje in de PlatformIO-werkbalk: `waveshare-esp32-s3-zero` of
   `seeed-xiao-esp32s3`.
4. Alleen de eerste keer: houd **BOOT** ingedrukt terwijl je de USB-kabel insteekt en laat hem
   daarna los.
5. Kies **Upload**, en daarna **Monitor** om het log te zien (115200 baud).

Na de eerste keer kun je updates via de webinterface installeren.

## Instellen

### 1. Wifi (optioneel, aanbevolen)

Bij de eerste start maakt de brug een wifi-netwerk aan met de naam **ZiggoRemote-Setup**. Verbind
daar met je telefoon mee, kies je eigen netwerk en vul het wachtwoord in. De webinterface staat
daarna op **http://ziggo-remote.local** (of op het IP-adres in het seriële log).

Zonder wifi werkt alles ook. Je leert de codes dan via de seriële console (zie verderop).

### 2. Koppelen met de box

1. Zorg dat de originele afstandsbediening **niet** in koppelmodus staat.
2. Houd de **standby-knop op de box** 5 seconden ingedrukt, tot het koppelscherm op de tv verschijnt.
3. De brug adverteert continu als `RemoteUnit`, en de box hoort hem vanzelf te koppelen. Het ledje
   wordt **blauw** zodra hij verbonden is.

De box accepteert waarschijnlijk maar één Bluetooth-afstandsbediening tegelijk. Na het koppelen van
de brug kan de originele afstandsbediening dus niet meer werken. Wil je terug, koppel dan de
originele afstandsbediening opnieuw: standby-knop op de box 5 s, daarna **TV + 0** op de
afstandsbediening 10 s.

### 3. Harmony instellen

Voeg in de Harmony-app een apparaat toe op wiens IR-codes **geen ander apparaat in de kamer
reageert**. Kies bijvoorbeeld een merk settopbox of mediaspeler dat je niet hebt. Wijs de commando's
van dat apparaat toe aan de knoppen van je activiteit "TV kijken". Tv-power en volume blijven
rechtstreeks naar je tv gaan, zoals je gewend bent.

### 4. Knoppen leren

Klik in de webinterface bij een knop op **Learn** en druk op de bijbehorende knop van je
Harmony-afstandsbediening. Herhaal dit voor elke knop die je wilt gebruiken. Met **Test** stuur je
een toets direct naar de box.

Koppelingen kun je als tekst exporteren en importeren ("Share or restore mapping"). Heb je een
koppeling gemaakt voor een bepaald Harmony-apparaatprofiel, deel hem dan, zodat anderen deze stap
kunnen overslaan.

## Gebruik

### Ledje

| Ledje | Betekenis |
|---|---|
| Rood, groen, blauw bij het aanzetten | Kleurentest |
| Oranje knipperend | Wacht op de box (niet gekoppeld of niet verbonden) |
| Blauw | Verbonden met de box |
| Paars knipperend | Leermodus: druk op een knop van je IR-afstandsbediening |
| Witte flits | IR-code ontvangen en doorgestuurd naar de box |
| Rode flits | Onbekende IR-code |
| Groene flits | IR-code geleerd |
| Snel wit knipperend | De box zoekt zijn afstandsbediening ("zoek mijn afstandsbediening") |

Lijkt oranje bij jou groen, zet dan `LED_SWAP_RG` in `platformio.ini` om.

### BOOT-knop

- 5 s ingedrukt houden: ontkoppelen van de box.
- 10 s ingedrukt houden: ontkoppelen van de box en de wifi-instellingen vergeten.

### Seriële console (USB, 115200 baud)

`keys`, `send <key>`, `learn <key>`, `clear <key>`, `map`, `status`, `unpair`, `wifi-reset`,
`raw <report> <usage>`

### HTTP-API

| Verzoek | Effect |
|---|---|
| `POST /api/send?key=ok` | Een toets indrukken (namen zoals in de webinterface en bij `keys`) |
| `POST /api/send?report=3&usage=0x40` | Een willekeurige HID-code sturen (om te ontdekken welke toetsen de box nog meer kent) |
| `GET /api/status` | Status als JSON |
| `GET /api/map`, `POST /api/map` | IR-koppelingen exporteren of importeren |

Voor Home Assistant, zie [extras/home-assistant/local-rest-command.yaml](extras/home-assistant/local-rest-command.yaml).

## Beperkingen

- Waarschijnlijk **één afstandsbediening tegelijk**: de originele afstandsbediening en de brug
  kunnen niet allebei gekoppeld zijn. Een doorgeefmodus, waarbij de originele afstandsbediening via
  de brug verbindt, is een mogelijke uitbreiding.
- **Spraakbediening** wordt niet ondersteund.
- In de **zuinige energiestand** gaat de box in diepe slaap en wordt hij alleen wakker met de knop op
  de box zelf. Zet de energiestand op snel/hoog als je hem op afstand wilt aanzetten.
- De webinterface en API hebben **geen wachtwoord**. Iedereen in je thuisnetwerk kan toetsen sturen
  of firmware uploaden. Zet de brug nooit open naar internet.
- Een software-update van Ziggo kan het protocol van de afstandsbediening veranderen, waardoor de
  brug niet meer werkt.

## Achtergrond

- [docs/ble-profile.md](docs/ble-profile.md): het volledige BLE/HID-profiel van de originele
  afstandsbediening en hoe het is uitgelezen (Engels).
- [tools/hid_dump.py](tools/hid_dump.py): het script waarmee de toetscodes zijn uitgelezen.
- [extras/home-assistant/cloud-route-package.yaml](extras/home-assistant/cloud-route-package.yaml):
  een alternatief zonder extra hardware, via Home Assistant en de Ziggo-cloud. Dat is trager en
  beperkter.

## Bijdragen

Handige bijdragen:

- Testresultaten, vooral uit andere Liberty Global-landen die dezelfde "4K Mini TV Box" gebruiken.
- IR-koppelingsbestanden voor Harmony-apparaatprofielen.
- Een ontwerp voor een kastje.

## Disclaimer

Dit is een onafhankelijk hobbyproject. Het is niet verbonden aan, goedgekeurd door of ondersteund
door Ziggo, VodafoneZiggo, Liberty Global, Universal Electronics of Logitech. Alle merknamen zijn
eigendom van hun respectievelijke eigenaren. Gebruik op eigen risico.

## Licentie

[MIT](LICENSE)
