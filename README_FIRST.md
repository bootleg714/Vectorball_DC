# Vectorball DC — v0.30 larger ball and finer frontal aiming

Based on v0.29, which you confirmed works great on real hardware. This candidate enlarges the current ball another25% and tunes frontal ground shots for finer aiming control.

## Build

Extract `Vectorball_DC_Ball_Aiming_v0.30` into a fresh folder. In your configured DreamSDK terminal run:

```sh
make clean
make gdemu
```

Load that folder's `gdemu/01/disc.cdi`. Same tools/dependencies; no compiled ELF/CDI included.

## Changes

Ball radius increases **2.75 -> 3.4375**, diameter **5.5 -> 6.875 DC units**. This is another25% above v0.29, or56.25% above the original ball. Visible size and collision size match. Mesh, shadow, framing, contact reach and wall/goal clearances share the new radius. Arena, goals, world scale and ball speed remain unchanged.

Hit angles already vary continuously; the ball's visible polygons do not limit shot directions. For grounded front-bumper hits, this pass makes small contact offsets less sensitive: a5-degree off-center contact now sends a stationary ball about3.25degrees off-center. Centered shots stay straight; larger offsets still produce wider shots without angle snapping. The adjustment fades smoothly into the existing side/rear response.

This changes frontal shot direction, preserving the accepted impulse strength, upward lift and car recoil. Existing ball motion still affects the final trajectory. Side/rear contacts and aerial/flip swats keep their previous response. The aiming adjustment applies to both cars.

Approved normal/boost caps remain37.39/61.00 DC units/s; total ball cap159.12. Faster acceleration, stronger ordinary hits, flip bonus, steering, gravity, jumping and five-minute match timing stay unchanged. Ball subdivisions and lean versus rendering stay unchanged.

## Playtest

Try straight shots and small left/right offsets, then wider contacts and shots on a moving ball. Compare normal and boosted hits, flip swats, banks, posts/crossbar, ball camera and versus FPS. The main question is whether frontal aiming feels more controllable without making wide shots difficult.

## Controls

| Control | Action |
|---|---|
| Stick left/right | Steer |
| Right trigger | Accelerate |
| Left trigger | Brake/reverse |
| A | Boost |
| B | Jump; release before second B for one airborne flip |
| Stick up + second B | Forward flip |
| Stick down + second B | Backflip, even while accelerating |
| B + left/right on first jump | Barrel roll |
| B while overturned | Recovery |
| X | Chase / ball camera |
| Y | Reset positions, preserve score |
| Start | Pause / resume |
| Y while paused | Reset match and scores |

For2P VERSUS use ports A (blue/top) and B (orange/bottom). Either Start pauses; P1 handles shared reset and B-on-pause returns to title. Disconnecting a required controller freezes versus.

## Checks and previews

Full host suite and sanitizer checks passed, including8aim sweeps of1,121contact angles each, both players' stick flips, recovery, bank/goal/ceiling contacts and long physics/split tests. No v0.30 target build/emulator/hardware test here; v0.29 is the accepted hardware baseline.

New `SIMULATION_PREVIEW.mp4`: three real ground shots with left, center and right offsets,2.5seconds each.640x480,7.5seconds total,20fps offline encoding; not game FPS. `BALL_PREVIEW.png` is new. Other PNGs/audio and the historical renderer comparison remain older references.

Run `sh tests/run.sh`. Results: `SIMULATION_RESULTS.txt`. Implementation/history: `HANDOFF.md`. Upper-wall traction/ceiling hanging and separate P2 engine voice remain pending.
