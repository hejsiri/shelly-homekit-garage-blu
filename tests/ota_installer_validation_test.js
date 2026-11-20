const assert = require('node:assert/strict');
const { validIP, verifyDevice } = require('../tools/ota/update/installer.js');
const rules = require('../tools/ota/update/compatibility.json');

const gen3 = { id: 'shelly1g3-34cdb07c28b0', model: 'S3SW-001X16EU', gen: 3,
  app: 'S1G3', ver: '2.2.0', fw_id: '20261109-150519/2.2.0-g7968dca' };
const plus1 = { id: 'shellyplus1-001122aabbcc', model: 'SNSW-001X16EU', gen: 2,
  app: 'Plus1', ver: '1.7.6', fw_id: '20261026-143003/1.7.6-g01e9e18' };
assert.equal(verifyDevice(gen3, rules).model, 'Shelly1Gen3');
assert.equal(verifyDevice({ id: 1, result: gen3 }, rules).model, 'Shelly1Gen3');
assert.equal(verifyDevice(plus1, rules).model, 'ShellyPlus1');
for (const change of [
  { id: 'shelly1pmg3-001122aabbcc' }, { model: 'S3SW-001P16EU' }, { gen: 4 },
  { app: 'Mini1G3' }, { ver: '2.1.0' }, { ver: '2.3.0' }, { ver: '2.2.0-beta1' },
  { ver: '2.16.1_blu.79' }, { fw_id: '' }, { fw_id: '20261109-150519/2.1.0-g7968dca' },
  { id: 'shelly1g3-34cdb07c28b0.evil' },
]) assert.throws(() => verifyDevice({ ...gen3, ...change }, rules));
for (const bad of [null, {}, [], 'text', { result: { error: 'no info' } }]) {
  assert.throws(() => verifyDevice(bad, rules));
}
assert.throws(() => verifyDevice({ ...plus1, ver: '1.7.5' }, rules));
assert.throws(() => verifyDevice({ ...plus1, ver: '1.7.7' }, rules));
assert(validIP('192.168.1.198'));
for (const ip of ['192.168.1.256', '192.168.01.1', '192.168.1.1/path', 'device.local', '']) assert(!validIP(ip));
console.log('PASS: hardware model, generation, app, stock version/build and IP validation; older/newer/custom firmware rejected');
