import { system, world } from '@minecraft/server';

// Refresh well before the final ten seconds, when Night Vision starts flashing.
// Query current players every time so respawns, joins and dimension changes recover.
system.runInterval(() => {
  for (const player of world.getAllPlayers()) {
    try {
      player.addEffect('minecraft:night_vision', 1200, {
        amplifier: 0,
        showParticles: false,
      });
    } catch (error) {
      // One departing player must not prevent the others receiving the effect.
      console.warn(`[Kitchen Night Vision] Could not refresh effect: ${error}`);
    }
  }
}, 100);
