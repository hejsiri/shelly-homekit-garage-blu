# Shelly-HomeKit Update Server

This is an inherited upstream tool, not the active Garage BLU update service.
Garage BLU publishes static, model-specific files with `tools/ota/prepare.py`
to hejsiri.pl. See the current [publishing instructions](../../README-BLU.md#publish-ota).
Running this Go server is not required for Garage BLU installation or updates.

## Overview

This server is intended as a single entry point for all update requests and will contain
the logic for serving the right update based on what the device is currently running.

Initial version will handle stock -> HomeKit upgrades but eventually HK update logic will be moved here as well,
replacing the current JSON + version regex matching done in UI.

## Running

`go build -v && ./shelly_update_server --listen-addr=:8080 --stock-model-map=stock_model_map.yml`

## Endpoints

 * `/update` - this is the main entry point for all device updates, will examine headers and will redirect to the actual ZIP file to update from (according to model and location template from `--dest-url-template`.
 * `/log` - will show recent history of requests from this address, for debugging.
