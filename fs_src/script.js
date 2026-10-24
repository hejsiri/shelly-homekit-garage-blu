/*
 * Copyright (c) Shelly-HomeKit Contributors
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Options.
const maxAuthAge = 24 * 60 * 60;
const updateCheckInterval = 24 * 60 * 60;
const uiRefreshInterval = 1;
const rpcRequestTimeoutMs = 10000;

// Globals.
let lastInfo = null;
let infoLevel = 0;
let host = null;
let socket = null;
let isConnected = false;

let pauseAutoRefresh = false;
let pendingGetInfo = false;
let updateInProgress = false;
let lastFwBuild = "";

let pendingRequests = {};

let nextRequestID = Math.ceil(Math.random() * 10000);
let authRequired = false;

const authInfoKey = "auth_info";
const authUser = "admin";
let authRealm = null;
let rpcAuth = null;

// Keep in sync with shelly::Component::Type.
class Component_Type {
  static kSwitch = 0;
  static kOutlet = 1;
  static kLock = 2;
  static kStatelessSwitch = 3;
  static kWindowCovering = 4;
  static kGarageDoorOpener = 5;
  static kDisabledInput = 6;
  static kMotionSensor = 7;
  static kOccupancySensor = 8;
  static kContactSensor = 9;
  static kDoorbell = 10;
  static kLightBulb = 11;
  static kTemperatureSensor = 12;
  static kLeakSensor = 13;
  static kSmokeSensor = 14;
  static kCarbonMonoxideSensor = 15;
  static kCarbonDioxideSensor = 16;
  static kMax = 17;
};

// Keep in sync with shelly::LightBulbController::BulbType.
class LightBulbController_BulbType {
  static kWhite = 0;
  static kCCT = 1;
  static kRGBW = 2;
  static kMax = 3;
};

function el(container, id) {
  if (id === undefined) {
    id = container;
    container = document;
  }
  return container.querySelector(`#${id}`);
}

function checkName(name) {
  return !!name.match(/^[a-z0-9\-]{1,63}$/i)
}

el("sys_save_btn").onclick = function() {
  if (!checkName(el("sys_name").value)) {
    alert(
        "Name must be between 1 and 63 characters " +
        "and consist of letters, numbers or dashes ('-')");
    return;
  }
  let data = {
    config: {
      name: el("sys_name").value,
      sys_mode: parseInt(el("sys_mode").value),
    },
  };
  el("sys_save_spinner").className = "spin";
  pauseAutoRefresh = true;
  callDevice("Shelly.SetConfig", data)
      .then(function() {
        setTimeout(() => {
          el("sys_save_spinner").className = "";
          pauseAutoRefresh = false;
          resetLastSetValue();
          refreshUI();
        }, 1300);
      })
      .catch(function(err) {
        el("sys_save_spinner").className = "";
        if (err.message) err = err.message;
        pauseAutoRefresh = false;
        alert(err);
      });
};

el("hap_setup_btn").onclick = function() {
  el("hap_setup_spinner").className = "spin";
  // Generate a code from device ID, wifi network name and password.
  // This way it remains stable but cannot be easily guessed from device ID
  // alone.
  let input = lastInfo.device_id + (lastInfo.wifi_ssid || "") +
      (lastInfo.wifi_pass_h || "");
  // Remove non-alphanumeric chars,
  // https://github.com/mongoose-os-apps/shelly-homekit/issues/1216
  input = input.replace(/[^a-z0-9]/gi, "");
  let seed = sha256(input).toLowerCase();
  let code = "", id = "";
  for (let i = 0; i < 8; i++) {
    code += (seed.charCodeAt(i) % 10);
    if (i == 2 || i == 4) code += "-";
  }
  for (let i = 0; i < 4; i++) {
    let si = (seed.charCodeAt(10 + i) + seed.charCodeAt(20 + i)) % 36;
    id += "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ".charAt(si);
  }
  console.log(input, seed, code, id);
  callDevice("HAP.Setup", {"code": code, "id": id})
      .then(function(info) {
        console.log(info);
        if (!info) return;
        el("qrcode_text_1").textContent =
            info.code.replace(/-/g, "").substring(0, 4);
        el("qrcode_text_2").textContent =
            info.code.replace(/-/g, "").substring(4);
        el("qrcode").innerText = "";
        new QRCode(el("qrcode"), {
          text: info.url,
          width: 160,
          height: 160,
          colorDark: "black",
          colorLight: "white",
          correctLevel: QRCode.CorrectLevel.Q,
        });
        el("hap_setup_info").style.display = "block";
        resetLastSetValue();
        refreshUI();
      })
      .catch(function(err) {
        if (err.message) err = err.message;
        alert(err);
      })
      .finally(function() {
        el("hap_setup_spinner").className = "";
      });
};

el("hap_reset_btn").onclick = function() {
  if (!confirm(
          "This will erase all pairings and clear setup code. " +
          "Are you sure?")) {
    return;
  }

  el("hap_reset_spinner").className = "spin";
  el("hap_setup_info").style.display = "none";
  callDevice("HAP.Reset", {"reset_server": true, "reset_code": true})
      .then(function() {
        el("hap_reset_spinner").className = "";
        resetLastSetValue();
        refreshUI();
      })
      .catch(function(err) {
        if (err.message) err = err.message;
        alert(err);
      });
};

el("fw_upload_btn").onclick = function() {
  let ff = el("fw_select_file").files;
  if (ff.length == 0) {
    alert("No files selected");
    return false;
  }
  uploadFW(ff[0], el("fw_spinner"), el("update_status"));
  return false;
};

el("wifi_save_btn").onclick = function() {
  el("wifi_spinner").className = "spin";
  let sta_static = el("wifi_ip_en").checked;
  let sta1_static = el("wifi1_ip_en").checked;
  let data = {
    sta: {
      enable: el("wifi_en").checked,
      ssid: el("wifi_ssid").value,
      ip: (sta_static ? el("wifi_ip").value : ""),
      netmask: (sta_static ? el("wifi_netmask").value : ""),
      gw: (sta_static ? el("wifi_gw").value : ""),
      nameserver: el("wifi_nameserver").value,
    },
    sta1: {
      enable: el("wifi1_en").checked,
      ssid: el("wifi1_ssid").value,
      ip: (sta1_static ? el("wifi1_ip").value : ""),
      netmask: (sta1_static ? el("wifi1_netmask").value : ""),
      gw: (sta1_static ? el("wifi1_gw").value : ""),
      nameserver: el("wifi1_nameserver").value,
    },
    ap: {
      enable: el("wifi_ap_en").checked,
      ssid: el("wifi_ap_ssid").value,
    },
  };
  if (el("wifi_pass").value != lastInfo.wifi_pass) {
    data.sta.pass = el("wifi_pass").value;
  }
  if (el("wifi1_pass").value != lastInfo.wifi1_pass) {
    data.sta1.pass = el("wifi1_pass").value;
  }
  if (el("wifi_ap_pass").value != lastInfo.wifi_ap_pass) {
    data.ap.pass = el("wifi_ap_pass").value;
  }
  data.sta_ps_mode = parseInt(el("wifi_sta_ps_mode").value);
  callDevice("Shelly.SetWifiConfig", data)
      .then(function(q) {
        el("wifi_conn_rssi_container").style.display = "none";
        el("wifi_conn_ip_container").style.display = "none";
        resetLastSetValue();
        refreshUI();
      })
      .catch(function(err) {
        el("wifi_spinner").className = "";
        if (err.message) err = err.message;
        alert(err);
        console.log(err);
      });
};

function setComponentConfig(c, cfg, spinner) {
  if (spinner) spinner.className = "spin";
  let data = {
    id: c.data.id,
    type: c.data.type,
    config: cfg,
  };
  pauseAutoRefresh = true;
  callDevice("Shelly.SetConfig", data)
      .then(function() {
        setTimeout(() => {
          if (spinner) spinner.className = "";
          pauseAutoRefresh = false;
          resetLastSetValue();
          refreshUI();
        }, 1300);
      })
      .catch(function(err) {
        if (spinner) spinner.className = "";
        if (err.message) err = err.message;
        alert(err);
        pauseAutoRefresh = false;
      });
}

function setComponentState(c, state, spinner) {
  if (spinner) spinner.className = "spin";
  let data = {
    id: c.data.id,
    type: c.data.type,
    state: state,
  };
  callDevice("Shelly.SetState", data)
      .then(function() {
        if (spinner) spinner.className = "";
        resetLastSetValue();
        refreshUI();
      })
      .catch(function(err) {
        if (spinner) spinner.className = "";
        if (err.message) err = err.message;
        alert(err);
      });
}

function autoOffDelayValid(value) {
  parsedValue = dateStringToSeconds(value);
  return (parsedValue >= 0.010) && (parsedValue <= 2147483.647);
}

function dateStringToSeconds(dateString) {
  if (dateString == "") return 0;

  let {days, hours, minutes, seconds, milliseconds} =
      dateString
          .match(
              /^(?<days>\d+)\:(?<hours>\d{2})\:(?<minutes>\d{2})\:(?<seconds>\d{2})\.(?<milliseconds>\d{3})/)
          .groups;

  return parseInt(days) * 24 * 3600 + parseInt(hours) * 3600 +
      parseInt(minutes) * 60 + parseInt(seconds) +
      parseFloat(milliseconds / 1000);
}

function secondsToDateString(seconds) {
  if (seconds == 0) return "";
  let date = new Date(1970, 0, 1);
  date.setMilliseconds(seconds * 1000);
  let dateString = Math.floor(seconds / 3600 / 24) + ":" +
      nDigitString(date.getHours(), 2) + ":" +
      nDigitString(date.getMinutes(), 2) + ":" +
      nDigitString(date.getSeconds(), 2) + "." +
      nDigitString(date.getMilliseconds(), 3);
  return dateString;
}

function nDigitString(num, digits) {
  return num.toString().padStart(digits, "0");
}

function rgbSetConfig(c) {
  let name = el(c, "name").value;
  let initialState = el(c, "initial").value;
  let svcHidden = el(c, "svc_hidden").checked;
  let autoOff = el(c, "auto_off").checked;
  let autoOffDelay = el(c, "auto_off_delay").value;
  let spinner = el(c, "save_spinner");

  if (name == "") {
    alert("Name must not be empty");
    return;
  }

  if (autoOff && autoOffDelay && !autoOffDelayValid(autoOffDelay)) {
    alert(
        "Auto off delay must follow 24 hour format D:HH:MM:SS.sss with a value between 10ms and 24 days.");
    return;
  }

  let cfg = {
    name: name,
    svc_hidden: svcHidden,
    initial_state: parseInt(el(c, "initial").value),
    auto_off: autoOff,
    in_inverted: el(c, "in_inverted").checked,
    transition_time: parseInt(el(c, "transition_time").value)
  };
  if (autoOff) {
    cfg.auto_off_delay = dateStringToSeconds(autoOffDelay);
  }
  if (c.data.in_mode >= 0) {
    cfg.in_mode = parseInt(el(c, "in_mode").value);
  }
  setComponentConfig(c, cfg, spinner);
}

function swSetConfig(c) {
  let name = el(c, "name").value;
  let svcType = el(c, "svc_type").value;
  let charType = el(c, "valve_type").value;
  let initialState = el(c, "initial").value;
  let autoOff = el(c, "auto_off").checked;
  let autoOffDelay = el(c, "auto_off_delay").value;
  let spinner = el(c, "save_spinner");

  if (name == "") {
    alert("Name must not be empty");
    return;
  }

  if (autoOff && autoOffDelay && !autoOffDelayValid(autoOffDelay)) {
    alert(
        "Auto off delay must follow 24 hour format D:HH:MM:SS.sss with a value between 10ms and 24 days.");
    return;
  }

  let cfg = {
    name: name,
    svc_type: parseInt(el(c, "svc_type").value),
    hk_state_inverted: el(c, "hk_state_inverted").checked,
    initial_state: parseInt(el(c, "initial").value),
    auto_off: autoOff,
    in_inverted: el(c, "in_inverted").checked,
    out_inverted: el(c, "out_inverted").checked,
  };
  if (autoOff) {
    cfg.auto_off_delay = dateStringToSeconds(autoOffDelay);
  }
  if (c.data.state_led_en >= 0) {
    cfg.state_led_en = el(c, "state_led_en").checked ? 1 : 0;
  }
  if (c.data.in_mode >= 0) {
    cfg.in_mode = parseInt(el(c, "in_mode").value);
  }
  cfg.valve_type = (svcType == 3) ? parseInt(el(c, "valve_type").value) : -1;
  setComponentConfig(c, cfg, spinner);
}

function sswSetConfig(c) {
  let name = el(c, "name").value;
  if (name == "") {
    alert("Name must not be empty");
    return;
  }
  let cfg = {
    name: name,
    type: parseInt(el(c, "type").value),
    inverted: el(c, "inverted").checked,
    in_mode: parseInt(el(c, "in_mode").value),
  };
  setComponentConfig(c, cfg, el(c, "save_spinner"));
}

function diSetConfig(c) {
  let cfg = {
    type: parseInt(el(c, "type").value),
  };
  setComponentConfig(c, cfg, el(c, "save_spinner"));
}

function tsSetConfig(c) {
  let name = el(c, "name").value;
  if (name == "") {
    alert("Name must not be empty");
    return;
  }
  let cfg = {
    name: name,
    unit: parseInt(el(c, "unit").value),
    update_interval: parseInt(el(c, "update_interval").value),
    offset: Math.round(parseFloat(el(c, "offset").value) * 100),
  };
  setComponentConfig(c, cfg, el(c, "save_spinner"));
}

function mosSetConfig(c) {
  let name = el(c, "name").value;
  if (name == "") {
    alert("Name must not be empty");
    return;
  }
  let cfg = {
    name: name,
    type: parseInt(el(c, "type").value),
    inverted: el(c, "inverted").checked,
    in_mode: parseInt(el(c, "in_mode").value),
    idle_time: parseInt(el(c, "idle_time").value),
  };
  setComponentConfig(c, cfg, el(c, "save_spinner"));
}

function wcSetConfig(c, cfg, spinner) {
  if (!cfg) {
    let name = el(c, "name").value;
    if (name == "") {
      alert("Name must not be empty");
      return;
    }
    cfg = {
      name: name,
      display_type: parseInt(el(c, "display_type").value),
      in_mode: parseInt(el(c, "in_mode").value),
      swap_inputs: el(c, "swap_inputs").checked,
      swap_outputs: el(c, "swap_outputs").checked,
    };
  }
  setComponentConfig(c, cfg, spinner);
}

function bluHTValues(sensor) {
  return sensor.temperature_ready !== false && sensor.humidity_ready !== false && Number.isFinite(sensor.temperature) && Number.isFinite(sensor.humidity) && sensor.humidity >= 0 ?
    `${sensor.temperature.toFixed(1)} °C · ${sensor.humidity}%` : "Waiting for measurement";
}
// Animation is only a time-based estimate; endpoint states come from firmware.
function garageRefresh(c, cd) {
  const view = c.garageMotionState || (c.garageMotionState = {});
  const now = performance.now(), old = view.motion;
  const blocked = cd.blu && cd.blu.enable && !cd.blu.ready;
  const state = blocked ? "unavailable" : cd.cur_state_str;
  const moving = state === "opening" || state === "closing";
  const signature = state + ":" + cd.move_time;
  if (view.signature === signature) return;
  const seconds = Math.max(1, Number(cd.move_time) || 30);
  const age = blocked ? 0 : Math.max(0, Number(cd.state_age_ms) || 0) / 1000;
  const transition = now - age * 1000;
  const fraction = old && old.duration > 0 ? Math.max(0, Math.min(1, (transition - old.start) / old.duration)) : 1;
  let from = old ? old.from + (old.to - old.from) * fraction : .5;
  if (view.state === 'opening') from = Math.min(.96, from);
  if (view.state === 'closing') from = Math.max(.04, from);
  let to = from, duration = 0;
  if (moving) {
    if (!old || view.state === 'unavailable') from = state === 'opening' ? 0 : 1;
    from = Math.max(0, Math.min(1, from + (state === 'opening' ? 1 : -1) * age / seconds));
    from = state === 'opening' ? Math.min(.96, from) : Math.max(.04, from);
    to = state === 'opening' ? 1 : 0;
    // Same full-travel speed in either direction, including after a stop.
    duration = Math.abs(to - from) * seconds * 1000;
  } else if (state === 'open' || state === 'closed') from = to = state === 'open' ? 1 : 0;
  view.signature = signature;
  view.state = state;
  view.motion = {from, to, duration, start: now};

}

// Fixed markup and numeric telemetry only; no icon assets or libraries.
function bluSensorStatus(node, sensor, timeout, kind = false) {
  const paired = sensor.paired ?? sensor.secure;
  let html = "Not paired";
  if (paired && !sensor.seen) html = `<span class="blu-badge blu-waiting">${sensor.protocol === "xiaomi" ? "Waiting for beacon" : "Waiting for encrypted beacon"}</span>`;
  if (paired && sensor.seen) {
    const battery = Number.isFinite(sensor.battery) && sensor.battery >= 0 ? Math.min(100, Math.round(sensor.battery)) : -1;
    const rssi = Number.isFinite(sensor.rssi) ? sensor.rssi : null;
    const bars = rssi === null ? 0 : rssi >= -60 ? 4 : rssi >= -70 ? 3 : rssi >= -80 ? 2 : 1;
    const age = Math.max(0, Math.floor(Number(sensor.age_s) || 0));
    const stale = age > timeout || sensor.ready === false;
    const state = kind === "button" ? (stale ? "Out of range" : "In range") : kind === "ht" ? bluHTValues(sensor) : kind ? (sensor.active ? "Motion" : "No motion") : sensor.active ? "Closed" : "Open";
    html = `<span class="blu-badge ${stale ? "blu-stale" : sensor.active ? "blu-closed" : "blu-open"}" title="${kind === 'ht' ? 'Temperature and humidity' : kind ? 'Motion sensor' : 'Sensor contact'}${stale ? ' — stale reading' : ''}">${state}${stale ? ' · stale' : ''}</span>` +
      `<span class="blu-power" title="Battery" aria-label="Battery ${battery < 0 ? 'unknown' : battery + '%'}"><span class="blu-battery" aria-hidden="true"><i style="width:${Math.max(0, battery)}%"></i></span>${battery < 0 ? '—' : battery + '%'}</span>` +
      `<span class="blu-signal" title="${rssi === null ? 'Signal unknown' : rssi + ' dBm'}" role="img" aria-label="Signal ${bars} of 4 bars">${[1,2,3,4].map(n => `<i class="${n <= bars ? 'filled' : ''}"></i>`).join('')}</span>` +
      `<span class="blu-age">${age}s ago</span>`;
  }
  if (node.bluHTML !== html) { node.innerHTML = html; node.bluHTML = html; }
}

function bluPairStatus(c, message, state) {
  const status = el(c, "blu_pair_status");
  updateInnerText(status, message);
  status.className = "blu-pair-status blu-pair-" + state;
}

function bluPairClearPINs(c) {
  c.bluPairCredentials = null;
  el(c, "blu_pair_pin").value = "";
  el(c, "blu_pair_current_pin").value = "";
}

function bluPairNeedsCurrentPIN(c) {
  const credentials = c.bluPairCredentials;
  return c.bluPairSelected && c.bluPairSelected.encrypted && credentials &&
    !/^[1-9][0-9]{5}$/.test(credentials.current);
}

function bluPairHasRecentReadings(c) {
  const blu = c.bluPairTelemetry || (c.data && c.data.blu);
  const role = ["closed", "open", "motion", "indoor", "outdoor", "button"][c.bluPairRole];
  const sensor = blu && blu[role];
  const selected = c.bluPairSelected && c.bluPairSelected.mac;
  return !!(sensor && selected && String(blu[role + "_mac"] || "").toUpperCase() === selected.toUpperCase() &&
    sensor.secure && sensor.seen && Number.isFinite(sensor.age_s) && sensor.age_s >= 0 &&
    sensor.age_s <= Math.min(15, blu.stale_after_s || 180));
}

function bluPairRender(c) {
  const state = c.bluPairState;
  const needsSetup = state === "waiting_encryption_pin";
  const scanning = state === "scanning";
  const needsCurrent = scanning && bluPairNeedsCurrentPIN(c);
  const choosing = scanning && c.bluPairCandidates.length > 1;
  const finished = ["done", "error", "idle"].includes(state);
  const show = (id, visible) => { el(c, id).style.display = visible ? "" : "none"; };
  show("blu_pair_new_container", needsSetup && !c.bluPairClosing);
  show("blu_pair_current_container", needsCurrent && !c.bluPairClosing);
  show("blu_pair_candidate_container", choosing && !c.bluPairClosing);
  show("blu_pair_selected", !!c.bluPairSelected);
  el(c, "blu_pair_pin").disabled = !needsSetup || !!c.bluPairAction;
  el(c, "blu_pair_current_pin").disabled = !needsCurrent || !!c.bluPairAction;
  if (needsCurrent) el(c, "blu_pair_current_container").open = true;
  updateInnerText(el(c, "blu_pair_current_summary"), needsCurrent ? "Current PIN required" : "Device already has a PIN?");
  updateInnerText(el(c, "blu_pair_selected"), c.bluPairSelected ? `Device detected: ${c.bluPairSelected.mac}` : "");
  const finding = ["starting", "scanning"].includes(state);
  const xiaomi = c.bluPairModel === 426;
  updateInnerText(el(c, "blu_pair_step"), finding ? "Step 1 of 2 · Find device" : state === "done" ? "Pairing complete" :
    finished ? "Pairing stopped" : "Step 2 of 2 · Set up device");
  let help = needsSetup ? "This device needs a six-digit PIN to enable encrypted readings. Enter a new PIN to finish setup." :
    needsCurrent ? "This device requires its existing PIN to connect. Enter it once to continue." :
    choosing ? "More than one matching device is nearby. Select the one you want to pair." :
    finding && xiaomi ? "Keep the Xiaomi LYWSDCGQ/01ZM powered on near Shelly. It broadcasts automatically; no pairing mode or PIN is needed. One detected device is added automatically. If several match, select yours." :
    finding ? "Hold the device button for more than 10 seconds, then release it. Keep it near Shelly. Pairing continues automatically when one matching device is detected." :
    state === "waiting_user_pairing" ? "Hold the same device's button for more than 10 seconds, then release it. Pairing resumes automatically when it is detected again." :
    state === "waiting_security_retry" ? "Keep the device near Shelly. The connection will resume automatically if the device is still in pairing mode." :
    state === "waiting_pairing_mode" ? "Keep the device near Shelly. Pairing will continue when it is detected again." :
    state === "waiting_encrypted_beacon" ? "Wake the device or trigger a reading to confirm that encrypted data is received." :
    state === "waiting_beacon" ? "Keep the thermometer near Shelly while temperature and humidity readings arrive. No button press or PIN is needed." :
    state === "done" ? "The device is ready to use. You can close this window." :
    state === "error" && bluPairHasRecentReadings(c) ? "Recent encrypted readings are available from this device using its saved key. This new pairing attempt did not finish. Close this window and check the device status before retrying." :
    finished ? "Pairing did not finish. Close this window before starting a new attempt." :
    "Keep the device near Shelly while pairing finishes.";
  if (c.bluPairClosing) help = "Stopping the pairing attempt…";
  updateInnerText(el(c, "blu_pair_help"), help);
  const labels = {
    starting: "Starting discovery…", scanning: needsCurrent ? "Enter the current device PIN to continue." : choosing ? "Choose a device to continue." : "Searching for your device…",
    connecting: "Connecting to device…", connecting_secure: "Connecting to device…",
    securing_link: "Securing the connection…", discovering_services: "Checking device…",
    enabling_beacon: "Configuring device…", setting_pin: "Configuring encryption…", setting_key: "Configuring encryption automatically…",
    verifying_existing_key: "Checking existing encrypted readings…", waiting_encryption_pin: "Encryption PIN required by this device.",
    waiting_key_ready: "Waiting for the encryption key…", reading_key: "Saving the encryption key…",
    reconnecting: "Restoring the connection…", reconnecting_secure: "Restoring the connection…",
    waiting_security_retry: "Device closed the connection. Waiting to reconnect…",
    waiting_user_pairing: "Hold the device button again for more than 10 seconds, then release it.", waiting_pairing_mode: "Waiting for the same device…",
    waiting_encrypted_beacon: "Waiting for encrypted beacon", waiting_beacon: "Waiting for temperature and humidity", done: "Device paired successfully.", idle: "Ready",
  };
  const message = c.bluPairClosing ? "Cancelling…" : c.bluPairValidationError || c.bluPairError || (state in labels ? labels[state] : state);
  const tone = c.bluPairValidationError || c.bluPairError || state === "error" ? "error" : state === "done" ? "success" :
    ["scanning", "waiting_encryption_pin", "waiting_user_pairing", "waiting_pairing_mode", "waiting_security_retry", "waiting_encrypted_beacon", "waiting_beacon"].includes(state) ? "waiting" : "progress";
  bluPairStatus(c, message, tone);
  show("blu_pair_status", !!message);
  const action = needsSetup ? "Enable encryption" : needsCurrent ? "Continue pairing" : choosing ? "Pair selected device" : "";
  show("blu_pair_confirm", !!action && !c.bluPairClosing);
  updateInnerText(el(c, "blu_pair_confirm"), action);
  el(c, "blu_pair_confirm").disabled = !!c.bluPairAction || (choosing && !c.bluPairSelected);
  updateInnerText(el(c, "blu_pair_cancel"), c.bluPairClosing ? "Cancelling…" : c.bluPairCancelFailed ? "Retry cancel" : finished ? "Close" : "Cancel");
  el(c, "blu_pair_cancel").disabled = !!c.bluPairClosing;
}

function bluPairStart(c, role) {
  if (!lastInfo || !lastInfo.auth_en) {
    alert("Set a web panel password in Security Settings before pairing devices.");
    return;
  }
  const dialog = el(c, "blu_pairing");
  if (dialog.open || c.bluPairClosing) return;
  c.bluPairSession = (c.bluPairSession || 0) + 1;
  c.bluPairRevision = 0;
  c.bluPairRole = role;
  c.bluPairModel = role === 3 || role === 4 ?
    (el(c, role === 3 ? "blu_indoor_type" : "blu_outdoor_type").value === "xiaomi" ? 426 : 3) : 0;
  c.bluPairState = "starting";
  c.bluPairCandidates = [];
  c.bluPairListSignature = "";
  c.bluPairSelected = null;
  c.bluPairAction = null;
  c.bluPairPending = c.bluPairClosing = c.bluPairFailed = c.bluPairNeedsCancel = c.bluPairCancelFailed = false;
  c.bluPairError = "";
  c.bluPairValidationError = "";
  bluPairClearPINs(c);
  c.bluPairCredentials = {current: ""};
  for (const id of ["blu_pair_pin", "blu_pair_current_pin"]) {
    el(c, id).oninput = () => { c.bluPairValidationError = ""; bluPairRender(c); };
  }
  el(c, "blu_pair_current_container").open = false;
  el(c, "blu_pair_candidate").replaceChildren();
  updateInnerText(el(c, "blu_pair_title"), ["Pair closing sensor", "Pair opening sensor", "Pair motion sensor", "Pair indoor H&T", "Pair outdoor H&T", "Pair BLU Button1"][role]);
  bluPairRender(c);
  dialog.showModal();
  bluPairRun(c, "Shelly.BLU.Discover", {model: c.bluPairModel}, () => { c.bluPairState = "scanning"; });
}

function bluPairRun(c, method, params, success) {
  if (c.bluPairAction || c.bluPairClosing) return;
  const session = c.bluPairSession;
  c.bluPairRevision++;
  c.bluPairPending = false;
  c.bluPairNeedsCancel = true;
  c.bluPairError = "";
  c.bluPairValidationError = "";
  const request = Promise.resolve().then(() => callDevice(method, params));
  c.bluPairAction = request;
  bluPairRender(c);
  request.then(() => {
    if (session === c.bluPairSession && !c.bluPairClosing) success();
  }).catch(e => {
    if (session !== c.bluPairSession || c.bluPairClosing) return;
    c.bluPairState = "error";
    c.bluPairFailed = true;
    c.bluPairError = e.message || String(e);
  }).finally(() => {
    if (session !== c.bluPairSession || c.bluPairClosing) return;
    c.bluPairAction = null;
    bluPairRender(c);
    bluPairRefresh(c);
  });
}

function bluPairSubmit(c) {
  if (c.bluPairState !== "scanning" || c.bluPairAction || c.bluPairClosing || !c.bluPairSelected ||
      !c.bluPairCredentials || bluPairNeedsCurrentPIN(c)) return;
  const params = {role: c.bluPairRole, mac: c.bluPairSelected.mac,
    current_pin: c.bluPairCredentials.current};
  c.bluPairState = "connecting";
  bluPairRun(c, "Shelly.BLU.Pair", params, () => { bluPairClearPINs(c); });
}

function bluPairRefresh(c) {
  if (["preparing", "done", "error", "idle"].includes(c.bluPairState) || c.bluPairAction ||
      c.bluPairClosing || c.bluPairFailed || c.bluPairPending || !el(c, "blu_pairing").open) return;
  const session = c.bluPairSession, revision = c.bluPairRevision;
  const current = () => session === c.bluPairSession && revision === c.bluPairRevision &&
    !c.bluPairClosing && el(c, "blu_pairing").open;
  c.bluPairPending = true;
  callDevice("Shelly.BLU.Status", {}).then(status => {
    if (!current()) return;
    // An accepted Pair request must never return to the credential form because
    // a delayed poll still reports scanning.
    if (status.state === "scanning" && !c.bluPairCredentials) return;
    c.bluPairState = status.state;
    c.bluPairError = status.error || "";
    if (["done", "error", "idle"].includes(status.state)) {
      c.bluPairNeedsCancel = false;
      c.bluPairValidationError = "";
      bluPairClearPINs(c);
    }
    if (status.state === "scanning") {
      const entries = (status.candidates || []).filter(candidate => !candidate.model ||
        (c.bluPairRole === 5 ? candidate.model === 1 : c.bluPairRole >= 3 ? candidate.model === c.bluPairModel : candidate.model === (c.bluPairRole === 2 ? 5 : 2)))
        .sort((a, b) => a.mac.localeCompare(b.mac));
      c.bluPairCandidates = entries;
      const selected = c.bluPairSelected && c.bluPairSelected.mac;
      c.bluPairSelected = entries.length === 1 ? entries[0] : entries.find(item => item.mac === selected) || null;
      const select = el(c, "blu_pair_candidate");
      const signature = entries.map(candidate => candidate.mac).join(",");
      // Polling RSSI must not rebuild an open sensor dropdown under the user.
      if (signature !== c.bluPairListSignature) {
        c.bluPairListSignature = signature;
        const placeholder = document.createElement("option");
        placeholder.value = ""; placeholder.textContent = "Choose a device";
        select.replaceChildren(placeholder, ...entries.map(candidate => {
          const option = document.createElement("option");
          option.value = candidate.mac;
          option.textContent = `${candidate.mac} · ${candidate.rssi} dBm`;
          return option;
        }));
      }
      const selectedValue = c.bluPairSelected ? c.bluPairSelected.mac : "";
      if (select.value !== selectedValue) select.value = selectedValue;
      select.onchange = () => {
        if (c.bluPairState !== "scanning" || c.bluPairAction || c.bluPairClosing) return;
        c.bluPairSelected = c.bluPairCandidates.find(item => item.mac === select.value) || null;
        c.bluPairError = "";
        c.bluPairValidationError = "";
        bluPairRender(c);
      };
      if (entries.length === 1 && !bluPairNeedsCurrentPIN(c)) {
        bluPairSubmit(c);
        return;
      }
    }
    bluPairRender(c);
  }).catch(e => {
    if (current()) { c.bluPairError = e.message || String(e); bluPairRender(c); }
  }).finally(() => { if (current()) c.bluPairPending = false; });
}

async function bluPairClose(c) {
  if (c.bluPairClosing) return;
  c.bluPairClosing = true;
  c.bluPairSession++;
  bluPairRender(c);
  // Finish an in-flight Discover/Pair before cancelling it. Otherwise a late
  // RPC could start pairing after the modal has already closed.
  if (c.bluPairAction) await c.bluPairAction.catch(() => {});
  c.bluPairAction = null;
  try {
    if (c.bluPairNeedsCancel) await callDevice("Shelly.BLU.Cancel", {});
    bluPairClearPINs(c);
    el(c, "blu_pairing").close();
    c.bluPairState = "idle";
    c.bluPairClosing = false;
    c.bluPairPending = false;
  } catch (e) {
    c.bluPairClosing = false;
    c.bluPairFailed = c.bluPairCancelFailed = true;
    c.bluPairState = "error";
    c.bluPairError = "Could not cancel pairing. " + (e.message || String(e));
    bluPairRender(c);
  }
}

function bluPairConfirm(c) {
  if (c.bluPairAction || c.bluPairClosing) return;
  const invalid = (message, id) => {
    c.bluPairValidationError = message;
    bluPairRender(c);
    el(c, id).focus();
  };
  if (c.bluPairState === "waiting_encryption_pin") {
    const pin = el(c, "blu_pair_pin").value;
    if (!/^[1-9][0-9]{5}$/.test(pin)) {
      invalid("Enter a six-digit encryption PIN from 100000 to 999999.", "blu_pair_pin"); return;
    }
    c.bluPairState = "setting_pin";
    bluPairRun(c, "Shelly.BLU.SetPIN", {pin}, () => { bluPairClearPINs(c); });
  } else if (c.bluPairState === "scanning") {
    if (bluPairNeedsCurrentPIN(c)) {
      const current = el(c, "blu_pair_current_pin").value;
      if (!/^[1-9][0-9]{5}$/.test(current)) {
        invalid("Enter the current six-digit device PIN.", "blu_pair_current_pin"); return;
      }
      c.bluPairCredentials.current = current;
    }
    bluPairSubmit(c);
  }
}

function gdoSetConfig(c, cfg, spinner) {
  if (!cfg) {
    let name = el(c, "name").value;
    if (name == "") {
      alert("Name must not be empty");
      return;
    }
    let moveTime = parseInt(el(c, "move_time").value);
    if (isNaN(moveTime) || moveTime < 10) {
      alert(`Invalid movement time ${moveTime}`);
      return;
    }
    let pulseTimeMs = parseInt(el(c, "pulse_time_ms").value);
    if (isNaN(pulseTimeMs) || pulseTimeMs < 20) {
      alert(`Invalid pulse time ${pulseTimeMs}`);
      return;
    }
    cfg = {
      name: name,
      move_time: moveTime,
      pulse_time_ms: pulseTimeMs,
      close_sensor_mode: parseInt(el(c, "close_sensor_mode").value),
      sensor_swap: el(c, "sensor_swap").checked,
      resume_after_stop: el(c, "resume_after_stop").checked,
    };
    if (c.data.open_sensor_mode >= 0) {
      cfg.open_sensor_mode = parseInt(el(c, "open_sensor_mode").value);
    }
    if (c.data.out_mode >= 0) {
      cfg.out_mode = parseInt(el(c, "out_mode").value);
    }
  }
  if (c.data.blu) {
    cfg.blu = {
      enable: el(c, "blu_enable").checked,
      closed_mac: el(c, "blu_closed_mac").value.trim().toUpperCase(),
      open_mac: el(c, "blu_open_mac").value.trim().toUpperCase(),
      stale_after_s: Number(el(c, "blu_stale_after_s").value),
    };
    for (const role of ["closed", "open", "motion", "indoor", "outdoor", "button"]) {
      if (!c.data.blu[role] || !c.data.blu[role].secure) continue;
      const name = el(c, "blu_" + role + "_name").value.trim();
      if (new TextEncoder().encode(name).length > 64) {
        alert("Device name is too long. Please use a shorter name.");
        return;
      }
      cfg.blu[role + "_name"] = name;
    }
    if (c.data.blu.button && c.data.blu.button.secure) {
      const carName = el(c, "blu_car_name").value.trim();
      if (new TextEncoder().encode(carName).length > 64) {
        alert("Car name is too long. Please use a shorter name.");
        return;
      }
      cfg.blu.car_name = carName;
    }
    const mac = /^(?:[0-9A-F]{2}:){5}[0-9A-F]{2}$/;
    if (cfg.blu.enable && (!mac.test(cfg.blu.closed_mac) || !mac.test(cfg.blu.open_mac) ||
        cfg.blu.closed_mac === cfg.blu.open_mac ||
        cfg.blu.closed_mac === "00:00:00:00:00:00" || cfg.blu.open_mac === "00:00:00:00:00:00")) {
      alert("Enter two distinct BLU sensor MAC addresses");
      return;
    }
    if (!Number.isInteger(cfg.blu.stale_after_s) || cfg.blu.stale_after_s < 30 || cfg.blu.stale_after_s > 3600) {
      alert("Sensor timeout must be 30-3600 seconds");
      return;
    }
  }
  setComponentConfig(c, cfg, spinner);
}

el("reboot_btn").onclick = function() {
  if (!confirm("Reboot the device?")) return;

  callDevice("Sys.Reboot", {delay_ms: 500}).then(function() {
    alert("System is rebooting and will reconnect when ready.");
  });
};

el("reset_btn").onclick = function() {
  if (!confirm(
          "Device configuration will be wiped and return to AP mode. " +
          "Are you sure?")) {
    return;
  }

  callDevice("Shelly.WipeDevice", {}).then(function() {
    alert("Device configuration has been reset, it will reboot in AP mode.");
  });
};

function findOrAddContainer(cd) {
  let elId = `c${cd.type}-${cd.id}`;
  let c = el(elId);
  if (c) return c;
  switch (cd.type) {
    case Component_Type.kSwitch:
    case Component_Type.kOutlet:
    case Component_Type.kLock:
      c = el("sw_template").cloneNode(true);
      c.id = elId;
      el(c, "state").onchange = function(ev) {
        setComponentState(c, {state: !c.data.state}, el(c, "set_spinner"));
        markInputChanged(ev);
      };
      if (cd.type == Component_Type.kSwitch ||
          cd.type == Component_Type.kOutlet) {
        el(c, "hk_state_inverted_container").style.display = "block";
      }
      el(c, "save_btn").onclick = function() {
        swSetConfig(c);
      };
      el(c, "auto_off").onchange = function(ev) {
        el(c, "auto_off_delay_container").style.display =
            this.checked ? "block" : "none";
        markInputChanged(ev);
      };
      break;
    case Component_Type.kStatelessSwitch:  // aka input in detached mode
    case Component_Type.kDoorbell:
      c = el("ssw_template").cloneNode(true);
      c.id = elId;
      el(c, "save_btn").onclick = function() {
        sswSetConfig(c);
      };
      break;
    case Component_Type.kWindowCovering:
      c = el("wc_template").cloneNode(true);
      c.id = elId;
      el(c, "open_btn").onclick = function() {
        setComponentState(c, {tgt_pos: 100}, el(c, "open_spinner"));
      };
      el(c, "close_btn").onclick = function() {
        setComponentState(c, {tgt_pos: 0}, el(c, "close_spinner"));
      };
      el(c, "save_btn").onclick = function() {
        wcSetConfig(c, null, el(c, "save_spinner"))
      };
      el(c, "cal_btn").onclick = function() {
        setComponentState(c, {state: 10}, null);
        el(c, "cal_spinner").className = "spin";
      };
      break;
    case Component_Type.kGarageDoorOpener:
      c = el("gdo_template").cloneNode(true);
      c.id = elId;
      el(c, "blu_pair_closed").onclick = () => bluPairStart(c, 0);
      el(c, "blu_pair_open").onclick = () => bluPairStart(c, 1);
      el(c, "blu_pair_motion").onclick = () => bluPairStart(c, 2);
      el(c, "blu_pair_indoor").onclick = () => bluPairStart(c, 3);
      el(c, "blu_pair_outdoor").onclick = () => bluPairStart(c, 4);
      el(c, "blu_pair_button").onclick = () => bluPairStart(c, 5);
      el(c, "blu_pair_confirm").onclick = () => bluPairConfirm(c);
      el(c, "blu_pair_cancel").onclick = () => bluPairClose(c);
      el(c, "blu_pairing").oncancel = event => {
        event.preventDefault();
        bluPairClose(c);
      };
      el(c, "save_btn").onclick = function() {
        gdoSetConfig(c, null, el(c, "save_spinner"));
      };
      el(c, "toggle_btn").onclick = function() {
        setComponentState(c, {toggle: true}, el(c, "toggle_spinner"));
      };
      break;
    case Component_Type.kDisabledInput:
      c = el("di_template").cloneNode(true);
      c.id = elId;
      el(c, "save_btn").onclick = function() {
        diSetConfig(c);
      };
      break;
    case Component_Type.kMotionSensor:
    case Component_Type.kOccupancySensor:
    case Component_Type.kContactSensor:
    case Component_Type.kLeakSensor:
    case Component_Type.kSmokeSensor:
    case Component_Type.kCarbonMonoxideSensor:
    case Component_Type.kCarbonDioxideSensor:
      c = el("sensor_template").cloneNode(true);
      c.id = elId;
      el(c, "save_btn").onclick = function() {
        mosSetConfig(c);
      };
      break;
    case Component_Type.kLightBulb:
      c = el("rgb_template").cloneNode(true);
      c.id = elId;

      let value = cd.bulb_type;
      let showct = (value == LightBulbController_BulbType.kCCT)
      let showcolor = (value == LightBulbController_BulbType.kRGBW)
      el(c, "hue_container").style.display = showcolor ? "block" : "none";
      el(c, "saturation_container").style.display =
          showcolor ? "block" : "none";
      el(c, "color_temperature_container").style.display =
          showct ? "block" : "none";
      el(c, "color_container").style.display =
          showct || showcolor ? "block" : "none";

      el(c, "state").onchange = function(ev) {
        setComponentState(c, rgbState(c, !c.data.state), el(c, "set_spinner"));
        markInputChanged(ev);
      };
      el(c, "save_btn").onclick = function() {
        rgbSetConfig(c);
      };
      el(c, "hue").onchange = el(c, "saturation").onchange =
          el(c, "color_temperature").onchange =
              el(c, "brightness").onchange = function(ev) {
                setComponentState(
                    c, rgbState(c, c.data.state), el(c, "toggle_spinner"));
                setPreviewColor(c, cd.bulb_type);
                markInputChanged(ev);
              };
      el(c, "auto_off").onchange = function(ev) {
        el(c, "auto_off_delay_container").style.display =
            this.checked ? "block" : "none";
        markInputChanged(ev);
      };
      break;
    case Component_Type.kTemperatureSensor:
      c = el("ts_template").cloneNode(true);
      c.id = elId;
      el(c, "save_btn").onclick = function() {
        tsSetConfig(c);
      };
      break;
    default:
      console.log(`Unhandled component type: ${cd.type}`);
  }
  if (c) {
    c.style.display = "block";
    el("components").appendChild(c);
  }
  return c;
}

function rgbState(c, newState) {
  return {
    state: newState, hue: el(c, "hue").value,
        saturation: el(c, "saturation").value,
        brightness: el(c, "brightness").value,
        color_temperature: el(c, "color_temperature").value
  }
}

function updateComponent(cd) {
  let c = findOrAddContainer(cd);
  let whatSensor;
  if (!c) return;
  switch (cd.type) {
    case Component_Type.kSwitch:
    case Component_Type.kOutlet:
    case Component_Type.kLock:
    case Component_Type.kLightBulb: {
      let headText = `Switch ${cd.id}`;
      if (cd.name) headText += ` (${cd.name})`;
      updateInnerText(el(c, "head"), headText);
      setValueIfNotModified(el(c, "name"), cd.name);
      el(c, "state").checked = cd.state;
      updatePowerStats(c, cd);
      if (cd.type == Component_Type.kSwitch ||
          cd.type == Component_Type.kOutlet) {
        el(c, "hk_state_inverted_container").style.display = "block";
        checkIfNotModified(el(c, "hk_state_inverted"), cd.hk_state_inverted);
      }
      if (cd.type == Component_Type.kLightBulb) {
        checkIfNotModified(el(c, "svc_hidden"), cd.svc_hidden);
        if (cd.hap_optional !== undefined && cd.hap_optional == 0) {
          el(c, "svc_hidden_container").style.display = "none";
        }
      }
      if (cd.svc_type !== undefined) {
        selectIfNotModified(el(c, "svc_type"), cd.svc_type);
        if (cd.svc_type == 3) {
          selectIfNotModified(el(c, "valve_type"), cd.valve_type);
          el(c, "valve_type_container").style.display = "block";
          updateInnerText(el(c, "valve_type_label"), "Valve Type:");
        } else {
          el(c, "valve_type_container").style.display = "none";
        }
      }
      selectIfNotModified(el(c, "initial"), cd.initial);
      if (cd.in_mode >= 0) {
        selectIfNotModified(el(c, "in_mode"), cd.in_mode);
        if (cd.in_mode != 3) {
          checkIfNotModified(el(c, "in_inverted"), cd.in_inverted);
          el(c, "in_inverted_container").style.display = "block";
        }
        if (!cd.hdim) {
          if (el(c, "in_mode_5")) el(c, "in_mode_5").remove();
          if (el(c, "in_mode_6")) el(c, "in_mode_6").remove();
        }
      } else {
        el(c, "in_mode_container").style.display = "none";
        el(c, "in_inverted_container").style.display = "none";
        if (el(c, "initial_3")) el(c, "initial_3").remove();
      }
      if (cd.out_inverted !== undefined) {
        checkIfNotModified(el(c, "out_inverted"), cd.out_inverted);
      }
      checkIfNotModified(el(c, "auto_off"), cd.auto_off);
      el(c, "auto_off_delay_container").style.display =
          el(c, "auto_off").checked ? "block" : "none";
      setValueIfNotModified(
          el(c, "auto_off_delay"), secondsToDateString(cd.auto_off_delay));
      if (cd.state_led_en !== undefined) {
        if (cd.state_led_en == -1) {
          el(c, "state_led_en_container").style.display = "none";
        } else {
          el(c, "state_led_en_container").style.display = "block";
          checkIfNotModified(el(c, "state_led_en"), cd.state_led_en == 1);
        }
      }

      if (cd.type == Component_Type.kLightBulb) {
        if (cd.bulb_type == LightBulbController_BulbType.kCCT) {
          headText = "CCT";
          if (lastInfo.model == "ShellyRGBW2" ||
              lastInfo.model == "ShellyPlusRGBWPM") {
            if (cd.id == 1) {
              headText += " R/G";
            } else {
              headText += " B/W";
            }
          }
        } else if (cd.bulb_type == LightBulbController_BulbType.kRGBW) {
          if (lastInfo.sys_mode == 4) {
            headText = "RGBW";
          } else {
            headText = "RGB";
          }
        } else {
          headText = "Light";
        }
        if (cd.name) headText += ` (${cd.name})`;
        updateInnerText(el(c, "head"), headText);
        setValueIfNotModified(el(c, "name"), cd.name);
        el(c, "state").checked = cd.state;
        updatePowerStats(c, cd);
        slideIfNotModified(el(c, "color_temperature"), cd.color_temperature);
        slideIfNotModified(el(c, "hue"), cd.hue);
        slideIfNotModified(el(c, "saturation"), cd.saturation);
        slideIfNotModified(el(c, "brightness"), cd.brightness);
        setValueIfNotModified(el(c, "transition_time"), cd.transition_time);
        setPreviewColor(c, cd.bulb_type);
      }
      break;
    }
    case Component_Type.kTemperatureSensor: {
      let headText = `Sensor ${cd.id}`;
      if (cd.name) headText += ` (${cd.name})`;
      setValueIfNotModified(el(c, "name"), cd.name);
      updateInnerText(el(c, "head"), headText);
      let v;
      if (cd.value !== undefined) {
        v = (cd.unit == 1 ? cel2far(cd.value) : cd.value);
        el(c, "unit").style.display = "inline";
      } else {
        v = cd.error;
        el(c, "unit").style.display = "none";
      }
      updateInnerText(el(c, "value"), v);
      selectIfNotModified(el(c, "unit"), cd.unit);
      el(c, "update_interval_container").style.display =
          (cd.unit == 2) ? "none" : "block";
      setValueIfNotModified(el(c, "update_interval"), cd.update_interval);
      setValueIfNotModified(el(c, "offset"), cd.offset / 100.0);
      break;
    }
    case Component_Type.kStatelessSwitch:
    case Component_Type.kDoorbell: {
      let headText = `Input ${cd.id}`;
      if (cd.name) headText += ` (${cd.name})`;
      updateInnerText(el(c, "head"), headText);
      setValueIfNotModified(el(c, "name"), cd.name);
      selectIfNotModified(el(c, "in_mode"), cd.in_mode);
      selectIfNotModified(el(c, "type"), cd.type);
      checkIfNotModified(el(c, "inverted"), cd.inverted);
      let lastEvText = "n/a";
      if (cd.last_ev_age > 0) {
        let lastEv = cd.last_ev;
        switch (cd.last_ev) {
          case 0:
            lastEv = "single";
            break;
          case 1:
            lastEv = "double";
            break;
          case 2:
            lastEv = "long";
            break;
          default:
            lastEv = cd.last_ev;
        }
        lastEvText = `${lastEv} (${secondsToDateString(cd.last_ev_age)} ago)`;
      }
      updateInnerText(el(c, "last_event"), lastEvText);
      break;
    }
    case Component_Type.kWindowCovering: {
      updateInnerText(el(c, "head"), cd.name);
      setValueIfNotModified(el(c, "name"), cd.name);
      selectIfNotModified(el(c, "display_type"), cd.display_type);
      updateInnerText(el(c, "state"), cd.state_str);
      selectIfNotModified(el(c, "in_mode"), cd.in_mode);
      checkIfNotModified(el(c, "swap_inputs"), cd.swap_inputs);
      checkIfNotModified(el(c, "swap_outputs"), cd.swap_outputs);
      let posText, calText;
      if (cd.cal_done == 1) {
        if (cd.cur_pos != cd.tgt_pos) {
          posText = `${cd.cur_pos} -> ${cd.tgt_pos}`;
        } else {
          posText = cd.cur_pos;
        }
        calText = `\
          movement time: ${cd.move_time_ms / 1000} s, \
          avg power: ${cd.move_power} W`;
        el(c, "pos_ctl").style.display = "block";
      } else {
        posText = "n/a";
        calText = "not calibrated";
        el(c, "pos_ctl").style.display = "none";
      }
      if (cd.state >= 10 && cd.state < 20) {  // Calibration is ongoing.
        calText = "in progress";
        el(c, "cal_spinner").className = "spin";
      } else if (!(cd.state >= 20 && cd.state <= 25)) {
        el(c, "cal_spinner").className = "";
        el(c, "open_spinner").className = "";
        el(c, "close_spinner").className = "";
      }
      updateInnerText(el(c, "pos"), posText);
      updateInnerText(el(c, "cal"), calText);
      break;
    }
    case Component_Type.kGarageDoorOpener: {
      garageRefresh(c, cd);
      garageDashboardRefresh(c, cd);
      if (cd.blu) {
        c.bluPairTelemetry = cd.blu;
        if (c.bluPairState === "error" && el(c, "blu_pairing").open) bluPairRender(c);
        bluPairRefresh(c);
        el(c, "blu_config").style.display = "";
        checkIfNotModified(el(c, "blu_enable"), cd.blu.enable);
        setValueIfNotModified(el(c, "blu_closed_mac"), cd.blu.closed_mac);
        setValueIfNotModified(el(c, "blu_open_mac"), cd.blu.open_mac);
        for (const role of ["motion", "indoor", "outdoor", "button"])
          setValueIfNotModified(el(c, "blu_" + role + "_mac"), cd.blu[role + "_mac"] || "");
        setValueIfNotModified(el(c, "blu_stale_after_s"), cd.blu.stale_after_s);
        updateInnerText(el(c, "blu_status"), !cd.blu.enable ? "Wired sensors selected" :
          (cd.blu.ready ? "BLU sensors ready" : "BLU sensors missing, stale or conflicting — controls blocked"));
        for (const role of ["closed", "open", "motion", "indoor", "outdoor", "button"]) {
          const sensor = cd.blu[role];
          el(c, "blu_" + role + "_name_row").style.display = sensor && (sensor.paired ?? sensor.secure) ? "" : "none";
          if (!sensor) continue;
          if (["indoor", "outdoor"].includes(role)) selectIfNotModified(el(c, "blu_" + role + "_type"), sensor.protocol === "xiaomi" ? "xiaomi" : "shelly");
          const name = role === "button" ? cd.blu.button_name ?? sensor.name ?? "" : sensor.name || "";
          setValueIfNotModified(el(c, "blu_" + role + "_name"), name);
          bluSensorStatus(el(c, "blu_" + role + "_status"), sensor, role === "button" ? cd.blu.presence_timeout_s || 45 : cd.blu.stale_after_s, role === "button" ? "button" : role === "motion" ? true : ["indoor", "outdoor"].includes(role) ? "ht" : false);
        }
        el(c, "blu_car_name_row").style.display = cd.blu.button && cd.blu.button.secure ? "" : "none";
        setValueIfNotModified(el(c, "blu_car_name"), cd.blu.car_name || "");
        el(c, "toggle_btn").disabled = cd.blu.enable && !cd.blu.ready;
      }
      updateInnerText(el(c, "head"), cd.name);
      setValueIfNotModified(el(c, "name"), cd.name);
      updateInnerText(el(c, "state"), cd.cur_state_str);
      selectIfNotModified(el(c, "close_sensor_mode"), cd.close_sensor_mode);
      setValueIfNotModified(el(c, "move_time"), cd.move_time);
      setValueIfNotModified(el(c, "pulse_time_ms"), cd.pulse_time_ms);
      checkIfNotModified(el(c, "sensor_swap"), cd.sensor_swap);
      checkIfNotModified(el(c, "resume_after_stop"), !!cd.resume_after_stop);
      if (cd.open_sensor_mode >= 0) {
        selectIfNotModified(el(c, "open_sensor_mode"), cd.open_sensor_mode);
      } else {
        el(c, "open_sensor_mode_container").style.display = "none";
      }
      if (cd.out_mode >= 0) {
        selectIfNotModified(el(c, "out_mode"), cd.out_mode);
      } else {
        el(c, "out_mode_container").style.display = "none";
      }
      if (cd.blu) {
        el(c, "sensor_swap_container").style.display = cd.blu.enable ? "none" : "";
        el(c, "close_sensor_mode_container").style.display = cd.blu.enable ? "none" : "";
        el(c, "open_sensor_mode_container").style.display = cd.blu.enable || cd.open_sensor_mode < 0 ? "none" : "";
      }
      break;
    }
    case Component_Type.kDisabledInput: {
      updateInnerText(el(c, "head"), `Input ${cd.id}`);
      selectIfNotModified(el(c, "type"), cd.type);
      break;
    }
    case Component_Type.kMotionSensor:
      whatSensor = whatSensor || "motion";
    case Component_Type.kOccupancySensor:
      whatSensor = whatSensor || "occupancy";
    case Component_Type.kContactSensor:
      whatSensor = whatSensor || "contact";
    case Component_Type.kLeakSensor:
      whatSensor = whatSensor || "leak";
    case Component_Type.kSmokeSensor:
      whatSensor = whatSensor || "smoke";
    case Component_Type.kCarbonMonoxideSensor:
      whatSensor = whatSensor || "carbon monoxide";
    case Component_Type.kCarbonDioxideSensor: {
      whatSensor = whatSensor || "carbon dioxide";
      let headText = `Input ${cd.id}`;
      if (cd.name) headText += ` (${cd.name})`;
      updateInnerText(el(c, "head"), headText);
      setValueIfNotModified(el(c, "name"), cd.name);
      selectIfNotModified(el(c, "type"), cd.type);
      checkIfNotModified(el(c, "inverted"), cd.inverted);
      selectIfNotModified(el(c, "in_mode"), cd.in_mode);
      setValueIfNotModified(el(c, "idle_time"), cd.idle_time);
      el(c, "idle_time_container").style.display =
          (cd.in_mode == 0 ? "none" : "block");
      let statusText =
          (cd.state ? `${whatSensor} detected` : `no ${whatSensor} detected`);
      if (cd.last_ev_age > 0) {
        statusText += `; last ${secondsToDateString(cd.last_ev_age)} ago`;
      }
      updateInnerText(el(c, "status"), statusText);
      break;
    }
    default: {
      console.log(`Unhandled component type: ${cd.type}`);
    }
  }
  c.data = cd;
  addInputChangeHandlers(c);
}

function updateStaticIPVisibility() {
  el("wifi_ip_container").style.display =
      (el("wifi_ip_en").checked ? "block" : "none");
  el("wifi1_ip_container").style.display =
      (el("wifi1_ip_en").checked ? "block" : "none");
}

function updateElement(key, value, info) {
  switch (key) {
    case "uptime":
      el("uptime_container").style.display = "block";
      updateInnerText(el("uptime"), durationStr(value));
      break;
    case "model":
      if (value.endsWith("RGBW2") || value.endsWith("RGBWPM")) {
        el("sys_mode_container").style.display = "block";
        if (value.endsWith("RGBW2") && el("sys_mode_0"))
          el("sys_mode_0").remove();
      } else {
        if (el("sys_mode_3")) el("sys_mode_3").remove();
        if (el("sys_mode_4")) el("sys_mode_4").remove();
        if (el("sys_mode_5")) el("sys_mode_5").remove();
        if (el("sys_mode_6")) el("sys_mode_6").remove();
        if (el("sys_mode_7")) el("sys_mode_7").remove();
      }
      updateInnerText(el(key), value);
      break;
    case "version":
    case "fw_build":
      if (value !== undefined && (value >= 0 && value < 100)) break;
    // fallthrough;
    case "device_id":
      updateInnerText(el(key), value);
      break;
    case "name":
      document.title = value;
      updateInnerText(el("device_name"), value);
      setValueIfNotModified(el("sys_name"), value);
      break;
    case "wifi_en":
    case "wifi1_en":
    case "wifi_ap_en":
      checkIfNotModified(el(key), value);
      break;
    case "wifi_ssid":
    case "wifi1_ssid":
    case "wifi_ap_ssid":
    case "wifi_pass":
    case "wifi1_pass":
    case "wifi_ap_pass":
    case "wifi_netmask":
    case "wifi1_netmask":
    case "wifi_gw":
    case "wifi1_gw":
    case "wifi_nameserver":
    case "wifi1_nameserver":
      setValueIfNotModified(el(key), value);
      break;
    case "wifi_ip":
    case "wifi1_ip":
      setValueIfNotModified(el(key), value);
      checkIfNotModified(el(`${key}_en`), (value != ""));
      updateStaticIPVisibility();
      break;
    case "host":
    case "wifi_conn_ip":
    case "wifi_conn_rssi":
    case "wifi_conn_ssid":
    case "wifi_status":
    case "mac_address":
      updateInnerText(el(key), value);
      el(`${key}_container`).style.display = (value ? "block" : "none");
      if (key == "wifi_conn_rssi" && value != 0) {
        // These only make sense if we are connected to WiFi.
        el("update_container").style.display = "block";
        el("revert_to_stock_container").style.display =
            (!updateInProgress ? "block" : "none");
        // We set external image URL to prevent loading it when not on
        // WiFi, as it slows things down.
        // if (el("donate_form_submit").src == "") {
        //  el("donate_form_submit").src =
        //      "https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif";
        //}
        // el("donate_form_submit").style.display = "inline";
        updateInnerText(el("wifi_ip"), value);
        el("wifi_container").style.display =
            (!updateInProgress ? "block" : "none");
      }
      break;
    case "wifi_connecting":
      el("wifi_spinner").className = (value ? "spin" : "");
      break;
    case "wifi_sta_ps_mode":
      selectIfNotModified(el("wifi_sta_ps_mode"), value);
      break;
    case "hap_paired":
      if (value) {
        updateInnerText(el(key), "yes");
        el("hap_setup_btn").style.display = "none";
        el("hap_reset_btn").style.display = "";
      } else {
        updateInnerText(el(key), "no");
        el("hap_setup_btn").style.display = "";
        el("hap_reset_btn").style.display = "none";
      }
      break;
    case "hap_cn":
      if (value !== el("components").cn) {
        el("components").innerHTML = "";
      }
      el("components").cn = value;
      break;
    case "components":
      if (!updateInProgress) {
        // The number of components has changed, delete them all and
        // start afresh
        if (lastInfo !== null && lastInfo.components.length !== value.length) {
          el("components").innerHTML = "";
        }
        for (let i in value) updateComponent(value[i]);
      } else {
        // Update is in progress, hide all components.
        el("components").innerHTML = "";
      }
      break;
    case "hap_running":
      if (!value) {
        updateInnerText(el("hap_ip_conns_max"), "server not running");
        el("hap_ip_conns_pending").style.display = "none";
        el("hap_ip_conns_active").style.display = "none";
      }
      break;
    case "hap_ip_conns_pending":
    case "hap_ip_conns_active":
    case "hap_ip_conns_max":
      if (info.hap_running) {
        el(key).style.display = "inline";
        updateInnerText(el(key), `${value} ${key.split("_").slice(-1)[0]}`);
      }
      break;
    case "wc_avail":
      if (value) {
        el("sys_mode_container").style.display = "block";
      } else if (el("sys_mode_1")) {
        el("sys_mode_1").remove();
      }
      break;
    case "gdo_avail":
      if (value) {
        el("sys_mode_container").style.display = "block";
      } else if (el("sys_mode_2")) {
        el("sys_mode_2").remove();
      }
      break;
    case "sys_mode":
      selectIfNotModified(el("sys_mode"), value);
      break;
    case "sys_temp":
      if (value !== undefined) {
        updateInnerText(el("sys_temp"), value);
        el("sys_temp_container").style.display = "block";
      } else {
        el("sys_temp_container").style.display = "none";
      }
      break;
    case "overheat_on":
      el("notify_overheat").style.display = (value ? "inline" : "none");
      break;
    case "ota_progress":
      if (value !== undefined && (value >= 0 && value <= 100)) {
        updateInnerText(
            el("version"), `${info.version} -> ${info.ota_version}`);
        updateInnerText(el("fw_build"), info.ota_build);
        updateInnerText(el("update_status"), value === 100 ? "Update complete; restarting..." : `${value}%`);
        setTimeout(() => setUpdateInProgress(true), 0);
      }
      break;
  }
}

function updatePowerStats(c, cd) {
  if (cd.apower === undefined) return;

  apower = Math.round(cd.apower * 10) / 10;
  console.log(apower)
  updateInnerText(el(c, "power_stats"), `${apower}W, ${cd.aenergy}Wh`);
  el(c, "power_stats_container").style.display = "block";
}

function getInfo() {
  return new Promise(function(resolve, reject) {
    if (pendingGetInfo) {
      reject(new Error("already connecting"));
      return;
    }
    pendingGetInfo = true;
    const accessEpoch = garageDashboardAccessEpoch;
    let method = (infoLevel == 1 ? "Shelly.GetInfoExt" : "Shelly.GetInfo");
    callDevice(method)
        .then(function(info) {
          pendingGetInfo = false;

          if (accessEpoch !== garageDashboardAccessEpoch) {
            reject(new Error("Panel session changed"));
            return;
          }

          if (!info) {
            reject();
            return;
          }

          // Update the essentials.
          ["name", "model", "device_id", "version", "fw_build"].forEach(
              (key) => {
                updateElement(key, info[key], info);
              });
          if (info.failsafe_mode) {
            el("sys_container").style.display = "block";
            el("firmware_container").style.display = "block";
            el("notify_failsafe").style.display = "inline";
            pauseAutoRefresh = true;
            reject();
            return;
          }

          if (method === "Shelly.GetInfo") {
            infoLevel = 1;
            // Get extended info.
            getInfo().then(resolve, reject);
            return;
          }

          lastInfo = info;
          // GetInfoExt is protected by the device ACL when authentication is enabled.
          garageDashboardSetAccess(true);

          el("sec_old_pass_container").style.display =
              (info.auth_en ? "block" : "none");
          el("firmware_container").style.display = "block";
          updateCommonVisibility(!updateInProgress);

          // the system mode changed, clear out old UI components
          if (lastInfo !== null && lastInfo.sys_mode !== info.sys_mode) {
            el("components").innerHTML = "";
          }

          for (let element in info) {
            updateElement(element, info[element], info);
          }

          resolve(info);
        })
        .catch(function(err) {
          console.log(err);
          infoLevel = 0;
          reject(err);
        })
        .finally(() => pendingGetInfo = false);
  });
}

function getVar(key) {
  let vs = window.localStorage.getItem(key);
  if (!vs) return undefined;
  let v = JSON.parse(vs);
  if (v.exp !== undefined && (new Date()).getTime() > v.exp) {
    console.log("Expired", key);
    localStorage.removeItem(key)
    return undefined;
  }
  return v.value;
}

function setVar(key, value, maxAge) {
  if (value === undefined) {
    console.log("Delete var", key);
    localStorage.removeItem(key);
    return;
  }
  let v = {value: value};
  if (maxAge > 0) {
    v.exp = (new Date()).getTime() + (maxAge * 1000);
  }
  console.log("SetVar", key, v, maxAge);
  window.localStorage.setItem(key, JSON.stringify(v));
  // Clear out cookie storage. Added: 2021/04/24.
  document.cookie = `${key}=;max-age=1`;
}

function setupHost() {
  host = (new URLSearchParams(location.search)).get("host") || location.host;

  if (!host) {
    host = prompt("Please enter the host of your shelly.");
    if (host !== null) {
      location.href = `${location.host}?host=${host}`;
    }
  }

  el("debug_link").href = `http://${host}/debug/log?follow=1`;
}

function reloadPage() {
  // If path or query string were set (e.g. '/ota'), reset them.
  let newHREF = `http://${location.host}/`;
  if (location.href != newHREF && !location.href.startsWith("file://")) {
    location.replace(newHREF);
  } else {
    location.reload();
  }
}

function connectWebSocket() {
  setupHost();

  return new Promise(function(resolve, reject) {
    let url = `ws://${host}/rpc`;
    console.log(`Connecting to ${url}...`);
    socket = new WebSocket(url);

    socket.onclose = function(event) {
      garageDashboardSetAccess(false);
      let error = `[close] Connection died (code ${event.code})`;
      if (isConnected) {
        console.log(error);
      }
      el("notify_disconnected").style.display = "inline";
      let pr = pendingRequests;
      pendingRequests = {};
      for (let id in pr) {
        pr[id].reject(error);
      }
      reject(error);
      socket = null;
    };

    socket.onerror = function(error) {
      console.log(`[error] Connection error`, error);
      socket.close();
    };

    socket.onopen = function() {
      console.log("[open] Connection established");
      el("notify_disconnected").style.display = "none";
      isConnected = true;
      resolve(socket);
    };

    socket.onmessage = function(event) {
      let resp = JSON.parse(event.data);
      let id = resp.id;
      let ri = pendingRequests[id];
      if (!ri) return;
      delete pendingRequests[id];
      console.log("[<-]", resp);
      if (resp.error && resp.error.code == 401) {
        rpcAuth = null;
        let authReq = JSON.parse(resp.error.message);
        let ar = null;
        if (!ri.ar) {
          authRealm = authReq.realm;
          ar = getAuthResp(authReq);
          console.log(`Auth required for ${ri.method}`, authReq, ar);
        } else {
          console.log(`Auth failed for ${ri.method}`, authReq);
          el("forgot_password").style.display = "block";
          setVar(authInfoKey, undefined);
        }
        if (ar) {
          console.log("Retrying with auth...");
          callDeviceAuth(ri.method, ri.params, ar)
              .then((resp) => ri.resolve(resp))
              .catch((err) => ri.reject(err))
              .finally(() => el("auth_log_in_spinner").className = "");
        } else {
          garageDashboardSetAccess(false);
          if (lastInfo !== null) {
            // Locked out, reload UI.
            reloadPage();
          } else {
            pauseAutoRefresh = true;
            el("auth_container").style.display = "block";
            el("auth_pass").focus();
            ri.reject(new Error("Please log in"));
          }
        }
      } else {
        if (ri.ar) {
          el("auth_container").style.display = "none";
          pauseAutoRefresh = false;
          setVar(authInfoKey, ri.ar.ai, maxAuthAge);
          rpcAuth = ri.ar.rpcAuth;
        }
        if (resp.error) {
          ri.reject(resp.error);
        } else {
          ri.resolve(resp.result);
        }
      }
    };
  });
}

// Implementation of the digest auth algorithm with SHA-256 (RFC 7616).
// We have to do it manually because browsers still don't support it
// natively.
function calcHA1(user, realm, pass) {
  return sha256(`${user}:${realm}:${pass}`);
}

function getAuthResp(req) {
  const method = req.method || "dummy_method";
  const uri = req.uri || "dummy_uri";
  const cnonce = "" + Math.ceil(Math.random() * 100000000);
  const qop = "auth";
  let ha1 = "";
  if (el("auth_pass").value !== "") {
    ha1 = calcHA1(authUser, req.realm, el("auth_pass").value);
    el("auth_pass").value = "";
  } else {
    let ai = getVar(authInfoKey);
    if (ai && ai.realm == req.realm) {
      ha1 = ai.ha1;
    }
  }
  if (!ha1) return null;
  if (req.algorithm != "SHA-256") return null;
  const ha2 = sha256(`${method}:${uri}`);
  const resp = sha256(`${ha1}:${req.nonce}:${req.nc}:${cnonce}:${qop}:${ha2}`);
  let authResp = {
    rpcAuth: {
      realm: req.realm,
      nonce: req.nonce,
      username: authUser,
      cnonce: cnonce,
      algorithm: req.algorithm,
      response: resp,
    },
    httpAuth: (
        `Digest realm="${req.realm}", uri="${uri}", username="${authUser}", ` +
        `cnonce="${cnonce}", qop=${qop}, nc=${req.nc}, nonce="${req.nonce}", ` +
        `response="${resp}", algorithm=${req.algorithm}`),
    ai: {
      realm: req.realm,
      ha1: ha1,
    },
  };
  if (req.opaque) {
    authResp.rpcAuth.opaque = req.opaque;
    authResp.http_auth += `, opaque="${req.opaque}"`;
  };
  return authResp;
}

function authHeaderToReq(method, uri, hdr, nc) {
  let authReq = {
    method: method,
    uri: uri,
    nc: nc,
    realm: /realm="([^"]+)"/.exec(hdr)[1],
    nonce: /nonce="([^"]+)"/.exec(hdr)[1],
    algorithm: /algorithm=([A-Za-z0-9-]+)/.exec(hdr)[1],
  };
  let opq = /opaque="([^"]+)"/.exec(hdr);
  if (opq !== null) {
    authReq.opaque = opq[1];
  }
  return authReq;
}

function callDeviceAuth(method, params, ar) {
  let id = nextRequestID++;
  return new Promise(function(resolve, reject) {
    try {
      let frame = {
        "id": id,
        "method": method,
      };
      if (params) {
        frame.params = params;
      }
      if (ar) {
        frame.auth = ar.rpcAuth;
      } else if (rpcAuth) {
        frame.auth = rpcAuth;
      }
      console.log("[->]", frame);
      socket.send(JSON.stringify(frame));
      let pr = {
        id: id,
        method: method,
        params: params,
        ar: ar,
        resolve: resolve,
        reject: reject,
      };
      pendingRequests[id] = pr;
      setTimeout(() => {
        if (!pendingRequests[id]) return;
        delete pendingRequests[id];
        pr.reject(new Error("Request timeout"));
        if (socket) {
          socket.close();
        }
      }, rpcRequestTimeoutMs);
    } catch (e) {
      reject(e);
    }
  });
}

function callDevice(method, params) {
  return callDeviceAuth(method, params, null);
}

function doLogin() {
  el("auth_log_in_spinner").className = "spin";
  getInfo();
}

el("auth_log_in_btn").onclick = function() {
  doLogin();
  return true;
};

el("auth_pass").onkeyup = function(e) {
  console.log(e);
  if (e.code == "Enter") doLogin();
  return false;
};

el("sec_log_out_btn").onclick = function() {
  garageDashboardSetAccess(false);
  rpcAuth = null;
  pauseAutoRefresh = true;
  setVar(authInfoKey, undefined);
  reloadPage();
  return true;
};

el("sec_new_pass").onkeyup = el("sec_conf_pass").onkeyup = function(e) {
  el("sec_save_btn").disabled =
      el("sec_new_pass").value !== el("sec_conf_pass").value;
};

el("sec_save_btn").onclick = function() {
  if (authRealm !== null) {
    let oldHA1 = calcHA1(authUser, authRealm, el("sec_old_pass").value);
    let goodHA1 = getVar(authInfoKey).ha1;
    if (oldHA1 !== goodHA1) {
      alert("Invalid old password!");
      setVar(authInfoKey, undefined);
      rpcAuth = null;
      garageDashboardSetAccess(false);
      return;
    }
  }
  let newHA1 = "";
  let realm = lastInfo.device_id;
  if (el("sec_new_pass").value !== "") {
    newHA1 = calcHA1(authUser, realm, el("sec_new_pass").value);
    if (!newHA1) {
      alert("No unicode in passwords please");
      return true;
    }
  }
  pauseAutoRefresh = true;
  el("sec_save_spinner").className = "spin";
  callDevice("Shelly.SetAuth", {user: authUser, realm: realm, ha1: newHA1})
      .then(function() {
        garageDashboardSetAccess(false);
        setVar(authInfoKey, undefined);
        reloadPage();
      })
      .catch(function(err) {
        if (err.message) err = err.message;
        alert(err);
      })
      .finally(function() {
        el("sec_save_spinner").className = "";
        pauseAutoRefresh = false;
      });
  return true;
};

// noinspection JSUnusedGlobalSymbols
function onLoad() {
  if (location.protocol != "file:") {
    if (location.pathname === "/ota") {
      let params = new URLSearchParams(location.search.substring(1));
      return downloadUpdate(
          params.get("url"), el("fw_spinner"), el("update_status"));
    } else if (location.pathname !== "/") {
      reloadPage();
    }
  }
  setInterval(refreshUI, uiRefreshInterval * 1000);
  el("wifi_ip_en").onchange = el("wifi1_ip_en").onchange = function(ev) {
    updateStaticIPVisibility();
    markInputChanged(ev);
  };
  addInputChangeHandlers(document);
  refreshUI();
}

let connectStarted = 0;

function refreshUI() {
  // if the socket is open and connected and the page is visible to the user
  if (document.hidden) return;
  if (!socket) {
    connectStarted = (new Date()).getTime();
    connectWebSocket().then(() => refreshUI()).catch(() => {});
    return;
  }
  if (socket.readyState !== 1) {
    el("notify_disconnected").style.display = "inline";
    let now = (new Date()).getTime();
    if (now - connectStarted >= 3000) {
      console.log("Connection timed out");
      socket.close();
      socket = null;
    }
    return;
  }
  if (pauseAutoRefresh) return;
  getInfo()
      .then(function(info) {
        if (lastFwBuild && info.fw_build != lastFwBuild) {
          // Firmware changed, reload.
          reloadPage();
          return;
        } else {
          lastFwBuild = info.fw_build;
        }
        checkUpdateIfNeeded(info);
      })
      .catch((err) => {});
}

function setValueIfNotModified(e, newValue) {
  newValue = newValue.toString();
  // do not update the value of the input field if the field currently has
  // focus or has changed since changes have been last saved.
  if (document.activeElement === e || e.dataset.changed == "true" ||
      e.value === newValue) {
    return;
  }
  e.value = newValue;
}

function checkIfNotModified(e, newState) {
  newState = Boolean(newState);
  // do not update the checked value if
  if (newState == e.checked || e.dataset.changed === "true") return;
  e.checked = newState;
}

function slideIfNotModified(e, newValue) {
  newValue = newValue.toString();
  if (newValue === e.value || e.dataset.changed === "true") return;
  e.value = newValue;
}

function selectIfNotModified(e, newSelection) {
  setValueIfNotModified(e, newSelection);
}

function markInputChanged(ev) {
  console.log("CHANGED", ev.target);
  ev.target.dataset.changed = "true";
}

function addOnChangeHandlers(els) {
  for (let i = 0; i < els.length; i++) {
    let el = els[i];
    if (el.onchange) continue;
    el.dataset.changed = "false";
    el.onchange = markInputChanged;
  }
}

function addInputChangeHandlers(el) {
  addOnChangeHandlers(el.getElementsByTagName("input"));
  addOnChangeHandlers(el.getElementsByTagName("select"));
}

function resetLastSetValue() {
  let inputs = document.getElementsByTagName("input");
  for (let i = 0; i < inputs.length; i++) {
    inputs[i].dataset.changed = "false";
  }
}

function updateInnerText(e, newInnerText) {
  newInnerText = newInnerText.toString();
  if (e.innerText === newInnerText) return;
  e.innerText = newInnerText;
}

function updateCommonVisibility(visible) {
  let d = (visible ? "block" : "none");
  el("gs_container").style.display = d;
  el("homekit_container").style.display = d;
  el("wifi_container").style.display = d;
  el("sec_container").style.display = d;
  el("sys_container").style.display = d;
}

function setUpdateInProgress(val) {
  updateInProgress = !!val;
  garageDashboardSyncVisibility();
  if (val) {
    el("components").innerHTML = "";
    el("update_btn").style.display = "none";
    el("revert_to_stock_container").style.display = "none";
    updateCommonVisibility(false);
  }
}

function durationStr(d) {
  let days = parseInt(d / 86400);
  d %= 86400;
  let hours = parseInt(d / 3600);
  d %= 3600;
  let mins = parseInt(d / 60);
  let secs = d % 60;
  return days + ":" + nDigitString(hours, 2) + ":" + nDigitString(mins, 2) +
      ":" + nDigitString(secs, 2);
}

async function downloadUpdate(fwURL, spinner, status) {
  setUpdateInProgress(true);
  spinner.className = "spin";
  status.innerText = "Downloading...";
  console.log("Downloading", fwURL);
  return fetch(fwURL, {mode: "cors"})
      .then(async (resp) => {
        console.log(resp);
        let blob = await resp.blob();
        if (!resp.ok || blob.type != "application/zip") {
          throw new Error("Invalid firmware download; try the ZIP file manually");
        }
        return uploadFW(blob, spinner, status);
      })
      .catch((error) => {
        spinner.className = "";
        console.log(error);
        status.innerText = `Error downloading: ${error}`;
        // Do not reset updateInProgress to make failure more prominent.
      });
}

async function uploadFW(blob, spinner, status, ar) {
  setUpdateInProgress(true);
  spinner.className = "spin";
  status.innerText = "Authenticating update...";
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 120000);
  try {
    // Get a Digest challenge with no body. Sending the ZIP before a 401 can
    // stall the browser's second large upload on the embedded HTTP server.
    if (!ar) {
      const probe = await fetch("/update", {cache: "no-store", signal: controller.signal});
      if (probe.status === 401) {
        const challenge = probe.headers.get("www-authenticate");
        if (!challenge) throw new Error("Missing update authentication challenge");
        ar = getAuthResp(authHeaderToReq("POST", "/update", challenge, "00000001"));
      } else if (!probe.ok && probe.status !== 404 && probe.status !== 405) {
        throw new Error(`Update authentication failed (HTTP ${probe.status})`);
      }
    }
    const fd = new FormData();
    fd.append("file", blob);
    const headers = new Headers();
    if (ar) headers.append("Authorization", ar.httpAuth);
    status.innerText = "Uploading...";
    const resp = await fetch("/update", {
      method: "POST", headers, body: fd, cache: "no-cache", signal: controller.signal,
    });
    const text = (await resp.text()).trim();
    if (!resp.ok) throw new Error(text || `HTTP ${resp.status}`);
    status.innerText = text || "Firmware uploaded; waiting for restart...";
    setVar("update_available", false);
  } catch (error) {
    status.innerText = error.name === "AbortError" ?
      "Update request timed out. Check the device version and update progress before retrying." :
      `Error uploading: ${error.message || error}`;
    // A timeout or disconnect can happen after the device began writing.
    // Keep controls blocked; polling still detects progress and a new build.
  } finally {
    clearTimeout(timeout);
    spinner.className = "";
  }
}

// major.minor.patch-variantN
function parseVersion(versionString) {
  version =
      versionString
          .match(
              /^(?<major>\d+).(?<minor>\d+).(?<patch>\d+)-?(?<variant>[a-z]*)(?<varSeq>\d*)$/)
          .groups
  version.major = parseInt(version.major);
  version.minor = parseInt(version.minor);
  version.patch = parseInt(version.patch);
  version.varSeq = parseInt(version.varSeq) || 0;

  return version;
}

function isNewer(v1, v2) {
  let vi1 = parseVersion(v1), vi2 = parseVersion(v2);
  if (vi1.major != vi2.major) return (vi1.major > vi2.major);
  if (vi1.minor != vi2.minor) return (vi1.minor > vi2.minor);
  if (vi1.patch != vi2.patch) return (vi1.patch > vi2.patch);
  if (vi1.variant != vi2.variant) return true;
  if (vi1.varSeq != vi2.varSeq) return (vi1.varSeq > vi2.varSeq);
  return false;
}

function checkUpdateIfNeeded(info) {
  // If device is in AP mode, we most likely don't have internet connectivity
  // anyway.
  if (info.wifi_conn_rssi == 0) return;
  let last_update_check = parseInt(getVar("last_update_check"));
  let now = new Date();
  let age = undefined;
  if (!isNaN(last_update_check)) {
    age = (now.getTime() - last_update_check) / 1000;
  }
  if (isNaN(last_update_check) || age > updateCheckInterval) {
    console.log(`Last update check: ${last_update_check} age ${
        age}, checking for update`);
    checkUpdate();
  }
  el("notify_update").style.display =
      (getVar("update_available") ? "inline" : "none");
}

function checkUpdate() {
  let model = lastInfo.model;
  let curVersion = lastInfo.version;
  let e = el("update_status");
  let se = el("update_btn_spinner");
  let errMsg =
      "Failed, check <a href=\"https://hejsiri.pl/firmware/garage-blu/\">firmware downloads</a>.";
  e.innerText = "";
  se.className = "spin";
  console.log("Model:", model, "Version:", curVersion);
  fetch(["Shelly1Gen3", "ShellyPlus1"].includes(model) ?
            "https://hejsiri.pl/firmware/garage-blu/update.json" :
            "https://rojer.me/files/shelly/update.json", {
    headers: {
      "X-Model": model,
      "X-Current-Version": curVersion,
      "X-Current-Build": lastInfo.fw_build,
      "X-Device-ID": lastInfo.device_id,
    }
  })
      .then(resp => resp.json())
      .then((resp) => {
        // save the cookie before anything else, so that if update not
        // found we still remember that we tried to check for an update
        setVar("last_update_check", (new Date()).getTime());

        let cfg, latestVersion, updateURL, relNotesURL;
        for (let i in resp) {
          let re = new RegExp(resp[i][0]);
          if (curVersion.match(re)) {
            cfg = resp[i][1];
            break;
          }
        }
        if (cfg) {
          latestVersion = cfg.version;
          relNotesURL = cfg.rel_notes;
          if (cfg.urls) updateURL = cfg.urls[model];
        }
        console.log("Version:", latestVersion, "URL:", updateURL);
        if (!latestVersion || !updateURL) {
          console.log("Update section not found:", model, curVersion, cfg);
          e.innerHTML = errMsg;
          se.className = "";
          return;
        }
        let updateAvailable = isNewer(latestVersion, curVersion);
        el("notify_update").style.display =
            (updateAvailable ? "inline" : "none");

        setVar("update_available", updateAvailable);
        if (!updateAvailable) {
          e.innerText = "Up to date";
          se.className = "";
          return;
        }
        se.className = "";
        e.innerHTML = `
        Version ${latestVersion} is available.
        See <a href="${relNotesURL}" target="_blank">release notes</a>.`
        el("update_btn_text").innerText = "Install";
        el("update_btn").onclick = function() {
          return downloadUpdate(
              updateURL, el("fw_spinner"), el("update_status"));
        };
      })
      .catch((error) => {
        console.log("Error", error);
        e.innerHTML = errMsg;
        se.className = "";
      });
}

el("update_btn").onclick = function() {
  checkUpdate();
};

el("revert_btn").onclick = function() {
  if (!confirm("Revert to stock firmware?")) return;

  el("revert_msg").style.display = "block";
  let stockURL = `https://rojer.me/files/shelly/stock/latest-nnn/${
      lastInfo.stock_fw_model}.zip`;
  downloadUpdate(stockURL, el("fw_spinner"), el("revert_status"));
};

function setPreviewColor(c, bulb_type) {
  let h = el(c, "hue").value / 360;
  let s = el(c, "saturation").value / 100;
  let t = el(c, "color_temperature").value;
  let r, g, b;

  // use fixed 100% for v, because we want to control brightness over pwm
  // frequency
  if (bulb_type == LightBulbController_BulbType.kCCT) {
    [r, g, b] = colortemp2rgb(t, 100);
  } else {
    [r, g, b] = hsv2rgb(h, s, 100);
  }

  r = Math.round(r * 2.55);
  g = Math.round(g * 2.55);
  b = Math.round(b * 2.55);

  rgbHex = [r, g, b]
               .map(x => nDigitString(x.toString(16), 2))
               .join("")
               .toUpperCase();

  el(c, "color_preview").style.backgroundColor = `rgb(${r}, ${g}, ${b})`;
  el(c, "color_name").innerHTML = `#${rgbHex}`;
  el(c, "hue_value").innerHTML = `${el(c, "hue").value}&#176;`;
  el(c, "saturation_value").innerHTML = `${el(c, "saturation").value}%`;
  el(c, "brightness_value").innerHTML = `${el(c, "brightness").value}%`;
  el(c, "color_temperature_value").innerHTML =
      `${el(c, "color_temperature").value}mired`;
}

function clamprgb(val) {
  let min = 0;
  let max = 255;
  return Math.max(min, Math.min(val, max))
}

function colortemp2rgb(t, v) {
  // Formula by Tanner Helland
  var temperature = 1000000.0 / t / 100.0;
  var scale = 1 / 2.55;

  return [
    (temperature <= 66 ?
         255 :
         clamprgb(
             329.698727446 * Math.pow(temperature - 60.0, -0.1332047592))) *
        scale,
    (temperature <= 66 ?
         clamprgb(99.4708025861 * Math.log(temperature) - 161.1195681661) :
         clamprgb(
             288.1221695283 * Math.pow(temperature - 60.0, -0.0755148492))) *
        scale,
    (temperature >= 66 ?
         255 :
         (temperature <= 19 ?
              0 :
              clamprgb(
                  138.5177312231 * Math.log(temperature - 10.0) -
                  305.0447927307))) *
        scale

  ];
}

function hsv2rgb(h, s, v) {
  if (s == 0.0) return [v, v, v];

  i = parseInt(h * 6.0);
  f = (h * 6.0) - i;
  p = v * (1.0 - s);
  q = v * (1.0 - s * f);
  t = v * (1.0 - s * (1.0 - f));
  i = i % 6;

  switch (i) {
    case 0:
      return [v, t, p];
    case 1:
      return [q, v, p];
    case 2:
      return [p, v, t];
    case 3:
      return [p, q, v];
    case 4:
      return [t, p, v];
    case 5:
      return [v, p, q];
  }
}

function cel2far(v) {
  return Math.round((v * 1.8 + 32.0) * 10) / 10;
}

// The dashboard shares the authenticated parent RPC client; it never stores credentials.
let garageDashboardComponent = null, garageDashboardTelemetry = null;
let garageDashboardAccess = false, garageDashboardAccessEpoch = 0;
function garageDashboardSyncVisibility() {
  const visible = garageDashboardAccess && !updateInProgress;
  el("header_container").style.display = visible ? "block" : "none";
  const frame = el("garage_dashboard");
  if (!frame) return;
  if (visible) {
    if (!frame.getAttribute("src")) frame.setAttribute("src", frame.getAttribute("data-src"));
  } else if (frame.getAttribute("src")) {
    frame.removeAttribute("src");
  }
}
function garageDashboardSetAccess(allowed) {
  garageDashboardAccess = !!allowed;
  if (!allowed) {
    garageDashboardAccessEpoch++;
    garageDashboardComponent = garageDashboardTelemetry = null;
  }
  garageDashboardSyncVisibility();
}
function garageDashboardRefresh(c, cd) {
  if (!garageDashboardAccess || updateInProgress) return;
  garageDashboardComponent = c;
  const motion = c.garageMotionState && c.garageMotionState.motion;
  garageDashboardTelemetry = {status: cd, motion: motion ? {
    from:motion.from,to:motion.to,duration:motion.duration,
    elapsed:Math.max(0,performance.now()-motion.start)
  } : null};
  const frame = el("garage_dashboard");
  if (frame && frame.contentWindow)
    frame.contentWindow.postMessage({garageTelemetry:garageDashboardTelemetry},location.origin);
}
window.addEventListener("message", function(event) {
  if (!garageDashboardAccess || updateInProgress) return;
  const frame = el("garage_dashboard");
  if (!frame || event.source !== frame.contentWindow || event.origin !== location.origin) return;
  if (event.data && event.data.garageAction === "theme" &&
      ["day", "night"].includes(event.data.theme)) {
    document.documentElement.dataset.theme = event.data.theme;
    return;
  }
  if (event.data && event.data.garageAction === "ready" && garageDashboardComponent)
    garageDashboardRefresh(garageDashboardComponent, garageDashboardTelemetry.status);
  if (!event.data || event.data.garageAction !== "toggle" || !garageDashboardComponent) return;
  // Reuse the existing control, including its blocked-state check and error handling.
  const button = el(garageDashboardComponent, "toggle_btn");
  if (!button.disabled) button.click();
});
