# Vectorball DC — v0.32 wall rollback

This revises the v0.31 trial to keep the wheels against the wall as the car slides back down, even with the gas floored.

## Build

Extract `Vectorball_DC_Wall_Slide_v0.32` into a fresh folder. In your configured DreamSDK terminal run:

```sh
make clean
make gdemu
```

Load that folder's `gdemu/01/disc.cdi`. Same tools/dependencies; no compiled ELF/CDI included.

## What changed

The previous trial reduced both driving traction and the forces keeping the tires against the wall. That let the car fall away.

This version keeps wall adhesion, suspension alignment and steering support while reducing the tire forces that drive the car upward. After a high crest, gravity brings it down along the wall. Straight climbs can roll backward down the wall; angled runs retain their carving/steering behavior. Upward engine force stays disabled until the car reaches the bottom bank, so holding the gas cannot restart the climb halfway down.

B still deliberately jumps away from the wall; the second B press can flip. Contact with a real surface is required—there is no attachment across the goal opening or out into empty space. Ceiling contacts still release. Lower-bank/flat handling, ball size/aiming, speeds, hit strength and match rules retain the accepted tuning.

## Playtest

Drive straight and at an angle from the pitch up the bank, then hold full throttle or boost through the crest and rollback. Look for the wheels staying against the wall and a smooth return toward the bank. Try steering, braking/reversing, jumping off during the slide, and the second-jump flip. Check both players/cameras, lower-wall carving, landings and versus FPS.

Host checks pass for96wall/player/throttle/boost fixtures, including straight and angled approaches from the pitch. Across28,316checked slide ticks over solid wall, each retains at least two wheel contacts. All return to the lower bank with full throttle and continuously replenished boost. Intentional jumps still detach. A15,600-tick lower-wall/flat comparison matches v0.30 byte-for-byte in the tested cases.

No v0.32 Dreamcast compile/emulator/hardware test here. v0.30 remains the accepted hardware comparison; v0.31 was the partial trial you reported falling away from the wall.

## Controls

B jumps; release B before the second press. Stick up + second B flips forward; stick down + second B flips backward. B + left/right on the first jump barrel-rolls. B when overturned recovers. X changes camera. Other controls are unchanged.

For2P VERSUS use ports A (blue/top) and B (orange/bottom). Either Start pauses; P1 handles shared reset and B-on-pause returns to title.

## Previews and checks

New `SIMULATION_PREVIEW.mp4` is an eight-second host inspection view: blue approaches the bank from the pitch, orange begins high on the wall, both holding throttle/boost with ordinary fuel consumption. Inspection cameras track the chassis to show contact/rotation; this is not a recording of the gameplay chase camera.20fps is offline encoding, not game FPS; badge dashes. `WALL_SLIDE_PREVIEW.png` is new. Other previews remain historical references.

Run `sh tests/run.sh`. Results: `SIMULATION_RESULTS.txt`. Implementation/history: `HANDOFF.md`. Separate P2 engine audio remains pending.
