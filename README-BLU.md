# Garage BLU — configuration and development

Garage BLU adds encrypted Shelly BLU devices and a garage dashboard to
[Shelly-HomeKit](https://github.com/mongoose-os-apps/shelly-homekit).
The current OTA release is **2.16.1-blu38 for Shelly Plus 1**. Shelly 1 Gen3
support remains in the source; new Gen3 builds and hardware checks are paused.
HomeKit uses Wi-Fi. BLU devices keep their original Shelly firmware.

User documentation (Polish, with the English UI labels):
[install/update](docs/AKTUALIZACJA.md), [configuration](docs/KONFIGURACJA.md),
[troubleshooting](docs/ROZWIAZYWANIE_PROBLEMOW.md), and
[release history](docs/HISTORIA_ZMIAN.md). The current panel and this document
use **Obstruction**, **In range / Out of range**, **Ready / Pressed** and
**Day / Night / Auto**. User-entered accessory names retain their language.

Previous versions have run on a physical Plus 1, with OTA and encrypted
Door/Window readings confirmed. Motion and Button1 readings have also been
observed in the panel. This release has host-side checks and a local UI preview;
its changes still need verification after installation on a physical garage.
A preview does not demonstrate successful BLE pairing or relay operation.

## Supported devices

| Device | Function |
| --- | --- |
| Shelly Plus 1 | Garage relay, BLE receiver, web panel and HomeKit connection; current build target. |
| Shelly 1 Gen3 | Separate ESP32-C3 target retained in source; no new release package. |
| Two BLU Door/Window, SBDW-002C | Fully closed and fully open endpoints. |
| One BLU Motion, SBMO-003Z | Motion in the panel and an optional HomeKit Motion Sensor service. |
| Two BLU H&T, SBHT-003C | Indoor/outdoor temperature and humidity in the panel and HomeKit. |
| One BLU Button1 | Single press: open / stop / close. Authenticated beacons: presence in BLE range. |

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

## Pair and name devices

1. Configure an administrator password and select **Garage Door Opener** mode.
   Pairing RPCs require authentication to be configured. The local panel uses
   HTTP/WebSocket, so use a trusted local network.
2. Click **Pair** beside the required role. Discovery starts in a modal.
3. Hold the device button for more than 10 seconds, then release it. One matching
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
   uses the first **Name** field in that section. Empty device names restore the default label.
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

Button1 beacons control only the picture of the car. A new authenticated packet
shows it; exceeding the presence timeout (default 45 seconds) hides it. The
manual show/hide button lasts until the next packet or presence transition.
Repeated status polls of the same packet preserve that manual choice. Presence
means the button is in BLE range, not proof of an exact parking location.
Beacons never trigger the relay. A fresh single press can trigger the normal
open / stop / close action when the garage inputs are ready.

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
panel regeneration while retaining the SDK cache. To build the retained Gen3
source, set `SHELLY_BLU_MODEL=Shelly1Gen3`; its image goes to
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

Run `python3 tools/ota/prepare.py --model ShellyPlus1` after a successful build.
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
New Gen3 hardware validation remains paused.
