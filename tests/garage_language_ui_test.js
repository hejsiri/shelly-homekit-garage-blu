const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const saved = new Map(); let alertText, confirmText;
const context = {
  localStorage: {getItem: k => saved.get(k), setItem: (k, v) => saved.set(k, v)},
  document: {readyState: 'loading', body: null, addEventListener() {}, dispatchEvent() {}, querySelectorAll() {return [];}},
  CustomEvent: class {constructor(type, options) {this.type = type; this.detail = options.detail;}},
  requestAnimationFrame() {},
  window: {addEventListener() {}, alert: text => {alertText = text;}, confirm: text => {confirmText = text; return true;}},
};
vm.createContext(context); vm.runInContext(fs.readFileSync('fs_src/i18n.js', 'utf8'), context);
const ui = context.window.GarageI18n;
assert.equal(ui.t('Save'), 'Save');
ui.setLanguage('pl');
assert.equal(saved.get('garage-ui-language'), 'pl');
assert.equal(ui.t('Save'), 'Zapisz');
assert.equal(ui.t('Show in dashboard'), 'Pokazuj w dashboard');
for(const [source,translated] of [['Temperature','Temperatura'],['Humidity','Wilgotność'],['Contact sensor','Czujnik kontaktowy'],['Motion','Ruch'],['Light','Światło'],['Smoke','Dym'],['Contacts closed','Zamknięte'],['Contacts open','Otwarte'],['Motion sensor','Czujnik ruchu'],['Button','Przycisk'],['Sensor','Czujnik']])assert.equal(ui.t(source),translated);
assert.equal(ui.t('Device detected: AA:BB:CC:DD:EE:01'), 'Wykryto urządzenie: AA:BB:CC:DD:EE:01');
assert.equal(ui.t('Open · stale'), 'Otwarta · nieaktualne');
assert.equal(ui.t('Signal 3 of 4 bars'), 'Sygnał: 3 z 4 kresek');
assert.equal(ui.t('Version 2.16.1-blu54 is available.'), 'Dostępna jest wersja 2.16.1-blu54.');
assert.equal(ui.t('2 active'), '2 aktywnych');
context.window.alert('Name must not be empty'); assert.equal(alertText, 'Nazwa nie może być pusta');
assert(context.window.confirm('Reboot the device?')); assert.equal(confirmText, 'Uruchomić urządzenie ponownie?');
assert.equal(ui.t('AA:BB:CC:DD:EE:01'), 'AA:BB:CC:DD:EE:01');
assert.equal(ui.t('Shelly.BLU.Pair'), 'Shelly.BLU.Pair');
ui.setLanguage('en'); assert.equal(ui.t('Save'), 'Save');
ui.setLanguage('fr'); assert.equal(ui.getLanguage(), 'en');
const index = fs.readFileSync('fs_src/index.html', 'utf8'), garage = fs.readFileSync('fs_src/garage.html', 'utf8');
assert(index.indexOf('src="i18n.js"') < index.indexOf('src="script.js"'));
assert(garage.indexOf('src="i18n.js"') < garage.indexOf('const system=matchMedia'));
console.log('PASS: EN/PL selection, persistence, static/dynamic text, alert/confirm, technical values and bundled language loader order');
