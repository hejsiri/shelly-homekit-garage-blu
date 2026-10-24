# Garage BLU — configuration and development

Garage BLU adds encrypted Shelly BLU devices, Xiaomi thermometers and a garage dashboard to
[Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit).
The current OTA release is **2.16.1-blu49 for Shelly Plus 1 and Shelly 1 Gen3**,
with separate model-specific ZIPs and checksums. The Gen3 image targets ESP32-C3;
its physical-device validation remains pending.
HomeKit uses Wi-Fi. BLU devices keep their original Shelly firmware.

User documentation (Polish, with the English UI labels):
[Shelly 1 Gen3 guide](docs/SHELLY_1_GEN3.md),
[install/update](docs/AKTUALIZACJA.md), [configuration](docs/KONFIGURACJA.md),
[troubleshooting](docs/ROZWIAZYWANIE_PROBLEMOW.md), and
[release history](docs/HISTORIA_ZMIAN.md). The current panel and this document
use **Obstruction**, **In range / Out of range**, **Ready / Pressed** and
**Day / Night / Auto**. User-entered accessory names retain their language.

Previous versions have run on a physical Plus 1, with OTA and encrypted
Door/Window readings confirmed. Motion and Button1 readings have also been
observed in the panel. This release has host-side checks, including web panel behavior;
its changes still need verification after installation on a physical garage.
A preview does not demonstrate successful BLE pairing or relay operation.

## Supported devices

| Device | Function |
| --- | --- |
| Shelly Plus 1 | Garage relay, BLE receiver, web panel and HomeKit connection; ESP32 / Plus1 image. |
| Shelly 1 Gen3 | Current separate ESP32-C3 / S1G3 image with the same Garage BLU features; physical tests pending. |
| Two BLU Door/Window, SBDW-002C | Fully closed and fully open endpoints. |
| One BLU Motion, SBMO-003Z | Motion in the panel and an optional HomeKit Motion Sensor service. |
| Two BLU H&T, SBHT-003C | Indoor/outdoor temperature and humidity in the panel and HomeKit. |
| Xiaomi LYWSD03MMC (experimental) | Stock firmware direct GATT reads without a PIN, bindkey or Xiaomi account. Indoor/Outdoor H&T only. B1.4 / 2.1.1_0159 needs a physical test. |
| Xiaomi LYWSDCGQ/01ZM (MJ_HT_V1) | Stock unencrypted MiBeacon temperature, humidity and battery. Replaces either Indoor or Outdoor BLU H&T; two thermometer slots total. No PIN, Xiaomi account or sensor firmware change. |
| One BLU Button1 | Single press: open / stop / close. HomeKit occupancy and programmable button services (single / double / long press) within the garage accessory. Authenticated beacons determine presence in BLE range. |

Other controller variants, including Mini, 1PM and Gen4, are not supported by
these packages. Door/Window Outdoor ZB (SBDW-302C) and a BLU leak sensor remain
planned. H&T Gen3 with an e-ink screen is a Wi-Fi device, not this BLU H&T.

## Install and update

Follow the [installation instructions](docs/AKTUALIZACJA.md) and use the
[Garage BLU OTA channel](https://hejsiri.pl/firmware/garage-blu/).
Only the controller receives this firmware; do not upload it to BLU devices.
The S1G3 and Plus1 packages are not interchangeable. Upstream packages do not
include Garage BLU features.

The panel supports **Check → Install** and ZIP file upload. It authenticates
before uploading, waits for the device response and reports failures/timeouts.
The garage dashboard is hidden during an update. With device authentication
enabled, it loads only after access to the protected full-status RPC succeeds;
logout or loss of the connection unloads it and clears cached dashboard data.

## Xiaomi LYWSDCGQ/01ZM

Since blu45, choose **Xiaomi LYWSDCGQ/01ZM** in the **Indoor H&T** or
**Outdoor H&T** type selector, then click **Pair**. Leave the thermometer powered
on near Shelly. Discovery and registration use its normal broadcasts; do not
hold its button or enter a PIN. A single matching device is added automatically;
when several match, select the intended MAC. The modal finishes only after both
temperature and humidity have arrived. Enter **Name** and save to name its readings.

The sensor keeps its stock firmware. This selector uses MiBeacon product `0x01aa`.
Other Xiaomi, BLE Mesh and Zigbee models are not enabled by this option. Temperature is shown at 0.1 °C and
humidity is rounded to integer percent, matching the existing H&T display.

A persisted protocol marker keeps passive Xiaomi assignments separate from
encrypted BTHome keys. RPC reports `paired: true`, `secure: false` and
`protocol: "xiaomi"`. Unencrypted packets cannot control the garage or substitute
for paired endpoints, Motion or Button1. Each measurement has its own timestamp;
battery-only packets do not refresh old temperature/humidity. HomeKit keeps the
existing Indoor/Outdoor service IDs and reports Status Fault for stale values.

Parser tests use a published stock-device vector from
[Bluetooth-Devices/xiaomi-ble](https://github.com/Bluetooth-Devices/xiaomi-ble/blob/main/tests/test_parser.py).
A physical LYWSDCGQ/01ZM was observed on Plus1/blu45: live temperature, humidity
and battery (36 %) appeared in the web panel. This did not test physical relay
operation or a full hardware test of the current release.

## Xiaomi LYWSD03MMC: experimental key-free GATT reads (blu49)

Choose **Xiaomi LYWSD03MMC (experimental)** in either H&T selector and click **Pair**.
Keep the stock thermometer powered near Shelly and close other connected Bluetooth
apps. No pairing-mode button press, PIN, manual MAC, cloud login or bindkey is required.
One matching device continues automatically; several require selecting the intended MAC.
Shelly verifies the Device Information model and a valid five-byte measurement before
saving the assignment. Failed probes keep the previous assignment.

The client reads characteristics by UUID, without fixed ATT handles, GATT writes or
SMP initiation. It reads model `0x2a24`, optional firmware/hardware `0x2a26` / `0x2a27`,
then `ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6` (or the stock mirror
`8edfffef-3d1b-9c37-4623-ad7265f14076`). Data is signed little-endian temperature in
hundredths of a degree, humidity percent and little-endian battery millivolts.
The panel marks battery percent `~` because it is an estimate from 2.1–3.1 V.
[The reference implementation](https://github.com/ManiAm/raspi-enviro-watch#reading-ble-device-characteristics)
demonstrates stock **2.1.1_0159 / B1.5**. **B1.4 / 2.1.1_0159 is not yet verified**.

Only one 10-second read transaction runs at a time, with a 3-second connection timeout.
Polling runs every 300 seconds, retries back off 60/120/300 seconds, and movement defers
or cancels reads. Scanning resumes once connected; a bounded 4-second scan grace keeps
fresh endpoints valid during connection setup, without extending their stale timeout.
GATT thermometer freshness is at least 900 seconds; failures never refresh old readings.
RPC reports `protocol: "xiaomi_gatt"`, `paired: true`, `secure: false`, per-role
`timeout_s`, `voltage_mv`, `battery_estimated`, and `gatt` diagnostics (versions/error).
HomeKit reuses existing H&T service IDs; each field retains normal fault/stale handling.

NVS protocol **4** identifies this key-free client. Withdrawn manual-key protocols **2/3**
remain rejected. Stock encrypted MiBeacon readings are not decrypted, old blu46 bindings
are not restored automatically, and **MCCGQ02HL remains unsupported**. Host tests exercise
the production asynchronous client, malformed payloads, identity, mirror fallback,
timeouts, cancellation, late callbacks, movement deferral, assignment commit/rollback,
role isolation and unchanged endpoint freshness checks. Physical validation is pending.

## Pair and name devices

1. Configure an administrator password and select **Garage Door Opener** mode.
   Pairing RPCs require authentication to be configured. The local panel uses
   HTTP/WebSocket, so use a trusted local network.
2. Click **Pair** beside the required role. Discovery starts in a modal.
3. For Shelly BLU, hold the device button for more than 10 seconds, then release it. Xiaomi thermometers use the key-free steps above. One matching
   device is selected automatically. If several match, choose the intended one.
4. No PIN is requested by default. The modal asks for one only if the device
   requires legacy authentication or PIN-based encryption activation.
5. For the same saved MAC/key, a new correctly decrypted reading can confirm
   the existing configuration without another BLE security exchange. Otherwise
   the controller bonds, enables beacons and reads the key. A device with no key
   and a writable key characteristic can receive a randomly generated key.
6. Completion requires a fresh, authenticated encrypted packet. A BLE connection
   alone is not success. If the radio sleeps, the modal asks for pairing mode
   again and resumes automatically on a fresh advertisement from that device.
   Cancellation and errors remain visible; a retry does not erase unrelated bonds.
7. After pairing, **Name** appears in a new row below **Pair**, before the status.
   Enter a name and use **Save** in the garage section. It is stored on Shelly
   and used on Motion/Button1 tiles and H&T readings. Endpoint names remain in
   settings; endpoint readings are not displayed above the garage. The garage tile
   uses the first **Name** field in that section. Button1 has a separate **Car name**
   field for the car presence tile. Empty Button1 fields show **Button** and
   **Name of car** placeholders; save stores empty values and restores default tile labels.
8. Pair both endpoint devices, enable **BLU sensors**, set the freshness timeout
   (default 180 seconds, permitted 30–3600), and save.

Names are limited to 64 UTF-8 bytes. Renaming keeps the encrypted keys and live
readings. Motion and H&T HomeKit services also use these names; Apple Home may
retain names that were manually assigned there. Optional services exist only
for securely paired devices, retain stable instance IDs, and report a fault
rather than disappear when the device stops responding.

The closing contact must detect its magnet only at the fully closed endpoint,
and the opening contact only at the fully open endpoint. Wired polarity/swap
settings do not change these BLU role assignments. A failed new pairing attempt
can coexist with working readings from an earlier saved key; the panel reports
recent encrypted readings separately from the result of that attempt.

## Dashboard and garage state

The dashboard receives state from the same firmware component as HomeKit.
After a movement timeout, either valid endpoint clears both the timeout flag
and HomeKit obstruction notification. Missing, stale or conflicting endpoints
cannot clear the fault. The animation estimates travel between endpoints from
the configured movement time; the endpoint contacts confirm the actual limits.
An obstruction displays a fixed 50% position as a visual fault indicator.
The scene is revealed only after all current theme images are loaded and decoded;
legacy house/car layers are not requested during page startup.

The **Continue previous direction after stop** switch chooses the direction of
the next relay impulse after a stop. Match it to the motor controller's behavior;
it does not reprogram the motor controller.

Button1 beacons update the car picture and a HomeKit occupancy service. A new authenticated packet
shows it; exceeding the presence timeout (default 45 seconds) hides it. The
manual show/hide button lasts until the next packet or presence transition.
It updates both the car picture and the dashboard's Car presence tile immediately:
hiding the car makes the tile inactive with Not detected. This temporary dashboard
choice does not change the actual HomeKit occupancy reading.
Repeated status polls of the same packet preserve that manual choice. Presence
means the button is in BLE range, not proof of an exact parking location.
Beacons never trigger the relay. A fresh single press can trigger the normal
open / stop / close action when the garage inputs are ready.

After securely pairing Button1, HomeKit exposes two services inside the existing
Shelly garage accessory, alongside Motion and H&T:

- **Car presence** (Occupancy Sensor): uses **Car name** and the same authenticated
  presence state as the dashboard. Missing beacons for 45 seconds mean normal
  absence; unavailable Bluetooth or a stopped BLE scan reports a fault.
- **Button** (Stateless Programmable Switch): uses Button1 **Name** and sends
  single, double and long presses, configurable in Apple Home. A single press
  also retains direct garage control. Double and long presses only send HomeKit
  events. Beacon packets and unsupported press types do not trigger button events.

Already paired Button1 devices gain these services after updating and restarting
the controller. No HomeKit reset or new BLU pairing is required. The garage's IDs
are preserved. The blu42 migration replaces Motion, Car presence and Button with
fresh service identities, retiring records that Apple Home has mixed between
Motion and Occupancy. It increments the configuration number once. Check and
reassign button actions and motion/presence automations after this migration.
On the affected Plus 1, blu42 was installed and the live database was inspected:
Apple Home now shows the garage, active/ready Motion, detected Car presence and
Button with single/double/long actions available. Physical press delivery for all
three event types and a full garage operation check remain pending.

Authenticated `Shelly.GetDebugInfo` now includes the actual running HAP database:
accessory/service/characteristic IDs, types, names and BLU source readiness.
It does not include pairing keys or credentials. This lets diagnostics distinguish
missing firmware services from conflicting controller records.

Motion and occupancy expose typed readings, Status Fault and Status Active even
before a fresh beacon. During a reception failure they retain their last reading
with a fault and inactive status. They do not report a valid absence from a stopped
scanner. The programmable button's read value remains null as required for IP HomeKit.

Since blu43, a securely paired BLU Motion also adds **Light sensor** to the same
garage accessory. It reads BTHome 0x05 (uint24, 0.01 lux) only from that Motion's
authenticated packets. Door/Window light readings do not create HomeKit services.
Illuminance has its own freshness timestamp: packets without lux retain the last
sample without renewing its age. Missing/stale samples or unavailable reception
set Status Fault and inactive status. HomeKit values are bounded to 0.0001–100000 lx.
The update adds a new service at IID 0x1b00 and refreshes the configuration number
once; the existing garage, motion, car and button identities remain unchanged.
No new BLE pairing or HomeKit reset is needed. Physical light reporting in this
release remains to be verified after installation.

Event mapping follows the [BTHome event definitions](https://bthome.io/format/#events)
and [Apple programmable switch events](https://github.com/apple/HomeKitADK/blob/master/HAP/HAPCharacteristicTypes.h).

## Authentication and replay protection

Only AES-CCM-authenticated BTHome v2 packets from the assigned MAC and key enter
the receive path. Plaintext, malformed payloads and replayed counters are rejected.
Each accepted encrypted counter is committed to NVS before the payload is exposed
to the application. A failed commit rejects the packet. There is no counter-wrap
or restart bypass. Re-pairing with the same key retains its counter history.

Each role's key, MAC and counter are in the `blu_secure` NVS namespace, separate
from JSON configuration and status RPCs. PINs are not saved in configuration.
This storage does not prevent physical flash extraction. The controller's
factory reset erases this namespace without resetting external BLU devices.
Persisting counters trades flash writes for replay protection; endurance and
radio timing still need assessment with real beacon rates.

Physical pairing mode and BLE bonding authorize enrollment. Required link
security and legacy authentication are preserved; not displaying an unnecessary
PIN field does not disable them. Existing bonds are renewed only for the
selected peer when the recovery path calls for it.

In BLU endpoint mode, commands are blocked before reliable endpoint readings,
during pairing, after a timeout in sensor freshness, on scan failure, or with
contradictory endpoints.
HomeKit current-position reads return an error when data is unreliable. Blocking
new relay impulses does not stop a motor already moving or replace its protections.

Protocol references:
- [Shelly BLE common features and GATT UUIDs](https://shelly-api-docs.shelly.cloud/docs-ble/common/)
- [Shelly encryption](https://shelly-api-docs.shelly.cloud/docs-ble/encryption/)
- [BTHome AES-CCM and replay requirements](https://bthome.io/encryption/)

## Build and test

Start Docker Desktop. The current default target is Shelly Plus 1:

```sh
bash tools/build-blu.sh
bash tests/run-blu-tests.sh
node --check fs_src/script.js
node tests/blu_pairing_ui_test.js
node tests/garage_auth_ui_test.js
node tests/garage_dashboard_ui_test.js
node tests/garage_visualization_ui_test.js
node tests/garage_car_presence_test.js
node tests/garage_named_status_test.js
node tests/garage_scene_loading_test.js
node tests/blu_button_indicator_test.js
node tests/ota_upload_ui_test.js
node tests/garage_simulator_stop_test.js
python3 tests/garage_http_assets_test.py
```

The helper uses `~/.mos/bin/mos`, stages sources under
`~/.mos/work/shelly-garage-blu` to avoid spaces in the build path, and copies
`build_ShellyPlus1/fw.zip` back to the checkout. It forces application and web
panel regeneration while retaining the SDK cache. To build the Gen3
target, set `SHELLY_BLU_MODEL=Shelly1Gen3`; its image goes to
`build_Shelly1Gen3/fw.zip`. Never interchange these targets.

Host checks cover the actual decoder and AES-CCM/replay implementation,
persistence failures/restarts, arbitrary payloads under ASan/UBSan, pairing
recovery helpers, strict status JSON, optional HomeKit services, endpoint
recovery, name validation, modal behavior, authorization and OTA errors.
They do not replace NimBLE, RF, relay or end-to-end Apple Home tests.

## Local previews

`python3 tools/simulator/panel-preview.py` serves the original panel at
`http://127.0.0.1:18766` with a clearly marked mock adapter. Pairing, saving names
and beacons here are simulated and do not contact Shelly.
`./tools/simulator/start.sh` serves the separate door-motion demonstration at
`http://127.0.0.1:18765` using a separate motion model.

## Publish OTA

Build both models before preparing a release:

```sh
bash tools/build-blu.sh
SHELLY_BLU_MODEL=Shelly1Gen3 bash tools/build-blu.sh
python3 tests/ota_release_models_test.py
python3 tools/ota/prepare.py
```

The publisher selects both models by default. Explicit `--model` options can
select a subset, but the public channel should keep both model URLs. It validates
model/platform, part hashes, slot/filesystem sizes and matching versions before
writing any release artifact or metadata.
It generates versioned ZIP/SHA-256 files, channel JSON, release metadata,
the download page, current user guides and CORS settings under
`releases/hosting/garage-blu/`. The output contains no device configuration,
keys or credentials. Download-page copy is maintained in
`tools/ota/landing.html`; the user guides come from `docs/*.md`.

For a firmware release, update current version/download information and the
package table in the installation guide from the actual manifest and checksum.
Keep old behavior in the release history. Confirm the model, ZIP integrity,
part hashes and sizes before publishing.

For documentation-only updates, use the existing release ZIP, confirm that its
SHA-256 and the channel/release JSON files are unchanged, then upload only the
HTML page and guides. Do not create a new firmware version for text-only repository
or hosting documentation changes. Existing installations read the updated guides
without an OTA install. English panel labels in the guides must match `fs_src`.

Upload the ZIP and checksum first to `domains/hejsiri.pl/public_html/firmware/garage-blu/`.
Verify the public ZIP hash and size, then publish metadata and switch `update.json`
last. Check public JSON, byte-range requests (206) and OPTIONS preflight (204).
Publishing a package does not install it on any device. Publishing documentation
only does not replace the ZIP or change the channel. Project and issue links point
to [hejsiri/shelly-homekit-garage-blu](https://github.com/hejsiri/shelly-homekit-garage-blu);
upstream links are references/attribution.

## Remaining hardware validation

Check the complete pairing flow on Door/Window and Button1 1.0.23, sleep/wake
recovery, encrypted packets after reboot, name persistence and HomeKit labels,
button presence transitions, late endpoint recovery after a timeout, OTA,
Wi-Fi/BLE coexistence, runtime heap and reception near the metal door.
The Gen3 build and archive are checked; physical Gen3 installation, BLE and relay validation remain pending.
