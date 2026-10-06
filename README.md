# SM64 for GMod (libsm64) — UNTESTED first draft

Play as Mario in Garry's Mod. Mario is simulated client-side by libsm64 (30 Hz); the server only
moves your player entity to Mario's position (validated + rate limited). Other players see your
normal playermodel, not Mario.

## Legal
No ROM, assets or Nintendo code is included. Every player supplies their own US ROM at
`garrysmod/data/sm64/baserom.us.z64` (8 MB .z64). Do not redistribute it or extracted assets.

## Setup
1. Build libsm64 (`make` in its repo; needs the US ROM to build the test, but the lib itself extracts assets at runtime).
2. Build this module: `cmake -S . -B build -DGMOD_MODULE_BASE=... -DLIBSM64=... && cmake --build build --config Release`
3. Copy `gmcl_sm64_win64.dll` (or `gmcl_sm64_linux64.dll`) to `garrysmod/lua/bin/`.
   Put `sm64.dll` (Windows) / `libsm64.so` (Linux) where the OS loader finds it (e.g. next to the GMod binary).
4. Put this addon folder in `garrysmod/addons/sm64_gmod/`.
5. In game: `sm64_toggle` (bind it), WASD move, Space = A (jump), Mouse1 = B (punch), Ctrl = Z (crouch/pound).
   `sm64_respawn` respawns; `sm64_cam_dist`, `sm64_flip_winding` are client ConVars.

## Test plan
1. `sm64_toggle` on flat ground: Mario appears, textured, stands on floor.
2. Walk, jump, triple jump, wall kick, crouch, ground pound.
3. Camera-relative movement is correct (W goes away from camera). If left/right or forward/back are inverted, flip signs in `Tick()` (`cl_sm64.lua`).
4. Falls through floors / spawn fails -> `sm64_flip_winding 1`, toggle off/on.
5. Server: confirm position updates are accepted and the player doesn't rubber-band.

## Known risks / TODO (nothing here has been compiled or run)
- Axis mapping, triangle winding, UV orientation and stick/camera signs are best guesses.
- `render.Clear` + scissor per pixel to fill the texture RT may need adjusting.
- World collision comes from the world physmesh; props and displacements may be missing/odd. Static only (no moving platforms).
- Int16 range limits Mario's world to about +-14400 Source units at scale 0.45; outside triangles are skipped.
- No interpolation between 30 Hz ticks, no sound, no other-player Mario rendering, no health/death handling.
