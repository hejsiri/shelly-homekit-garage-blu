# Garage door with encrypted Shelly BLU sensors

Target: **Shelly 1 Gen3**, not Mini or Gen4. HomeKit uses Wi-Fi. The two
Shelly BLU Door/Window (SBDW-002C) sensors retain their original firmware.

This is an experimental implementation. The firmware compiles, cryptographic
and host-side tests pass, but BLE pairing and operation have **not** been tested
on physical Shelly hardware. Do not treat a demonstration in the local web
preview as evidence of successful radio pairing.

## Wspierane urządzenia Garage BLU

| Urządzenie | Status w tym wydaniu |
| --- | --- |
| **Shelly 1 Gen3 (S1G3)** | Jedyny model z przygotowaną paczką Garage BLU; kompilacja i testy lokalne przeszły, testy na sprzęcie pozostają do wykonania. |
| **Shelly BLU Door/Window (SBDW-002C)** | Zaimplementowane dwa czujniki: pełne zamknięcie i pełne otwarcie bramy. Czujniki zachowują fabryczny firmware; parowanie i odbiór BLE wymagają testów na sprzęcie. |
| Shelly 1 Mini Gen3, 1PM Gen3, pozostałe Gen3, Plus, Gen1 i Pro | Brak paczek i obsługi Garage BLU dla tych modeli w tym wydaniu. |
| Shelly Gen4 | Brak portu Garage BLU dla platformy Gen4. |
| Shelly BLU H&T (temperatura i wilgotność) oraz Shelly BLU Button , czujnik ruchu Shelly BLU Motion oraz czujnik wycieku Shelly BLU | Wkrótce — planowane wsparcie; jeszcze niedostępne w aktualnym wydaniu. |
| Inne czujniki BLE | Obsługa nie została dodana. |

## Installation and OTA updates

Follow the [Polish installation and update instructions](docs/AKTUALIZACJA.md).
Download Garage BLU firmware from <https://hejsiri.pl/firmware/garage-blu/>.
The upstream rojer.me packages do not contain these BLU changes.

## Add a sensor without the Shelly application

BLU Door/Window sensors do not need a firmware update or replacement firmware.
They are intended to run their original Shelly firmware; pair them with the
controller through the Garage BLU web panel. Only the Shelly 1 Gen3 controller
receives the Garage BLU OTA package. Physical pairing remains unverified.

1. Set an administrator password in the web panel's **Security Settings**.
   Provisioning RPCs require authentication to be configured. Use a trusted
   local network: the existing panel uses HTTP/WebSocket, not HTTPS.
2. Select **Garage Door Opener** mode and click **Pair** beside **Closing sensor**
   or **Opening sensor**.
3. Hold the BLU sensor's button for more than 10 seconds to enable pairing mode.
   Choose the discovered Door/Window sensor from the list. Discovery is limited
   to eight devices advertising the Door/Window model and pairing-mode flag.
4. For a new or factory-reset sensor, choose a **new six-digit PIN** (100000–999999).
   For an encrypted sensor, also enter its **current PIN** and choose a different
   new PIN; alternatively factory-reset that sensor first. No Shelly app is
   required. Keep the new PIN for later reconfiguration.
5. Click **Pair selected sensor**. Firmware establishes a bonded BLE connection,
   enables beacon mode, writes the new PIN, reconnects with PIN authentication,
   and reads the newly generated BTHome key. Keys are stored on Shelly and are
   never included in the web status response. PINs are not saved in configuration.
6. Wait for **Sensor paired and encrypted state verified**. Completion requires
   an authenticated encrypted state advertisement, not just a GATT connection.
   Wake the sensor if necessary. The pairing operation can be cancelled or can
   time out; errors remain visible in the panel.
7. Repeat for the second endpoint. Enable **BLU endpoint sensors**, set the
   timeout (default 180 seconds, 30–3600 allowed), and save. Saving the general
   BLU configuration restarts the device; pairing saves its own assignment.

Discovery lasts 60 seconds, connection/configuration has a 90-second deadline,
and encrypted-beacon verification can wait for up to 180 seconds. The sensors
remain encrypted if setup fails after the PIN write: retry using that new PIN,
or factory-reset the sensor. A stale bond can be replaced once on the explicitly selected outgoing pairing
connection; this does not bypass PIN authentication. If reconnection requires
physical pairing mode again, the panel asks you to hold the sensor button again.

For mounting, the closing sensor must detect its magnet only at the fully
closed endpoint, and the opening sensor only at the fully open endpoint.
Wired polarity, swap and open-sensor-disable settings do not affect BLU roles.

## Authentication and replay protection

Only AES-CCM-authenticated BTHome v2 advertisements from each assigned MAC and
key are used. Plaintext packets are rejected, including for previously paired
MACs. Unknown, malformed and unsupported contact data do not update state.

Each accepted encrypted counter must be strictly greater than its predecessor.
The counter is committed to NVS **before** the authenticated payload is exposed
to the application. A failed commit rejects the message. This also prevents a
captured packet being accepted again after controller restart. There is no
counter-wrap or counter-reset exception; rotate the key by pairing with a new PIN when needed.

Each role's key, MAC and counter are stored in the `blu_secure` NVS namespace,
separate from the JSON configuration and FS RPCs. This is not encrypted storage
against physical flash extraction. The Shelly factory-reset routine erases the
namespace; it does not reset the external BLU sensors. Persisting every new
counter trades NVS writes for replay protection; flash endurance and timing
must be assessed with actual beacon rates during hardware validation.

New-device enrollment begins with physical pairing mode and BLE bonding; the
initial trust step is not a claim of cryptographic device identity. Existing
encrypted devices require PIN authentication. The new PIN rotates the BTHome
key so that old-key recordings cannot bootstrap a newly enrolled device.

Before both first readings, after a timeout, while BLU pairing is active, on
scan failure or with contradictory endpoints, BLU-controlled garage commands
are blocked. HomeKit's current position read returns an error when data is
unreliable. This prevents new relay pulses; it does not physically stop a motor
already moving or replace the motor controller's safety functions.

Protocol references:
- [Shelly common features and GATT UUIDs](https://shelly-api-docs.shelly.cloud/docs-ble/common/)
- [Shelly encryption](https://shelly-api-docs.shelly.cloud/docs-ble/encryption/)
- [BTHome AES-CCM and replay requirements](https://bthome.io/encryption/)

## Build and checks on this Mac

Mongoose OS CLI: `~/.mos/bin/mos`, built natively for Apple Silicon from official
`mongoose-os/mos`, commit `b44964e63a926c1ac2af7496c8749555d6c3e166`.
USB dependencies are under `~/.mos/deps`. No Homebrew installation is needed.
The official prebuilt download URLs returned HTTP 404 on 2026-10-22.

Start Docker Desktop and run:

```sh
./tools/build-blu.sh
./tests/run-blu-tests.sh
node --check fs_src/script.js
```

The build script stages sources under `~/.mos/work/shelly-garage-blu` because
`mos` rejects paths containing spaces. It copies the firmware back to
`build_Shelly1Gen3/fw.zip`. This target retains the existing Gen3 pinout and
OTA layout. Do not flash this image onto another model.

Host tests use the actual firmware decoder and AES-CCM implementation with
an NVS test adapter. They verify a published BTHome interoperability vector,
modified ciphertext/MIC, wrong identity, truncation, plaintext rejection,
failed persistence, replay in the same process and after a process restart,
sensor freshness/conflicts, and 100,000 arbitrary payloads under ASan/UBSan.
They do not simulate the NimBLE connection or security state machine.

## Local previews

`python3 tools/simulator/panel-preview.py` serves the original firmware panel
at `http://127.0.0.1:18766` with a clearly marked mock RPC adapter. The **Pair**
flow here is a UI demonstration and does not change any sensor or store a key.

`./tools/simulator/start.sh` serves the separate door-motion demonstration
at `http://127.0.0.1:18765`. It uses the real plaintext decoder for generated
sample packets and a separate motion model. It does not test the new secure
receive path, BLE hardware, ESP32 execution or HomeKit networking.

## Remaining hardware validation

Verify physical discovery, Just Works enrollment of reset sensors, PIN
rotation/reconnection, wrong-PIN failure, beacon interval, timeout/cancellation,
power loss during persistence, NVS endurance, runtime heap, Wi-Fi/BLE
coexistence, metal-door reception and end-to-end HomeKit/relay behavior.

## Build verification — 2026-10-22

Success, built S1G3/esp32c3 version 2.16.1 (20261022-202353).

App size: 1356768 of 2818048 (48%). ZIP integrity and local security/decoder tests passed.
BLE pairing on physical hardware remains unverified.

## OTA hosting — hejsiri.pl

The Shelly1Gen3 web panel uses `https://hejsiri.pl/firmware/garage-blu/update.json`.
Other model panels retain the upstream update channel. Garage BLU releases use
`2.16.1-bluN` versions so subsequent BLE builds can be compared by the existing
version parser. Publish only the tested build intended for this model.

After `bash tools/build-blu.sh`, run `python3 tools/ota/prepare.py` to generate
`releases/hosting/garage-blu/`: versioned ZIP, SHA-256, channel JSON, metadata,
static download page and CORS/preflight configuration. The output contains no
sensor keys, configuration backups or device credentials. Upload this directory
to `domains/hejsiri.pl/public_html/firmware/garage-blu/` on the active hosting.
Verify downloaded ZIP SHA-256, range requests and cross-origin preflight after upload.
Hosting a ZIP does not initiate an update on any device.

### Published build — 2026-10-23

Built S1G3/esp32c3 `2.16.1-blu1` (`20261023-062416`). ZIP: 2,241,371 bytes;
application: 1,356,752 / 2,818,048 bytes. Includes the pairing status colors
and hejsiri.pl update channel. SHA-256:
`2d9bcd027678f7ae74349cc295e50e8b1444f1b3d4c1c12178168a838be3ece9`.

Published to the active Hostido document root. Public HTTPS download matches
this checksum; JSON CORS headers, OPTIONS preflight (204), and ZIP byte-range
requests (206) verified. No device was flashed; hardware validation is pending.
