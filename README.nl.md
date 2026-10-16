# ziggo-harmony

Bedien de **Ziggo Mediabox Next Mini** met een **Logitech Harmony** (of een andere infrarood-
afstandsbediening), met een klein ESP32-bordje van ongeveer €15.

🇬🇧 [English version](README.md)

> **Status: werkt.** Getest met een Waveshare ESP32-S3-Zero, een Harmony Smart Control (Hub) en een
> Ziggo Next Mini: koppelen, alle knoppen behalve spraak, en standby dat meegaat met de
> Harmony-activiteit. De XIAO ESP32S3 is nog niet getest. Meld je resultaten graag via de issues.

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
| **38 kHz IR-ontvanger**, VS1838B of TSOP38238, of een kant-en-klaar IR-oogje aan een 3,5 mm-kabel | Zie "Ontvanger aan een kabel" hieronder |
| Optioneel: 3,5 mm-stereo-inbouwbus (schroefdraad ≈ 8 mm) | Voor de kabel van het IR-oogje; past in het kastje |
| USB-C-datakabel en een USB-lader | |
| Optioneel: een 3D-geprint kastje | [extras/case](extras/case): voor de S3-Zero met een 3,5 mm-bus |

**Kastje.** In [extras/case](extras/case) staan een parametrisch OpenSCAD-ontwerp, STL-bestanden en
Bambu Studio-projecten (Bambu Lab A1, PLA): een onderbak voor de ESP32-S3-Zero met de USB-C-opening aan
de ene kant en de 3,5 mm-bus aan de andere, en een klikdeksel met een lichtgeleider boven de led (print
die in transparant filament) en een geleidebuisje voor een paperclip naar de BOOT-knop. Buitenmaat
53 × 22 × 16 mm. Alle maten staan als variabelen bovenaan het `.scad`-bestand.

Een Harmony met **Hub** stuurt de IR via de Hub. De brug moet dan dus de Hub (of een IR-mini-blaster)
kunnen zien, en niet de afstandsbediening.

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

**Ontvanger aan een kabel (aanrader bij een Hub).** Een kant-en-klaar IR-oogje aan een 3,5 mm-kabel
(een "IR-verlengkabel" voor settopboxen) laat je de ontvanger vlak bij de Hub leggen, terwijl de ESP32
ergens anders ligt. Monteer een 3,5 mm-inbouwbus in het kastje. De bedrading van zulke kabels
verschilt per merk, dus meet hem eerst door met een multimeter in de diodestand (rood in VΩ):

- **GND** is het contact waarbij je met rood erop naar beide andere contacten ongeveer 0,6–0,8 V
  ziet (andersom OL);
- tussen de andere twee geleidt hij alleen van **OUT** (rood) naar **VCC** (zwart).

Bij het geteste oogje (Amazon B07KM1Q48Y) is dat: tip = VCC, ring = OUT, sleeve = GND. Zulke oogjes
hebben vaak geen eigen pull-up op OUT; de firmware zet daarom de interne pull-up van GPIO1 aan.

## Flashen

Elke build levert per bordje twee bestanden op:

- `factory.bin`: de complete image, die je één keer via USB op adres `0x0` flasht.
- `firmware.bin`: alleen de applicatie, voor updates via wifi.

### De eerste keer, vanuit de browser (zonder installatie)

1. Download `factory.bin` voor je bordje bij de nieuwste
   [release](https://github.com/jvduuren/ziggo-harmony/releases), of bouw hem zelf (zie hieronder).
2. Houd **BOOT** op het bordje ingedrukt terwijl je het in de USB-poort van je computer steekt en
   laat de knop daarna los.
3. Open [esptool-js](https://espressif.github.io/esptool-js/) in **Chrome of Edge** (Safari en
   Firefox ondersteunen geen Web Serial). Klik op **Connect** en kies de seriële poort van het
   bordje.
4. Zet het flash-adres op `0x0`, kies `factory.bin` en klik op **Program**.
5. Haal het bordje los en steek het weer in. Het ledje toont rood, groen en blauw en knippert
   daarna oranje.

### Met PlatformIO

1. Installeer [VS Code](https://code.visualstudio.com/) met de extensie **PlatformIO IDE**.
2. Clone deze repository en open de map in VS Code.
3. Kies de omgeving voor je bordje in de PlatformIO-werkbalk: `waveshare-esp32-s3-zero` of
   `seeed-xiao-esp32s3`.
4. Alleen de eerste keer: houd **BOOT** ingedrukt terwijl je de USB-kabel insteekt en laat hem
   daarna los.
5. Kies **Upload**, en daarna **Monitor** om het log te zien (115200 baud).

### Updates via wifi

Upload `firmware.bin` (niet `factory.bin`) in de webinterface onder Maintenance, of vanaf de
opdrachtregel:

```
curl -F "firmware=@firmware.bin" http://<ip-van-de-brug>/update
```

Het log kun je ook zonder USB bekijken: in de webinterface onder Maintenance, of via `/api/log`.

## Instellen

### 1. Wifi (optioneel, aanbevolen)

Bij de eerste start maakt de brug een wifi-netwerk aan met de naam **ZiggoRemote-Setup**. Verbind
daar met je telefoon mee, kies je eigen netwerk en vul het wachtwoord in. De webinterface staat
daarna op het IP-adres van de brug. Dat vind je in de clientlijst van je router of in het seriële
log. Geef hem in je router een vast adres (DHCP-reservering). **http://ziggo-remote.local** werkt
soms ook, maar niet betrouwbaar: als wifi en Bluetooth tegelijk aan staan, slaapt de wifi en mist hij
de meeste mDNS-vragen.

Kon je niet op tijd verbinden? Staan er al wifi-gegevens in het bordje, dan gaat het setup-netwerk
na 3 minuten dicht. Herstart dan het bordje, of wis de wifi-gegevens (BOOT 10 s, of `wifi-reset` in de
seriële console).

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
reageert**. Aanbevolen: **Microsoft "Windows Media Center"** (in de app soms "Media-pc" of "Media
Center PC"). Dat gebruikt het RC6-protocol, heeft alle knoppen die je nodig hebt, en er is een
kant-en-klare koppeling voor (zie stap 4).

1. Voeg het apparaat toe aan je activiteit "TV kijken" en wijs de bedieningsknoppen eraan toe:
   pijltjes, OK, terug, menu, gids, kanalen, cijfers en afspelen. Volume en tv-power blijven naar je
   tv of versterker gaan, zoals je gewend bent.
2. Synchroniseer. Met een Hub-afstandsbediening (Smart Control, Companion, Elite) werken de knoppen
   alleen binnen een activiteit, en de brug moet de **Hub** of de IR-blaster kunnen zien.

De Harmony stuurt de **Power** van de Media-pc als een activiteit start en als hij stopt. Gekoppeld
aan Standby gaat de box dus mee aan en uit met de activiteit. Standby is een wisselknop: loopt de box
uit de pas, gebruik dan **Help** in de Harmony-app.

### 4. Knoppen koppelen

**Met het Media-pc-profiel:** importeer [extras/harmony/media-center.txt](extras/harmony/media-center.txt)
in de webinterface onder "Share or restore mapping" (plak de tekst en klik op **Import**). Klaar.

**Met een ander apparaat:** klik in de webinterface bij een knop op **Learn** en druk op de
bijbehorende knop van je Harmony. Herhaal dit voor elke knop. Met **Test** stuur je een toets direct
naar de box.

- Klik je nog eens op **Learn**, dan krijgt een knop er een **extra** code bij, tot maximaal 3.
  Bijvoorbeeld zowel Play als Pause voor Play/Pause. Met **×** wis je alle codes van die knop.
- Een goede code heeft een protocolnaam zoals **RC6**, **NEC** of **SAMSUNG**. Signalen die de
  firmware niet kan ontcijferen ("UNKNOWN") slaat hij bij het leren over, want die verschillen vaak
  per druk. Komt een knop alleen als UNKNOWN binnen, kies dan een ander Harmony-apparaat.
- Onder **Maintenance → Log** in de webinterface zie je welke codes er binnenkomen.

Koppelingen kun je als tekst exporteren en importeren ("Share or restore mapping"). Een knop die in
een import voorkomt, krijgt de codes uit die import. Heb je een koppeling gemaakt voor een bepaald
Harmony-apparaatprofiel, deel hem dan, zodat anderen deze stap kunnen overslaan.

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
`raw <report> <usage>`, `scan [seconden]`

`scan` logt de Bluetooth-advertenties van afstandsbedieningen in de buurt, byte voor byte. Zo is
uitgezocht welke advertentie de box verwacht.

### HTTP-API

| Verzoek | Effect |
|---|---|
| `POST /api/send?key=ok` | Een toets indrukken (namen zoals in de webinterface en bij `keys`) |
| `POST /api/send?report=3&usage=0x40` | Een willekeurige HID-code sturen (om te ontdekken welke toetsen de box nog meer kent) |
| `GET /api/status` | Status als JSON |
| `GET /api/log` | Recente logregels |
| `POST /update` (multipart, veld `firmware`) | Firmware-update met `firmware.bin` |
| `GET /api/map`, `POST /api/map` (tekst) | IR-koppelingen exporteren of importeren |
| `POST /api/scan?seconds=60` | Bluetooth-advertenties van afstandsbedieningen in de buurt loggen |

Voor Home Assistant, zie [extras/home-assistant/local-rest-command.yaml](extras/home-assistant/local-rest-command.yaml).

## Beperkingen

- Waarschijnlijk **één afstandsbediening tegelijk**: de originele afstandsbediening en de brug
  kunnen niet allebei gekoppeld zijn. Een doorgeefmodus, waarbij de originele afstandsbediening via
  de brug verbindt, is een mogelijke uitbreiding.
- **Spraak/zoeken** werkt niet: de box start dan een spraaksessie en wacht op audio van een
  microfoon. Zoeken kan wel via het menu van de box.
- **ziggo-remote.local** (mDNS) werkt niet betrouwbaar. Gebruik het IP-adres.
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
