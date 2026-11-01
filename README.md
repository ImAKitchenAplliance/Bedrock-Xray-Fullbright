# Bedrock X-ray & Fullbright

Two independent resource packs for Minecraft Bedrock on Windows. These are initial,
in-game **unverified** packs, not a client mod. No behavior packs, scripts, commands,
external textures, or achievements-changing world settings are included.

## Downloads

- [Kitchen X-ray](dist/Kitchen-Xray.mcpack)
- [Kitchen Fullbright — Vibrant Visuals](dist/Kitchen-Fullbright-VV.mcpack)

Download each file to your Windows PC, then double-click it to import into Minecraft.
In a test world's **Settings → Resource Packs → My Packs**, activate the desired
pack. You can activate either independently, or both together. Put these above other
packs that change the same resources. Reload the world after changing packs.
To uninstall, deactivate the packs; imported copies can be deleted in Minecraft's
storage settings. Import a rebuilt pack again after incrementing its version if
Minecraft keeps an older cached copy.

## X-ray

Overrides 38 common natural terrain blocks with an entirely transparent texture.
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
```

Editable definitions live in `tools/build.py`. The build writes the unpacked packs
under `packs/` and reproducible import archives under `dist/`. Stable pack UUIDs
allow updates. Raise `VERSION` before distributing an updated release. Pack manifests
require Bedrock 1.21.90 or newer; actual renderer compatibility requires game testing.

Format references checked against Mojang's public
[Bedrock samples](https://github.com/Mojang/bedrock-samples/tree/main/resource_pack):
`blocks.json`, `manifest.json`, and `lighting/global.json`. The packs contain generated
assets, not redistributed vanilla texture files.
