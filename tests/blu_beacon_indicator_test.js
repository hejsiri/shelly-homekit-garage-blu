const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('fs_src/script.js', 'utf8');
const start = source.indexOf('function bluReadingAge(');
const end = source.indexOf('function bluPairStatus(', start);
const timers = new Map(), classes = new Set();
let nextTimer = 0, flashes = 0;
const node = {classList: {add: c => { classes.add(c); flashes++; }, remove: c => classes.delete(c)}};
const context = {
  setTimeout(callback, ms) { assert.equal(ms, 650); timers.set(++nextTimer, callback); return nextTimer; },
  clearTimeout(id) { timers.delete(id); },
};
vm.createContext(context); vm.runInContext(source.slice(start, end), context);
const sensor = {secure: true, seen: true, ready: true, age_s: 0, packet_seq: 10, active: false, battery: 90, rssi: -60};
const update = (identity = 'AA:01:bthome') => context.bluSensorStatus(node, sensor, 180, false, identity);
const blue = () => classes.has('blu-received');
const finish = () => { for (const [id, cb] of timers) { timers.delete(id); cb(); } };
update(); assert(!blue(), 'Initial snapshot must not pretend to be a new beacon');
sensor.age_s = 1; update(); assert.equal(flashes, 0, 'Polling does not flash');
sensor.packet_seq++; sensor.age_s = 0; update(); assert(blue()); assert.equal(flashes, 1);
sensor.age_s = 1; update(); assert(blue()); assert.equal(flashes, 1, 'Same beacon must not restart pulse');
sensor.packet_seq++; sensor.age_s = 0; update(); assert.equal(timers.size, 1, 'Replace previous pulse timeout');
finish(); assert(!blue(), 'Restore original badge color');
update(); assert(!blue(), 'An already displayed packet cannot flash again');
sensor.active = true; update(); assert(!blue(), 'A changed display alone is not a beacon');
sensor.packet_seq++; update(); assert(blue());
update('BB:02:bthome'); assert(!blue(), 'Changing assigned device clears old pulse');
sensor.packet_seq = 1; update(); assert(!blue(), 'Sequence reset establishes a baseline');
sensor.packet_seq++; sensor.age_s = 181; update(); assert(!blue(), 'Stale snapshot cannot flash');
sensor.age_s = 0; update(); assert(!blue());
sensor.packet_seq++; update(); assert(blue());
sensor.secure = false; update(); assert(!blue(), 'Unpairing clears pulse');
sensor.secure = true; delete sensor.packet_seq; update(); assert(!blue(), 'Missing sequence cannot produce reception feedback');
sensor.packet_seq = 4294967295; update(); sensor.packet_seq = 0; update(); assert(blue(), 'Handle uint32 sequence wrap');
assert(fs.readFileSync('fs_src/style.css', 'utf8').includes('.blu-sensor.blu-received .blu-badge'));
console.log('PASS: beacon flash, no repeat on polling, timeout reset, device change, stale/unpaired data, sequence reset and wrap');

// Restored battery is visible before the first new state beacon, without a false flash.
sensor.seen=false;sensor.battery=45;sensor.protocol='xiaomi_bindkey';update();
assert(node.innerHTML.includes('45%'));assert(node.innerHTML.includes('Waiting for encrypted beacon'));
assert(!blue());
sensor.battery=-1;update();assert(!node.innerHTML.includes('blu-power'));
console.log('PASS: cached battery before first beacon and hiding unknown battery without inventing contact state');

// A smoke sensor must not turn missing/stale data into an all-clear.
sensor.secure=true;sensor.seen=false;sensor.battery=-1;
context.bluSensorStatus(node,sensor,180,'smoke','smoke');
assert(node.innerHTML.includes('Waiting for smoke status'));
sensor.seen=true;sensor.ready=true;sensor.age_s=0;sensor.active=true;
sensor.smoke_seen=true;sensor.smoke_age_s=0;
context.bluSensorStatus(node,sensor,180,'smoke','smoke');
assert(node.innerHTML.includes('Smoke detected')&&node.innerHTML.includes('blu-alarm'));
sensor.age_s=200;sensor.ready=false;
sensor.smoke_age_s=200;
context.bluSensorStatus(node,sensor,180,'smoke','smoke');assert(node.innerHTML.includes('Smoke detected'));
sensor.active=false;context.bluSensorStatus(node,sensor,180,'smoke','smoke');
assert(node.innerHTML.includes('No data')&&!node.innerHTML.includes('No smoke detected'));
console.log('PASS: unknown/stale smoke never reports all-clear; known alarm remains visible');
