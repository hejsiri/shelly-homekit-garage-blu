const fs = require('node:fs'), vm = require('node:vm'), assert = require('node:assert/strict');
const source = fs.readFileSync('fs_src/script.js', 'utf8');
const cases = source.slice(source.indexOf('    case "wifi_sta_ps_mode":'), source.indexOf('    case "hap_paired":'));
const elements = {
  wifi_sta_ps_mode: { value: 1 },
  wifi_sta_ps_mode_0: { disabled: false },
  wifi_ps_note: { style: {} },
};
const context = { el: key => elements[key], selectIfNotModified: (element, value) => { element.value = value; } };
vm.createContext(context);
vm.runInContext('function update(key, value) { switch(key) { ' + cases + ' } }', context);
context.update('wifi_min_ps_mode', 1);
assert.equal(elements.wifi_sta_ps_mode_0.disabled, true);
assert.equal(elements.wifi_ps_note.style.display, 'block');
context.update('wifi_sta_ps_mode', 1);
assert.equal(elements.wifi_sta_ps_mode.value, 1);
context.update('wifi_sta_ps_mode', 2);
assert.equal(elements.wifi_sta_ps_mode.value, 2);
context.update('wifi_min_ps_mode', 0);
assert.equal(elements.wifi_sta_ps_mode_0.disabled, false);
assert.equal(elements.wifi_ps_note.style.display, 'none');
console.log('PASS: Gen3 disables mode 0 and explains requirement; other models retain all options');
