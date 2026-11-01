// Tests scheduling and recovery using a mock API, not the Minecraft renderer.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
let callback;
let players = [];
const calls = [];
const warnings = [];
const context = vm.createContext({ console: { warn: text => warnings.push(text) } });
const api = new vm.SyntheticModule(['system', 'world'], function () {
  this.setExport('system', { runInterval(fn, ticks) { callback = fn; assert.equal(ticks, 100); } });
  this.setExport('world', { getAllPlayers() { return players; } });
}, { context });
const module = new vm.SourceTextModule(readFileSync(new URL('../src/night-vision/main.js', import.meta.url), 'utf8'), { context });
await module.link(name => { assert.equal(name, '@minecraft/server'); return api; });
await module.evaluate();
assert.equal(typeof callback, 'function');
callback(); // Empty world must be safe.
const player = { addEffect(...args) { calls.push(args); } };
players = [{ addEffect() { throw Error('player left'); } }, player];
callback();
assert.equal(warnings.length, 1);
assert.equal(calls.length, 1); // A disconnect must not interrupt the other players.
assert.equal(calls[0][0], 'minecraft:night_vision');
assert.equal(calls[0][1], 1200);
assert.equal(calls[0][2].showParticles, false);
assert.equal(calls[0][2].amplifier, 0);
players = [player, player]; // Re-query and refresh current players on the next tick.
callback();
assert.equal(calls.length, 3);
console.log('PASS Night Vision scheduling, effect options, joins, refresh, and disconnect isolation (mock API)');
