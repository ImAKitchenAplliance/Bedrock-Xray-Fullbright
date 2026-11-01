# Bedrock X-ray & Fullbright

Four independent resource packs and an optional Night Vision behavior pack for
Minecraft Bedrock on Windows. These are experimental packs, not a client mod.
**X-ray is confirmed broken on the reported Windows Bedrock v26.60 setup.**
The other packs still need in-game validation. The Night Vision option uses a
world script; the other four are resource-only packs.

## Downloads

- [Kitchen Brighter Blocks](dist/Kitchen-Brighter-Blocks.mcpack)
- [Kitchen Solid Ores](dist/Kitchen-Solid-Ores.mcpack)
- [Kitchen Night Vision — no Vibrant Visuals](dist/Kitchen-Night-Vision.mcpack)
- [Kitchen X-ray — known broken on Windows v26.60](dist/Kitchen-Xray.mcpack)
- [Kitchen Fullbright — Vibrant Visuals](dist/Kitchen-Fullbright-VV.mcpack)

Download each file to your Windows PC, then double-click it to import into Minecraft.
In a test world's **Settings → Resource Packs → My Packs**, activate the desired
pack. You can activate either independently, or both together. Put these above other
packs that change the same resources. Reload the world after changing packs.
To uninstall, deactivate the packs; imported copies can be deleted in Minecraft's
storage settings. Import a rebuilt pack again after incrementing its version if
Minecraft keeps an older cached copy.

## Night Vision — standard graphics, no Vibrant Visuals

This is the practical alternative for Windows machines without Vibrant Visuals.
It is a **behavior pack**, not a fullbright texture pack: every five seconds it
applies 60 seconds of Night Vision I, without particles, to every player in the
world. It uses stable `@minecraft/server` 2.0.0; no Beta APIs experiment is requested.
Night Vision brightens dark areas but does not remove fog, darkness effects, or
Bedrock's hidden-face culling, and does not change actual block light or mob spawning.

Double-click `Kitchen-Night-Vision.mcpack` to import, then activate it under your
world's **Behavior Packs → My Packs**. Test on a copy of your world first: behavior
packs can affect achievement eligibility; check the game's warning before saving.
You need control of the world/server. This cannot be installed as a global resource
pack to gain Night Vision on arbitrary multiplayer servers.

The effect is automatically reapplied after death, drinking milk, joining, or
changing dimensions, within five seconds. Deactivate the pack and reload to stop;
its last effect expires within 60 seconds of active game time. It refreshes any
existing Night Vision effect, so do not use alongside another add-on that manages
that effect unless you want this behavior.

**In-game checks still required:** with Simple/Fancy graphics, enter a sealed dark
room and wait five seconds. Confirm Night Vision is active and visibility improves.
Drink milk and confirm the effect returns. Repeat after death/respawn and in the
Nether/End. Disable the pack, reload, and wait 60 seconds to confirm it expires.
If nothing happens, send the Minecraft version and content-log script errors.

## X-ray — known broken on reported Windows v26.60 setup

**User-tested failure:** terrain renders black rather than see-through, including
when X-ray is the only active resource pack. This rules out the Brighter Blocks
and Solid Ores combination as a necessary cause. The result is consistent with
opaque terrain rendering ignoring texture alpha; the internal renderer cause has
not been verified. No working fix is available. Deactivate this pack on that setup.
The archive is retained for investigation, not as a working download. File checks
passing do not override this failed in-game result.

Overrides 35 common natural terrain blocks with an entirely transparent texture.
Ores, ancient debris, chests, bedrock, water, lava, and building blocks keep their
normal textures. This deliberately does not recolor ores or hide dangerous fluids.

**Renderer limitation:** transparency does not force Bedrock to draw hidden faces.
Fully enclosed ores may remain invisible, and opaque block render paths can ignore
texture alpha. This is a terrain-transparency approach, not a guarantee of seeing
all underground blocks. It does not change collision: invisible terrain remains
solid. Check the pack in a disposable Creative world before relying on it. Server
resource packs and anti-xray can also affect the result. Use only where permitted.

## Fullbright (Vibrant Visuals only)

Uses Bedrock's `lighting/global.json` ambient lighting settings: white ambient
illumination at 1.0 instead of the Mojang sample's 0.02. It is intended to brighten
unlit areas without changing block textures. This is an ambient-light approximation
of fullbright, not a guaranteed flat/unlit shader. Lighting, fog, exposure, dimensions,
and engine version can affect the result.

Select **Settings → Video → Graphics Mode → Vibrant Visuals** on supported hardware.
The pack declares the `pbr` capability. If Vibrant Visuals is unavailable, this
texture-only pack cannot provide fullbright on your Windows renderer.
**Simple/Fancy RenderDragon modes do not support the old lightmap fullbright trick.**
This pack intentionally does not include a misleading legacy lightmap.

## In-game acceptance checks (still required)

1. Import both files separately; confirm both names and icons appear without errors.
2. In a Creative test world, compare stone/deepslate terrain with X-ray off/on. Check
   exposed and fully enclosed diamond ore separately, as face culling differs.
   Check that water, lava, ore textures, and solid-block collision remain unchanged.
3. Enable Vibrant Visuals, enter a sealed unlit room at night, and compare Fullbright
   off/on after reloading. Also check the Nether and End. If lighting does not change,
   inspect Minecraft's content log for the lighting resource before using the pack.
4. Activate both packs and repeat. Deactivate both to confirm vanilla visuals return.

No Windows Minecraft client is available in the build environment. Archive and
resource validation passes do **not** establish that either visual effect works
in-game; the above checks remain outstanding.

## Build

Python 3.10+; no packages or network access required:

```sh
python3 tools/build.py
python3 tools/validate.py
# Optional script logic tests (Node.js 18+; mock API, not Minecraft):
node --experimental-vm-modules tools/test-night-vision.mjs
```

Editable definitions live in `tools/build.py`; the Night Vision script is
`src/night-vision/main.js`. The build writes the unpacked packs
under `packs/` and reproducible import archives under `dist/`. Stable pack UUIDs
allow updates. Raise `VERSION` before distributing an updated release. Pack manifests
require Bedrock 1.21.90 or newer; actual renderer compatibility requires game testing.

Format references checked against Mojang's public
[Bedrock samples](https://github.com/Mojang/bedrock-samples/tree/main/resource_pack):
`blocks.json`, `manifest.json`, and `lighting/global.json`. The original packs contain generated assets. Brighter Blocks additionally includes
modified vanilla sample textures, credited in its asset notice.

## Brighter Blocks — standard graphics resource pack

[Download Kitchen Brighter Blocks](dist/Kitchen-Brighter-Blocks.mcpack).
Lifts each block texture's RGB channels 40% toward white, preserving original detail,
alpha, dimensions and animation-strip layout. No behavior pack, cheats, scripts,
Vibrant Visuals, or ray tracing required. Targets the vanilla `textures/blocks`
color assets in the recorded Mojang sample snapshot, including animated textures.
Blocks rendered from entity textures (for example chests), biome color tints,
UI/items, and future textures outside that snapshot are not brightened by this pack.

This changes texture colors, **not lighting**: even pale textures still become dark
in unlit areas. It does not change mob spawning, collision, fog, or ore visibility
behind solid blocks. The modified vanilla assets remain subject to the Minecraft
EULA; attribution and the source snapshot are included in the pack.

## Solid Ores — standard graphics resource pack

[Download Kitchen Solid Ores](dist/Kitchen-Solid-Ores.mcpack).
Replaces entire ore faces with flat, vivid colors, including normal and deepslate
variants. Ancient debris is the naturally generated source of netherite, so both
ancient debris and the netherite storage block are pink. Gilded blackstone is yellow.

| Ore | Color |
| --- | --- |
| Diamond | Blue |
| Ancient debris / netherite block | Pink |
| Emerald | Green |
| Gold / Nether gold | Yellow |
| Iron | Orange |
| Copper | Turquoise |
| Coal | Pale gray |
| Redstone | Red |
| Lapis | Purple |
| Nether quartz | White |

Despite the requested glowing appearance, these are high-visibility colors,
**not light-emitting or emissive textures** on standard graphics. Normal lighting
and face shading still apply. No behavior changes or modified drops are included.

Import either `.mcpack` by double-clicking, then activate under **Resource Packs**.
When combining, order packs from highest to lowest: **Solid Ores → X-ray (optional)
→ Brighter Blocks**. Other texture packs may conflict. Reload the world after
changing the stack. Test the ore palette in a lit Creative world, then compare the
brightness pack in daylight and an unlit room. Check plants/glass for preserved
transparency and water/lava for animation. In-game validation remains outstanding.

To regenerate these assets, install `Pillow==12.3.0` in a Python environment and run:

```sh
python3 tools/build-visibility.py /path/to/bedrock-samples
python3 tools/test-visibility.py /path/to/bedrock-samples
```

Use Mojang sample commit `15701fa9c13923f12662984c32c124327e40e3d3` with all
`resource_pack/textures/blocks` color assets available. Generated assets are checked
in, so ordinary `python3 tools/build.py` packaging needs no download or Pillow.
