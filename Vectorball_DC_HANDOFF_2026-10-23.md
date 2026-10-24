# Vectorball DC handoff — current prototype 0.13 wall carving

## Resume here — latest play-test result, October 23, 2026

**Accepted baseline: v0.13 wall carving.** The user tested the latest build and said: “bam. good job!” The angled climb/crest/downward arc is now accepted. The one remaining correction reported is **deliberate player turning while riding the wall**. Work paused for the next session; no steering fix has been implemented after that feedback.

**Immediate next task:** refine player-commanded wall steering while preserving the working neutral-stick carving behavior. The current code is intended to suppress momentum-following assistance during steering, but the play test shows that deliberate turning still needs refinement. Do not describe manual override as fully verified or complete.

Suggested investigation (hypotheses, not diagnosed findings):

1. Reproduce left/right wall steering and countersteering during ascent, near the crest and on descent. Establish whether the remaining problem is response strength, steering direction, delay or competition between the target-heading torque, wheel steer forces and carving assist.
2. Inspect vehicle.c's transported contact_forward target, steering update, wall_arc latch/input thresholds, yaw damping and front-wheel forces. Ensure actual player intent takes priority; avoid a sudden correction when returning the stick to neutral.
3. Add behavioral checks for deliberate left/right turns and countersteering, alongside the existing eight unsteered carving regressions. Keep the good arcs, flat-ground handling, jump behavior, contact loss and recovery intact.
4. Verify in host simulation and reference renders first, then package a separate source build candidate for the user to compile/test. Focus on this small remaining correction before expanding features.

**Current source package:** Vectorball_DC_Wall_Carving_v0.13.zip, containing the full source, tests, HANDOFF.md, SIMULATION_RESULTS.txt and side-by-side SIMULATION_PREVIEW.mp4. This standalone handoff adds the latest acceptance/next-step notes after that ZIP was delivered; it supersedes the handoff inside that existing ZIP. No game code changed for this handoff update.

**Preserve:** 120 × 160 field; ball diameter4.4; goals24 × 8; six-section banks6 wide/3 high; rounded corners; accepted car models, sound, adaptive cameras and working carving arc. Long-term direction is a satisfying original Dreamcast car-and-ball game, developed through small tested steps. No need to enlarge the arena or add more features now.

**Validation boundaries:** strict host checks passed for v0.13 (detailed below), and user play-testing accepted the carving. The user's latest acceptance did not specify device or numerical FPS; do not invent a new hardware/performance result. Earlier real-hardware reports confirmed good performance/no drops with the enlarged field and ball. This environment has no Dreamcast compiler/emulator; host renders and tests do not establish target FPS.

**Build:** in the new source folder, use the existing configured DreamSDK terminal and run `make gdemu` -> `gdemu/01/disc.cdi`. `make cdi` is an alias. Preserve the working elf2bin/scramble/makeip/makedisc workflow and vehicle.o linkage. Do not reintroduce mkdcdisc or duplicate KOS linker flags; kos-cc already supplies them. Keep ZIP file timestamps in the past to avoid the earlier future-time warnings.

The remaining sections document the implementation, tests, lessons and earlier milestones. Read this resume section as the latest status; older version sections retain their historical test status.

## Implementation and checks: 0.13 angled wall arcs

User accepted 0.12 as much better but reported that angled bank entry became a straight nose-up climb, then a backward slide. Clarified desired behavior using a skateboarder carving a ramp: preserve lateral travel, crest under gravity, face along the descending arc and return forward to the field. Prior tests proved stability/return height but did not verify facing direction; that was an important coverage gap.

Reference review: live Netick CarController.cs (same public main source) has wheel forces and a limited SimulateAutoStabilization for tipped bodies: input present, environment contact, fewer than three grounded wheels, below world-height10. It does not provide an explicit automatic downhill-heading rule. That distinction matters: gravity shapes motion but does not by itself steer a wheeled body's nose along velocity. Our momentum-following arc assistance is a deliberate original arcade control choice. Centripetal acceleration is already the net normal acceleration from contact plus gravity; do not add v²/r as a separate extra force.

- Reproduced the 31.5-degree approach: original forward.z fell from about .523 on the floor to about .05 on the wall, then forward.y stayed near +1 during descending velocity. New carving_test.c fails the unmodified 0.12 build at entry heading >.30, establishing the regression.
- CarSurface adds contact_up/contact_forward, a validity flag and wall_arc. Minimal normal-to-normal rotation transports the intended heading around the bend, independently of up-vector alignment. A damped torque about the contact normal follows that heading; pose/velocity continue to integrate through the body solver.
- Tire sphere collisions now apply normal impulses only. Their former Coulomb tangential friction duplicated explicit probe tire grip and strongly scrubbed travel/caused bank sticking. Chassis sphere friction remains. Lateral probe grip is reduced continuously on steep surfaces; floor value unchanged.
- A bounded travel-direction yaw assist engages for a sufficiently angled, moving wall run, follows the arc through the crest and descent, and releases on flat ground/contact loss/jump. The intended control policy yields to steering input; reverse/brake input disables it. Latest user feedback identifies deliberate wall steering as the remaining refinement, so manual override is not yet accepted. A head-on stop is deliberately allowed to roll back instead of choosing an arbitrary sideways turn. The target frame clears on the accepted flat-driving fast path to avoid stale headings on a later wall entry.
- Renderer bug found while comparing videos: world box() had incorrectly shared car_up extrusion from the previous car draw. World boxes now extrude on world-up; car_box remains car-up. Added a world-render prefix hash regression across flat/tilted/flat car frames. This also explains changing scenery in earlier host previews when CPU was disabled; with a normal CPU its final flat frame often hid the issue.
- All strict host checks pass. Eight long-side mirrored 30/45-degree, unsteered carves retain entry direction, crest, face down while descending, exit the bank moving forward; minimum entry lateral forward .435, most negative descending forward.y -.579, maximum peak6.96. Head-on case has no forced sideways turn. Existing four wall climbs/jumps, 28 stress approaches/corners/recovery, ten-minute physics/CPU/jump runs, scoring, audio and rotated camera framing all pass. Max straight-climb rotation3.81 degrees/step. First24 stress cases max speed37.08, displacement.315/step, post-contact spin11.81rad/s. CPU kickoff5.53s, isolated flat jump2.01 unchanged.
- Jump regression now measures the outward impulse increment (>8.5) and positive outward motion, rather than requiring absolute outward speed>6 after arriving with greater inward momentum. JUMP_SPEED remains9.5.
- Carving and world-render regressions additionally pass ASan/UBSan (host leak detector disabled because process tracing prevents LeakSanitizer); no reported memory/UB errors. Final reference geometry: kickoff4290 triangles, corner2565. The corrected world extrusion changes host count versus flattened first frames in prior versions; these are not target FPS results.
- SIMULATION_PREVIEW.mp4: 12-second side-by-side host reference, left 0.12 physics and right0.13, each using fixed world rendering. Two mirrored 31.5-degree runs, initial speed20, drive/boost until y>3 then coast, no steering/jump. tests/preview.c emits the revised raw PPM frames. Full results and the expected baseline failure in SIMULATION_RESULTS.txt.
- No target toolchain/emulator here. Deliver full source Vectorball_DC_Wall_Carving_v0.13, build make gdemu -> gdemu/01/disc.cdi. Keep 0.12 for comparison; timestamps fixed in the past; no compiled ELF/CDI. User play-testing accepted the carving after delivery; deliberate wall steering remains open. Device and FPS were not specified in that latest feedback.
- Latest feedback: carving accepted; refine deliberate player turning while on the wall next. Preserve the accepted arc, field and ball sizes. Further device/FPS information has not been supplied for this version.

## Previous test: 0.12 wheel-contact simulation gate

User found 0.11 closer but still insufficient physically, and requested verification in simulation before another build candidate. Revisited Netick CarController.cs: per-wheel ray contacts, forces at contact points, rigid-body angular velocity, wheel-side stabilization, local-up jumps; its spring body animation is visual-only. Implemented an original bounded C approximation of those ideas; no reference source/assets copied.

- New vehicle.c/vehicle.h, linked as vehicle.o in the existing Makefile. Four probes, spring/damper normal forces, per-wheel motor and lateral grip at contact points, front-wheel steer axes, local inertia approximation and integrated angular velocity. Contact-normal stabilization/damping controls overshoot. Tire/chassis sphere collision impulses and friction prevent sinking and endless inverted sliding. Body motion integrates through center of mass, then derives the wheel-plane origin from orientation.
- Removed the surface-travel coordinate and prescribed normal orientation. CarSurface now holds actual up/forward/angular velocity, four WheelContact records, grounded count, attachment flag, jump cooldown and dynamic-mode flag. game_car_basis uses actual orientation in dynamic mode. Collision separation no longer repositions cars on a surface rail.
- Accepted flat driving retained as a fast path when more than three units inward of the bank toe. Near the perimeter, dynamic mode persists through falls until four flat contacts and a settled upright attitude allow a return. This explicitly preserves the existing floor behavior rather than claiming an entirely unified rigid-body car everywhere.
- Six-section elliptical quarter profile replaces the straight slope: d=-W+W*sin(theta), h=H*(1-cos(theta)); W6/H3 unchanged. arena_bank_profile shared by height lookup, wheel ray surfaces, ball impacts and rendering. arena_bank_vertex computes intersection-based polygon inset vertices so adjacent bank strips meet. Goal bank caps use the curved profile. Rounded plan-view corners unchanged.
- Cached finite surface data for 30 arena edges plus two above-goal wall segments. Wheel contacts stop at WALL_TOP; existing arena containment/backstop behavior retained for chassis/ball above the perimeter. Car goal-entry restriction retained, including dynamic cars crossing goal-side gaps. Goal aperture/scoring for the ball unchanged.
- Jump impulse follows actual car up, with a contact cooldown; airborne angular momentum and gravity retained. No unconditional airborne upright lerp. Slow overturned recovery under throttle adds a bounded recovery torque near the floor. Body/tire collision shapes and contact stabilization are approximations; tuning remains open.
- Existing sound/camera features preserved. Attached vertical speed contributes to engine sound; camera framing projects the actual rotated body. CPU uses the same contact solver and 3D stuck detection. No AI aerial tactics added.
- Simulation gate PASS: strict host C99 -O2 -Wall -Wextra -Werror -pedantic. Four wall approaches show partial/mixed wheel normals, bank tilt, vertical traction and normal jumps; max orientation increment 2.73 degrees per 120 Hz step. 24 speed/angle cases (6/20/37, straight/angled, four directions) plus four diagonal rounded-corner cases passed finite-state/containment, speed/displacement and settling checks. First 24 max speed37.08, displacement0.310/step, post-contact spin9.65rad/s. Separate low-speed inverted recovery reaches tire contact under throttle. Also existing ten-minute physics/CPU/jump runs, goal/post/crossbar checks, sound and camera checks pass. CPU kickoff score5.53s and flat jump apex2.01 unchanged.
- Curved banks launch some ball rebounds higher; corner return checks now allow a complete four-second bounce arc rather than demanding both horizontal velocities reverse within one second. Root containment tolerances include .05 units because the tire-bottom/root relation changes under tilt (the model's tire bottom is ~.02 above the origin); physical sphere constraints remain active. Flat jump apex checks remain exact to the previous range.
- SIMULATION_RESULTS.txt contains raw host check output and scope. SIMULATION_PREVIEW.mp4 is a 12-second, 20fps host software render (climb/release, then jump-off), not emulator/hardware footage. Inspected sequence frames and bank/wall close-ups. Kickoff reference geometry3,903 triangles; no hardware FPS claim.
- Build candidate only: no kos-cc or sh-elf-gcc available here. User must run make gdemu and test on Dreamcast. Full package Vectorball_DC_Wheel_Contacts_v0.12; includes all source/tests/docs, results and reference video, with past ZIP timestamps. No compiled ELF/CDI. Build process unchanged except adding vehicle.o.
- Next feedback: wheel contact through bend, traction and steering on walls/corners, jump momentum/landings, recovery, camera comfort, goal-side transitions and FPS. Do not expand features until this foundation feels good. Springs/grip/damping and the approximate collision body may need tuning.

## Previous test: 0.11 surface driving correction

User supplied hardware screenshots: 0.10 cars levitated upright along the bank, stopped at the wall; Rocket League reference showed the intended vertical car with wheels against the wall. This was missing functionality in the previous implementation, not a performance report.

- Added CarSurface for each car: attached edge, distance along the bank/wall, contact up/forward vectors and reattachment cooldown. Flat-drive path retains original constants; contact path projects acceleration, grip and gravity onto a bank or vertical-wall frame.
- Travel starts along the six-by-three bank and continues vertically above its lip. Speed transports across the bend and around corner facets; heading transports with facet normal changes. Surface normals drive car pitch/roll, including car-box extrusion (previously always world-up).
- Grounded bank/wall B jumps launch along the contact normal. Wall momentum survives detachment; world gravity and automatic upright recovery act while detached. Near WALL_TOP minus one unit, release inward with retained vertical momentum. No ceiling, free roll controls or general rigid-body solver. Straight slope/vertical joins remain angular; corner facets remain six per quadrant.
- Goal-side surface ends detach inward when no adjacent contact edge is nearby, preventing a switch to a distant wall. End-wall contact continues across the goal mouth only above GOAL_H. Ball goal fit/scoring unchanged. Cars remain barred from entering goals below the crossbar.
- Tilted car-ball ellipsoid uses the contact up vector and a ball-plus-body vertical extent. Flat grounded car-ball path unchanged. Surface car bumps use 3D center separation/impulse; flat car bump path retained. CPU speed/stuck detection and attached-car engine speed now include vertical travel.
- game_car_basis is shared by renderer, camera bounds and host camera tests. Car model coordinates, including box upper faces, transform through right/up/forward. Camera height follows car elevation above ordinary jump height; horizon stays world-upright. Perimeter mesh now follows the rounded contact wall, with a separate mesh strip above each goal.
- Added wallride_test.c: four wall climbs, bank/wall normals and orthonormal bases, upward momentum, normal jumps, gravity descent, top release, along-wall driving, corner-facet continuity (<1 unit/step), goal-side end continuity (<2 units/step), tilted ball hit and reset. Existing bank tests now expect tilted normal jump/closer wall contact. Stability bounds allow intentional high wall launches; the original isolated flat jump apex remains 2.01. Camera tests now project true rotated bounds and run a wall-climb/jump/fall sequence.
- Full strict host C99 checks pass: physics, banks, wall driving, CPU, jumping, sound events, sound stand-ins, camera and finite renderer output. CPU kickoff goal remains 5.53 seconds. Dedicated wall-jump sample peak 20.29; this includes vertical momentum from the climb. Software snapshots inspected for tilt and wall orientation. Kickoff 3,723 submitted triangles, bank close view 2,622, wall close view 2,610 (sample counts, not a maximum or hardware performance measurement).
- Hardware/target compilation pending for 0.11; no KOS compiler or emulator available here. Ask user to test transition feel, wheel contact, wall steering, jump-off behavior, descent, camera comfort, goal-side seams and FPS. 0.9 remains the accepted hardware size/performance baseline.
- Full-source package Vectorball_DC_Wallride_v0.11; build unchanged: make gdemu -> gdemu/01/disc.cdi. ZIP timestamps fixed in the past; no compiled artifacts.

## Previous test: 0.10 banks and rounded corners

User accepted 0.9 on hardware: field size and larger ball feel good; no reported frame-rate drops. Retain 120 × 160 / ball diameter 4.4 as the baseline. User approved the proposed bank/corner step.

- BANK_W 6, BANK_H 3, CORNER_R 14. Six facets per quarter circle, with straight side/end runs split around the goals (30 segments total).
- game.c exports arena_edge and arena_floor for rendering and tests. Collision plane starts/normals cached once to avoid repeated physics trig. Corner planes restricted to their corner quadrants; central goal mouths stay flat.
- Ball contact uses closest points on the bank cross-section (toe/slope/lip), normal impulses with existing .64 restitution, then existing .8 wall restitution/backstop containment. Goal fit/scoring checks retained. Bank collision rechecked after car hits.
- Grounded cars follow bank support height upright; jumping uses local support height, landing returns to it. Rounded walls constrain cars and CPU waypoints. Car-to-car separation rechecks boundary/support. No tilt, vertical wall driving, new acceleration constants or aerial moves. Upright wheel/body alignment on slopes is approximate.
- Rendering replaces rectangular perimeter wall boxes with shared curved wall and bank faces; caps close exposed bank ends beside goals. Stadium outer fences/terraces remain rectangular, outside the curved playing wall. Inner bank vertices use radial offsets, a small visual approximation to the faceted collision planes. Shadows remain simple floor silhouettes.
- Added bank_test.c: both side lifts, roll-back, all four diagonal rebounds, bank jump/support, four-corner car containment and both goal lanes. Existing physics/CPU/jump/audio/camera/render checks pass. Jump stability height bound now allows bank elevation plus original hop apex; stationary flat-ground apex check remains unchanged at 2.01.
- Reference snapshots inspected for side bank and curved corner. Initial kickoff geometry is about 4,000 submitted triangles (previous ~3,952); this is not a hardware FPS measurement.
- 0.10 target compilation and hardware performance remain unverified here; no KOS compiler or emulator available. Ask user to compare rebound feel, car bank traversal, corner/goal seams, camera framing and smoothness on hardware.
- Full source delivery folder Vectorball_DC_Banks_v0.10; build unchanged: make gdemu -> gdemu/01/disc.cdi. No compiled artifacts; ZIP timestamps fixed in the past.

## Previous test: 0.9 enlarged field and ball

User requested both proposed resizes after accepting 0.8 visuals/gameplay. This is a separate full-source test package: `Vectorball_DC_Large_Test_v0.9`; keep 0.8 for comparison. Historical sections below describe their respective milestones.

- Field 120 × 160, from 72 × 126; half-extents 60/80.
- Ball radius 2.2, from 1.65; diameter 4.4, roughly 1.07 times visible car length (~4.1). No claim of an exact Netick visual ratio.
- Goal 24 × 8, from 18 × 6.5; spawns ±38, from ±30.
- Ground clearance and CPU attack/jump thresholds expressed relative to BALL_R, preserving previous clearance offsets. Aerial ellipsoid vertical weight now derived from horizontal reach divided by BALL_R (previously fixed 4); car footprint unchanged.
- Ball shadow follows radius. Skyline spread follows arena bounds; building/floor/ball mesh counts unchanged. Existing adaptive camera already accounts for BALL_R; normal setback remains 11.5.
- Driving, steering, boost, jump, gravity, rebound/impulse strengths, audio and car models unchanged. No external source/assets imported.
- Host C99 warning-clean tests all pass: physics, CPU, jumping, sound events, sound API stand-ins, camera framing and finite renderer output. CPU kickoff score 5.53 seconds; hop apex 2.01. Reference views: default 3,952 triangles, corner 2,979, ballcam-normal 3,724, high-ball 3,779. These are samples, not maximum geometry counts or target performance measurements.
- No target compiler/emulator available here. 0.9 DreamSDK compilation and hardware play/FPS remain unverified. User accepted 0.8 as looking/playing well; no numerical FPS measurement supplied.
- Build process unchanged: `make gdemu` -> `gdemu/01/disc.cdi`. Full package includes ip.txt, all source, tests and docs; no compiled artifacts. ZIP timestamps fixed at 2020-01-01.

## Goal

An original polygonal car-and-ball game to sharpen Dreamcast development skills. Prioritize a satisfying one-car / one-ball / one-goal loop before expanding the game. This is a fresh independent project, not an edit to Harrier DC or Neo Cycles.

## Implemented

- Self-contained C / KallistiOS project; native PVR untextured opaque triangles.
- 48 by 84 unit checkerboard arena, low perimeter walls, colored goals, center ring.
- Procedural car with four block wheels, cabin and hood stripe; faceted ball.
- Right trigger throttle, left trigger brake/reverse, analog steering with a dead zone, boost with recharge, separate brake.
- Chase and ball-tracking cameras, smoothed camera position and wall bounds.
- Fixed 120 Hz physics with real elapsed time accumulator; catch-up capped to 100 ms per rendered frame.
- Simplified upright car with lateral grip. Circle footprint and a ball-height gate for car impacts.
- Gravity, ball floor rebound, rolling/air drag, wall rebound, impact lift, speed cap.
- Goal aperture checks for the entire ball, scoring on full crossing, two-second goal display and reset.
- Scores, boost bar, camera indicator, pause, practice reset, whole-match reset.
- Embedded 5x7 HUD font. No filesystem or assets required.

## Scope decisions

Solo practice is intentional for the first hardware milestone. There is no AI, multiplayer, aerial control, jumping, sloped banks, sound, match timer, animated wheels, or title screen yet. The initial plan's sloped edges are deferred; walls are vertical in this build. Cars cannot drive through the goal openings; only the ball can. The arena is open above and has no ceiling.

The renderer transforms vertices in software, clips triangles at the near plane, projects to 640x480, and sends reciprocal depth to PVR. Culling is disabled for this simple prototype. All geometry is opaque. The initial frame submits roughly 1,900 triangles including the bitmap HUD; this is an observation from the host, not a performance claim about Dreamcast.

Physics and rendering are separate from KOS so the core can be checked locally. Flat shadows are opaque floor patches. The car is boxy placeholder geometry, not a final model. Sine/cosine are used freely for this initial scope; profile on hardware before optimizing.

## Validation and limitations

Host C99 checks passed with warnings treated as errors. Tests verify the key scoring/collision paths and finite state over ten simulated minutes. Host reference rasterization of the actual triangle submissions was inspected. No Dreamcast compiler, emulator or hardware was available in the creation environment. A true target build and playtest remain mandatory. Do not describe this version as hardware-tested or distribute it as a verified working CDI.

Known simplifications: collision is discrete at 120 Hz; no swept collision solver; the collision footprint is circular despite the rectangular mesh; ball height collision is approximate; the ball does not visually rotate; camera view direction can change sharply during close ball tracking. Goal corners and extreme wall/ball/car overlaps deserve target playtesting. Extreme hitch time is discarded instead of trying to simulate all missed time.

## Next session

1. Build using the user's working KOS installation. Fix any toolchain/API compatibility errors from the exact log.
2. Test the CDI in redream and real hardware. Verify image, inputs, scoring, pause and reset.
3. Tune steering, grip, speed, ball weight and camera based on user feedback.
4. Add lightweight sound feedback and polish the car silhouette once the driving loop feels good.
5. Add one CPU opponent: approach a point behind the ball relative to its target goal, then commit to a shot; handle recovery and own-goal avoidance.
6. Add a match timer, then consider sloped edges, jumping and split-screen as separate milestones.

## Useful tuning locations

- Arena size / goal aperture / ball radius / physics step: `src/game.h`.
- Acceleration 20, boosted acceleration 34, top forward speed 25 / boost 37, steering rate 2.15: `src/game.c`.
- Ball gravity 18, bounce .64, impact multiplier 1.45, speed limit 52: `src/game.c`.
- Camera distance 13, height 8, projection scale 390: `src/render.c`.
- Rendering resolution 640x480, input mapping, frame cap: `src/main.c`.

## Build correction after first user compilation

User's DreamSDK / SH GCC 13.2.0 compiled main.c, game.c and render.c successfully. Linking failed because the original Makefile explicitly passed KOS_LDFLAGS to kos-cc, which already injects those flags, duplicating shlelf.xc. Removed the explicit flags from the wrapper invocation. Added an explicit compilation rule so project C99 / warning flags are actually used. Added `gdemu` as an alias for `cdi` to match the user's familiar command. The uploaded Harrier session-2 notes confirm `make clean` then `make gdemu`, but do not provide that project's image-generation recipe. This alias still uses mkdcdisc to generate a CDI; it does not claim to reconstruct that old workflow. Link and image generation still require user verification.

## DreamSDK disc workflow correction

The user confirmed all C files now compile and vectorball.elf links. Disc creation then failed because mkdcdisc is not installed. Their newly supplied GDEMU_CDI_BUILD.txt documents the working tools: elf2bin, scramble, makeip, and makedisc. Replaced mkdcdisc entirely with that workflow. elf2bin converts vectorball.elf into vectorball.bin; scramble writes gdemu/cd_root/1ST_READ.BIN; makeip uses the new project ip.txt to create gdemu/IP.BIN; makedisc takes that bootstrap explicitly and uses --audio. Final output: gdemu/01/disc.cdi. make gdemu and make cdi both run this workflow. make gdemu-clean removes the generated disc staging files. The earlier alias-to-mkdcdisc notes above are superseded. The compile/link step is user-verified; the new image recipe still needs actual DreamSDK execution and boot testing. Recipe sequencing and paths were checked using stand-in tools only.

## Prototype 0.2 — one CPU opponent

### Confirmed baseline

The user successfully built and booted prototype 0.1 using the DreamSDK GDEMU workflow. They confirmed the game plays, the player can hit the ball, and goals work. Do not change the working disc recipe. Hardware versus emulator was not specified in that confirmation; frame rate was not measured.

### Changes

- Orange CPU starts at z=+20 facing the blue goal; player remains blue at z=-20.
- Shared drive() and car_ball() helpers preserve the original player acceleration, steering, grip, boost, speed limits and impact parameters. CPU follows the same rules.
- CPU predicts the ball position 0.22 seconds ahead, chooses a staging point 6 units behind it relative to the blue goal, and circles around when on the wrong side.
- When behind and lined up within 3 units laterally and 11 units total, the CPU commits to a shot. A 0.65-second commitment avoids changing targets every moment of contact.
- Steering error controls braking and cornering speed. CPU uses its normal rechargeable boost on straight shot approaches.
- If CPU speed stays below 1.4 for a second, reverse recovery engages for 0.9 seconds. No teleport or extra steering force is used.
- Two circular car bodies separate and exchange impulse with equal mass. Wall clamping keeps both cars within bounds after bumps.
- Both cars and the AI timers/boost reset after goals and with Y. Existing score and pause controls remain.
- Added a post-impact side-wall clamp for the ball so wall-adjacent car impacts do not leave it outside the pitch.
- Orange car uses the same procedural mesh. An inspected host frame submits about 1,930 triangles; initial view about 1,940. This is geometry count, not hardware performance.

### Validation

Original solo physics regressions remain available by disabling the CPU in that test's setup. All passed. New CPU checks passed: scores against an idle player moved aside in 2.88 simulated seconds; wrong-side approach moves the ball toward the blue end without a blue own-goal in the tested 35-second setup; reversing escapes a side wall; opposed cars exchange momentum; exact overlapping cars separate without NaNs; resets clear CPU state; ten simulated minutes with both cars preserve finite positions, legal bounds and boost range. Inspected a host reference render with both cars near the ball. Host compile uses C99, -Wall -Wextra -Werror -pedantic.

Target build and playtest of this CPU update remain pending. These are deterministic scenario checks, not proof the CPU never makes a bad shot. The AI is a first-pass opponent: no dedicated goalkeeper, aerial play, difficulty menu, or sophisticated corner strategy. It can still miss shots or take a wide recovery route. Next step is user feedback on aggression, turning, collisions and ball tracking before adding features.

## Prototype 0.3 — upright jumps and airborne ball hits

User confirmed the 0.2 CPU match works and is fun. The accepted next step is jumping; sound/goal celebrations remain later work.

### Controls and physics

B now jumps; its old brake assignment is removed. Left trigger keeps the existing brake/reverse behavior. Input has a new jump field. The shared drive routine detects a press edge at the physics level so holding B cannot repeatedly jump on landing. Midair re-press does not create a second jump or queue one. Jump speed is 9.5, car gravity 22, giving a measured host apex of 2.01 units and roughly 0.86 seconds in the air. Cars remain upright, with no flip or double jump. In air, steering is 30% of normal, acceleration 25%, lateral damping .65 instead of 8, and forward drag .08. Ground driving constants remain unchanged. Vertical state uses each car's existing position.y and velocity.y.

### Ball, opponent and camera

Grounded ball impacts retain the previous contact rules. Airborne contacts use an upright ellipsoid with vertical normal/relative velocity, letting a jumping car hit higher balls and impart upward velocity. Cars with a base-height difference greater than 1.7 can pass over/under each other; closer cars retain the horizontal bump rule. They cannot stack or drive on one another. This is purpose-built arcade collision, not a rigid-body solver.

The CPU requests the same normal jump when an airborne ball is between 2.7 and 5.3 units high, within 7 units, with the CPU behind the shot and reasonably aligned. It receives the same press-edge and landing restrictions. Both cars' held-jump state and vertical motion reset with the existing reset flow. Pause/controller loss still freeze the physics through main.c.

Rendering adds car base height to the existing mesh; shadows remain on the floor. Camera height stays at 8 world units during a jump, with modest chase target elevation tracking, making the hop visible without dragging the camera upward. Ball camera continues tracking the ball.

### Verification

All previous solo and CPU checks passed. New tests passed for hop height, landing, held input, repress in the air, aerial contact on a ball out of ground reach, car-over-car clearance, CPU jump, state reset and ten simulated minutes of repeated jumps with both cars. The actual triangle submissions were rendered with the host software rasterizer and the jump view inspected. Target compilation/playtest for 0.3 remain pending. The working Makefile, ip.txt and DreamSDK audio/data CDI sequence are untouched.

Next: test B timing, braking with left trigger, jumping shots, CPU jump attempts and frame rate on user's setup. Tune jump/airborne contact from feedback. Impact sounds and a goal celebration are candidates after the jump feel is accepted. No new sound is implemented in this update.

## Prototype 0.4 — sound feedback

User confirmed the 0.3 jump update works really well and is super fun. Accepted next step: engine pitch, hit strength, jump/landing sounds and a goal cheer. Handling is deliberately preserved.

### Implementation

- New audio.c/audio.h integrate KOS snd_init, the SFX manager and AICA channel updates. Five reserved mono channels: player engine, ball thump, jump, landing and cheer. Explicit channels prevent one effect from stealing another. Allocation/load failure cleans up partial resources and continues silently.
- New sound_bank.c/sound_bank.h synthesize five original PCM16 samples once at startup: periodic motor hum, rubbery thump, rising jump whoosh, landing thud, and a short filtered-noise/voiced synthetic cheer with a bright chord. No recordings or external file loading. Memory: 58,336 sample bytes total, uploaded to sound RAM. Temporary synthesis buffers are freed after loading. All raw lengths are multiples of 32 bytes and under the SFX sample-count limit.
- The engine loop starts once; pitch and volume change with speed/throttle/boost. AICA UPDATE commands change frequency/volume at most 30 times per second, without restarting the sample every frame. Pitch smoothing uses the rendered-frame dt. The motor is intentionally quieter than effects. No CPU motor is played in this first pass.
- game.h has SoundEvents (hit/jump/land strengths and a goal flag). main.c clears events once per rendered frame before physics, then submits aggregated events after all fixed steps. Physics records events but does not call audio APIs. Short-frame catch-up therefore retains the strongest hit instead of losing or replaying every substep.
- Ball hits record closing speed; substantial floor/wall rebounds also report a reduced strength. SFX hit cooldown is 70 ms. Hit volume uses strength and distance from the player. Jump/landing strengths are lower for CPU. Goal stops the engine and plays one cheer; the loop returns after field reset.
- Pause/controller loss stops the owned channels. Resuming restarts only the motor. Y reset stops effects and clears timers. No pause replay or stale-event effects. Sound samples stay allocated across matches/resets.
- Makefile adds audio.o and sound_bank.o plus header dependencies. The previously working elf2bin -> scramble -> makeip -> makedisc --audio disc recipe is retained. No assets or new packages are required.

### Verification and pending target work

All solo, CPU, jump and render checks passed. Added actual physics event tests and a host sound backend with API stand-ins. Checks cover raw size/sample limits, amplitude headroom, channel ownership, no per-frame engine restart, pitch increasing with speed, stronger hit volume, hit cooldown, jump/landing cues, mute/resume and single goal/reset playback. Stand-ins validate our logic, not compatibility with the installed KOS or audible hardware output.

Compared 72,000 fixed physics steps against delivered 0.3 using identical scripted steering, throttle, braking, boost and jumps. Every gameplay field matched byte-for-byte at every step (aggregate hash 9c481dda); new event state excluded from the comparison. No handling constants or AI decisions changed.

Audio listening, actual DreamSDK compilation, AICA playback and frame-rate checks remain pending. No measured audio quality or hardware performance claim. Next test: idle engine versus acceleration/boost, soft versus hard ball hits, jumping/landing, both goal cues, pause/resume and resetting while a cheer plays. Tune volume/pitch from user feedback before adding music. Playback gains/engine range are in audio.c; synthesis timbre and sample lengths in sound_bank.c.

## Prototype 0.5 — original sports hatchback visual pass

### Confirmed baseline and references

User confirmed everything works as it should in 0.4, and sound effects are good. Supplied six screenshots of a Rocket League-like GTA San Andreas mod. Agreed first visual step: replace the placeholder cars before upgrading the stadium. References informed sporty hatchback proportions, dark glass, wheel arches, round wheels, spoiler and team paint. The new mesh is original procedural geometry; no mod or Rocket League assets are imported. Screenshots do not establish that mod's rendering/hardware requirements.

### Changed

Only src/render.c changes production behavior, and only visually. Both teams use a shared low-poly hatchback mesh with a bevelled front/rear, sloped hood, shoulder panels with actual wheel cutouts, arch lips, sloped windshield and rear hatch, side glass with pillars, mirrors, handles, grille, splitters, exhaust tips, license-plate patch and a raised spoiler. Blue paint is now a more saturated blue; CPU stays orange. Tires are ten-sided cylinders with contrasting rim sectors/hubs. They are modelled but not rolling/steering animated yet. Headlights and rear lights are opaque colored geometry, not dynamic lighting or transparent glow. Existing ground shadows are retained.

Player/cpu mesh transformation still follows position, heading and jump height. Per-car sin/cos is cached once at draw time; wheel and arch circle samples are constants. No new assets, dependencies, physics fields or disc-copy steps. Current model stays close to the prior visual footprint; the existing collision body is unchanged.

### Validation

Inspected host software-rasterized front three-quarter, rear three-quarter, side and normal chase views. Refined rear glass placement to avoid burying its top edge in the hatch panel. Added optional `front`, `rear`, `side` views to the existing host snapshot helper for future inspection. The helper cameras are not player-accessible features.

Existing physics, CPU, jump, sound-event/audio-backend checks passed with C99 and warnings treated as errors. Byte-compared game.c, game.h, main.c, audio.c/audio.h and sound_bank.c/sound_bank.h with the delivered working 0.4 package; all identical. No handling/input/audio change. Initial scene count: about 2,934 submitted triangles versus 1,942 previously, with both cars included. Rendering near clipping still works in the tested corner view.

Target compilation, PVR appearance and performance of the new mesh remain pending. Host snapshots are not Dreamcast screenshots. Next user test: assess front/rear/side silhouette while turning and jumping, CPU color/readability, and frame-rate smoothness. Next planned visual milestone is a nighttime city stadium: pitch markings, stronger goal areas, skyline and perimeter treatment, while retaining the accepted gameplay. Boost flames and ball detailing remain later candidates.

## Prototype 0.6 — adaptive ball-camera framing

### Hardware baseline and diagnosis

User tested 0.5 on real hardware: no FPS issues, plays great. Requested simulation of the slight zoom/distance adjustment they observed in a Rocket League-like ball camera, aiming to keep their car framed. The claim about Rocket League's precise implementation is not verified; this is a purpose-built solution for Vectorball.

Reproduced the defect with the existing camera: constant 13-unit setback, look-at-ball orientation and inside-wall clamping. A car at the origin with ball at (0,20,8) projected its conservative bounds to screen Y 740..1117 (480-line output), entirely offscreen. An overhead ball could place some car corners behind the eye. With car near z=-40, the camera compressed to about one unit of horizontal setback and lost the car below the frame.

### Camera changes

Camera struct now stores projection focal length, orbit yaw, smoothed distance/height, prior car position, initialization and mode. main.c already depends on render.h in the working Makefile, so the changed header rebuilds both main and render.

Ball camera blends normalized directions toward the car center and ball (52% ball influence). Normal distance starts at 13. Elevated/close balls increase distance; setback and height are eased. The camera remains ground anchored during jumps. Ball mode stays three units inside perimeter bounds and raises the eye when a wall limits setback. Normal projection scale is 390, with adaptive widening based on conservative projected car/ball boxes. Ordinary zoom-out/in is eased (returning inward is slower); a looser car border provides immediate safety correction during rapid contacts. Extreme cases can widen much further than the ordinary small adjustment.

Orbit yaw eases toward the ball and is limited to 3.5 radians/sec. Within three horizontal units it retains its prior bearing, avoiding a heading fallback when the ball crosses the car. When ball and car point in widely opposing directions from the eye, prioritize the car during orbit catch-up; ball may temporarily leave view. Orientation is smoothed, with a safe fallback if the car would go behind the camera. Large car teleports reset camera state. Chase mode preserves the accepted prior position/target calculation and fixed projection; large reset jumps now snap safely.

No physics, CPU, controls, meshes or audio changed. X remains the mode toggle. No wall geometry is hidden or removed.

### Simulation/visual verification

Added camera_test.c: bounds/depth and basis checks for normal/high/overhead/behind/side ball positions, all four corners, a 60-second scripted moving-target sequence with jumping car and overhead passes, a two-minute actual physics match with goals/resets, and toggling modes. Bounds checks include all eight corners of a conservative rotated car box, screen borders/HUD clearance, finite focal length and orbit-rate limit. All pass. Existing physics, CPU, jump, audio and render checks also pass.

Inspected before/after host reference frames for normal, high, overhead and back-wall situations. Included CAMERA_COMPARISON.png inside this update ZIP; these are software reference renders, not hardware screenshots. The high-ball static case now frames the car at about Y341..396. The overhead case about Y361..392. These measured positions describe selected test cases, not a guarantee for all gameplay/camera sequences. Boundary foreground geometry can still occupy the edge of the view.

Next: user playtest in BALL CAM (X), especially high balls, reversing under the ball, wall/corner scrambles, jump hits, camera toggles and goal resets. Tune zoom/orbit/height timing from actual feel. No hardware performance or comfort result is claimed yet for 0.6. Stadium visual upgrade remains planned after this camera refinement.

## Physics reference review — Netick Rocket Cars

### Current acceptance and user preference

User confirmed the 0.6 ball-camera refinement works really well. Once the arena is bigger, bring the ball-camera distance in a little and re-test framing. Do not change the accepted camera or physics during this reference review. 0.5 was explicitly tested on real hardware with no reported FPS issues; 0.6's successful test platform was not specified.

Repository: https://github.com/NetickNetworking/NetickRocketCars
Repository HEAD reported during review: 907ec50519d61920eefbbd34897aecf2e81e5dc1.
Reviewed actual CarController.cs (569 lines), CarWheel.cs (18 lines), Ball.cs (57 lines), and CarCameraController.cs (249 lines). Source files were fetched read-only from main; the camera file needed a Windows-1252 decoding fallback. No reference source was copied into the game and no physics changes were made.

### Useful code findings

CarController performs per-wheel ground raycasts, applies motor/grip forces at contact positions, and adjusts steering using speed and an editable curve. It computes sideways slip from lateral versus forward speed and uses a grip curve with a minimum. Drift reduces grip on selected wheels; opposing throttle adds braking. Boost uses different ground/air strength. A timed second jump can produce directional air boost/rotation, with separate pitch/yaw/roll control and self-righting assistance. Drag uses exponential decay. Wheel steering/spin are animated, and body suspension uses a separate damped visual spring.

The public numeric defaults are not complete tuning: serialized prefab/scene fields and curve assets may override them. Do not transplant force values directly into our C game or assume matching scale/mass/timestep.

Ball.cs adds configurable gravity, tracks the last touching player and scales impact audio by relative speed. Collision solving is supplied by Unity rigidbody physics; this script does not contain a portable sphere-versus-car solver. Ball material/collider tuning was not inspected. Adapt behavior in our existing solver rather than describe this as a drop-in ball-physics port.

CarCameraController includes smoothed car/ball modes and speed-driven field-of-view changes. Its ball-follow bearing uses a flattened horizontal ball direction. Our accepted camera already addresses our particular vertical/wall framing failures; the reference does not replace the need for our own framing checks.

### Proposed adaptation order (not implemented)

1. Enlarge and dress the arena as planned, then slightly reduce normal ball-camera setback and re-run corner/high-ball checks.
2. Add speed-dependent steering and optional slip-dependent grip as small independently testable tuning changes. Retain the current accepted feel as the baseline.
3. Animate wheel rolling/steering and add modest visual body lean/landing suspension. These can improve responsiveness without replacing physical handling.
4. Treat banked-wall driving, air roll, directional dodges and full self-righting as later systems: they require real surface contact/orientation work beyond our upright-car approximation.

Netick/Unity/PhysX/network prediction are not a Dreamcast build dependency. Suitable ideas must be re-expressed in our C simulation and evaluated on hardware. No Dreamcast frame-time estimate was established for these proposed systems.

### Licensing and sources

The repository's code license is MIT, copyright 2026 Karrar; preserve its notice if code or substantial portions are adapted. The README lists third-party art/audio under separate terms. None of those assets were imported. This review makes no changes to our original car models or sound bank.

- Vehicle and visual suspension: https://github.com/NetickNetworking/NetickRocketCars/blob/main/Assets/Rocket%20Cars/Scripts/Player/Car/CarController.cs
- Wheel data: https://github.com/NetickNetworking/NetickRocketCars/blob/main/Assets/Rocket%20Cars/Scripts/Player/Car/CarWheel.cs
- Ball: https://github.com/NetickNetworking/NetickRocketCars/blob/main/Assets/Rocket%20Cars/Scripts/World/Ball.cs
- Camera: https://github.com/NetickNetworking/NetickRocketCars/blob/main/Assets/Rocket%20Cars/Scripts/Player/Car/CarCameraController.cs
- License: https://github.com/NetickNetworking/NetickRocketCars/blob/main/LICENSE

## Prototype 0.7 — larger arena and closer ball camera

User authorized enlarging the arena and bringing the accepted ball camera closer after the Netick reference review. This update does not import or adapt Netick source and does not yet add the planned nighttime stadium scenery or replace the handling model.

### Dimensions and layout

Arena half extents increase from 24/42 to 36/63: full pitch 72 x 126 rather than 48 x 84. Both dimensions increase 50%, area increases 125% (2.25x original). Goal half-width increases 7 -> 9 (full 14 -> 18); height remains 6.5. Ball/car scale and collision bodies are unchanged. Kickoff spawns move from z=+/-20 to +/-30, configured by SPAWN_Z. Goal triggers and all collision limits already reference arena constants. Rendering now does too: floor, side/end walls, goal frame/back/side panels, sidelines and center line are calculated from ARENA_X/ARENA_Z/GOAL_W instead of old numeric limits.

Fixed 8 x 14 floor tile count is retained: tiles are now nine units wide/long instead of six. A wider field therefore does not multiply floor geometry. Center circle radius increases 8 -> 10. Added lightweight goal-area outlines (14-unit half-width, 12-unit depth at current settings) for distance cues. Initial host chase reference submits approximately 2,942 triangles versus 2,934 previously. Counts depend on camera/near clipping, and are not hardware frame-time measurements.

CPU setup distance increases 6 -> 7, circle-around side offset 8 -> 9; both named in game.h. Target-goal geometry and boundary clamping use the enlarged arena dimensions. AI acceleration, boost, turn speed, attack/jump rules and recovery timer stay unchanged. Other physics constants are unchanged. drive(), aerial_car_ball() and car_ball() were byte-compared with the working source and match exactly.

### Camera

BALL_CAM_DISTANCE in render.h changes normal ball-camera setback 13 -> 11.5 (about 11.5% closer). Initialization, mode switch and adaptive distance formula use that constant. Height, orbit rate, smoothing, automatic widening and conservative car-frame safeguards retain their accepted behavior. Chase camera keeps its 13-unit distance and fixed projection. X remains the camera toggle. No controls/audio/model changes.

### Validation

Existing host checks pass: solo physics, goals/posts/crossbar, CPU scoring/recovery/bumps, jumps/landings, sound events/backend logic, and simulated ten-minute matches. Added checks that the former end line z=43 and side limit x=25 are playable interior (no phantom old boundary), and that a ball inside the new wider mouth scores. CPU can score against an idle player moved aside in 4.35 simulated seconds, versus 2.88 in the original layout. This is one deterministic kickoff scenario, not an overall difficulty metric.

Camera tests now use actual enlarged corners and moving-target paths spanning the larger pitch; all car-framing checks pass, including the two-minute physics match with jumps/goals and mode toggle. A static center-field test confirms the closer normal setback. Inspected host renders of new kickoff, ball cam, back wall and corner. Near-goal foreground geometry can still occupy the lower edge as previously noted.

Target compilation, pacing, camera feel and hardware FPS for 0.7 remain pending. Next user test: is the 1v1 field too empty or the right amount of breathing room, do wider goals still feel fair, is 11.5 close enough for ball cam, and is real hardware still smooth? Adjust scale/camera in small increments before changing steering/grip. Then proceed with the nighttime city-stadium visual pass if the layout is accepted.

## Prototype 0.8 — nighttime city stadium

### Acceptance and scope

User accepted 0.7 larger arena/closer camera: feels a lot better. Authorized the next visual pass: city skyline, floodlights, perimeter fencing and stronger team-goal colors. This update preserves the accepted arena/camera/gameplay and introduces no Netick code/assets.

### Visual implementation

Only src/render.c changes production behavior. Added navy sky bands at a depth behind all world geometry, 32 fixed stars and a 16-triangle moon. Original city blocks surround all sides, with sparse warm/cool windows, some stepped roofs and two landmark spires. A dark outer plaza supports the skyline/stands visually. Stepped spectator terraces sit outside collision walls. Open perimeter wire strips sit above the side walls; end fencing begins above goal crossbars so it never bars a scoring opening. Six pole-mounted floodlight banks have 48 bright lamp faces. Existing floor tile count is retained, with greener/lighter colors. Blue/orange goal backs, extra surround strips and slightly raised floor paint distinguish ends.

Everything uses opaque untextured polygons in the existing PVR pass. Lamps/windows/moon are colored faces, not real-time light sources. No transparency, light volumes, dynamic shadows, texture transfers, external assets, new sound or new build dependencies. New structures are decorative and outside the existing playing boundaries. The floor paint is at y=.01 below markings at y=.02 to avoid covering lines. Sky details have fixed deterministic placement, with no per-frame random reseeding.

### Checks

Inspected host software renders of kickoff, normal/high ball camera and a corner. All existing physics, AI, jump, audio and camera checks passed on the final source with C99, -Wall -Wextra -Werror -pedantic. A sweep of 600 ball-camera angle samples around the full accepted pitch and high balls submitted 2,580–4,356 triangles, all with finite projected coordinates. Kickoff is about 3,953 versus 2,942 previously. These counts are not measured Dreamcast FPS. Target compiler, PVR output, sound coexistence and performance still need the user's test.

Verified production game.c/game.h/render.h byte-for-byte with the delivered 0.7 patch. Also compared camera_update and car-model routines with 0.7: identical. No changes to physics, controls, ball camera, dimensions, car scale, audio or AI. Working Makefile/ip.txt/disc workflow remain in place. Patch only needs src/render.c replaced and rebuilt.

Next user test: assess the nighttime scene while driving and jumping, whether buildings/fences or lights distract from the ball, both goal mouths, corner views, and hardware FPS during high-ball pullback. Tune visual density from measured/observed hardware results before adding boost effects, wheel animations, or further physics changes.
