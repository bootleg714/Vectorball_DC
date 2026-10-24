# Resume here — neon vector arena (single screen), October 23, 2026

User asked to spice up the generic test arena and chose a "neon vector arena" look. Mid-task they added: **leave the versus (split-screen) arena as is — it runs 34–39 FPS on hardware.** So every change is single-screen only (`viewport_height==480`); split views take the exact previous code paths and colours.

Render-only change in `src/render.c`; no gameplay, physics, collision, camera or HUD changes. Still one opaque PVR list, flat colours, no textures or translucency. Glow is faked with a bright thin core strip over a wider dim halo strip.

- Pitch: dark team-tinted halves (navy at the blue end, -z; plum at the orange end, +z) on the same 8x14 tiles.
- Glowing vector grid: 11 long lines (blue/orange per half) and 13 cross lines, core plus halo.
- Midfield line and centre emblem: white core over a purple halo, a glowing ring, a cyan hexagon and two team chevrons pointing at the goal each team attacks.
- Banks: dark steel, lighter towards the lip. A neon toe line where pitch meets bank, a neon band on the wall above the lip and a neon top rail. Long edges are split so the colour runs blue -> magenta -> orange along the arena (`neon_at(z)`). Fence wires recoloured.
- Goals: darker team-tinted nets with three glowing slats, team-coloured goal-area outlines, and a big neon frame with three stacked chevrons on the end wall just outside the playing surface (z = ±(ARENA_Z+.45), behind the bank).
- Sky and city: purple horizon gradient, a striped synthwave sun low behind the orange end (replaces the moon), darker buildings with magenta/cyan windows, neon lips on the terraces, team-coloured spire tops. Car/ball floor shadows darker to suit the dark pitch.

Host evidence (no Dreamcast compiler/emulator here; no hardware FPS claim):
- Split screen unchanged: a 400-frame versus hash fixture (8 car/ball/camera layouts, pause overlay, FPS label) submits 1,341,200 triangles with an identical FNV-1a hash of every vertex and colour on the old (4ad39a2) and new render.c.
- Full strict `tests/run.sh` passes (33 PASS lines, same as before). Single-screen kickoff snapshot 5016 -> 5649 triangles (+633, ~13%); corner snapshot 2534 -> 2654. Other single views: centre 3695 -> 4202, side 3634 -> 4162, goal 3155 -> 3448.
- `NEON_ARENA_PREVIEW.png`: host software renders (kickoff, overview, goal, centre). Host rasteriser aliasing makes thin lines look dashed at a distance; hardware should draw them solid.

Hardware test: single-player/CPU matches on GDEMU/Redream — check the look, readability of the ball and cars against the dark pitch, that no strips flicker against the floor (they sit .018–.06 above it), and single-screen FPS. Versus should look and run exactly as before.

---

# Resume here — v0.32 wheel contact during rollback candidate, October 23, 2026

User clarified v0.31 “kinda worked” but the car falls off at the top; wants skateboard-like movement with wheels staying on the wall while sliding down, even with gas floored. v0.30 remains accepted hardware baseline. v0.31 was a partially successful trial, not accepted for contact feel. v0.32 is host-tested and awaits their real hardware test. No target compiler/emulator here and no ELF/CDI supplied.

Build fresh Vectorball_DC_Wall_Slide_v0.32: make clean, make gdemu ->gdemu/01/disc.cdi. Same Makefile/PVR/dependencies, Input and Game layout (Game844host bytes, unchanged from v0.31).

## Fix: separate support from driving traction

v0.31 multiplied wall adhesion, body alignment, angular damping and steering torque by the same fading grip as tire drive. At a slip value.10 this left weak contact support, allowing a tumble/free fall. Restore full wall-contact support independently of tangential grip:

- Wheel longitudinal/lateral forces still use surface_grip and slip cap.10, preserving the upper fade/anti-hover cutoff.
- Average support is1 for actual non-ceiling wheel contacts,0 for ceiling normals.y<-.5. Adhesion10, angular damping10, normal alignment80, yaw tracking and manual wall steering use support instead of fading grip.
- Keep existing probes/springs/collision integration. No pose snaps, artificial position locks or attachment without wheel contact. A real goal opening can naturally lose contact. B jump zeroes contact/forces and starts cooldown as before; full adhesion does not hold a deliberate jump in place.
- Keep positive upward motor disabled while upper_wall_sliding. Move slip reset from30%arena height (~14.81) to chassis center<BANK_H+1 (4units). This keeps the engine from restarting an ascent halfway down while gas is floored; ordinary drive returns at the lower bank. No new state fields.

Fade start/end and high-crest latch thresholds stay as v0.31 (40->78%H, latch above70%H or above45%H with vy<=.25). Positive upward air thrust remains suppressed while latched/near roof, avoiding ceiling pinning. Ball3.4375radius, aiming bias, speed caps, hit gains, boost usage, jumps/rolls/flips/recovery, lower wall/flat handling and matches unchanged. Straight head-on rollback can be backward with nose still up; angled carving/steering remains available.

## Verification: contact retention, not merely descent

Expanded upper_wall_test from64 to96fixtures: both humans, X/Z walls and both sides, normal/full boost, high uphill rest, high sideways run, max-speed upward near-roof approach, initial wall climb from12units, plus natural straight/angled approaches from the flat pitch at20units/s. Fuel replenished every physics tick so decline is not caused by tank depletion.

During latched sliding, chassis center>BANK_H+3 (6units), actual solid wall, and after first6settling ticks: require>=2wheels, wall-normal alignment>.95 and wall gap<.5. Exclude the actual goal-opening/crossbar transition (end wall, |x|<GOAL_W+1.5, height<GOAL_H+2): there is no solid wall to hold onto there. Measured28,316retained-contact ticks, minimum alignment rounds.995, max wall gap.137units. Every fixture returns to center<4units with latch cleared within12seconds; worst5.71s. Finite positions, <1unit displacement/tick and max car speed checks retained.

Regression demonstration: run the stricter contact check against preserved v0.31; it fails on boosted sideways wall descent at height24.44, tick170, one wheel contact, alignment.973 and gap.400. The revised support model passes. This catches the reported fall-away behavior that the old descent-only checks missed.

Both ceiling/body-contact setups still drop with sustained boost. New high-wall jump/second-flip fixtures for both humans verify intentional B detaches with >8units/s wall-away velocity, earns one air charge and permits the next flip. Reset clears both slip flags. Full strict portable suite passed, including all prior wall/carving/steering/recovery/tilt/ball/aiming/controller/split/match/audio/camera checks. ASan/UBSan upper-wall fixtures passed with detect_leaks=0 (LeakSanitizer task inspection unsupported in runner).

Repeat optional lower_wall_compare against v0.30, compiling current with -DUPPER_WALL to exclude new int.13fixtures*1200ticks=15,600ticks,13,041,600legacy bytes match exactly. SHA256 a06cdba75890e4082d76c9ff0010bbb868d72667657284431952be2239a64a6d. This is evidence for those host trajectories, not every input/compiler. Full results in SIMULATION_RESULTS.txt. No new hardware FPS claim.

## Preview and next test

Updated wall_grip_preview.c: blue starts on flat pitch15units from side wall,20units/s toward it; orange starts at80%H with wheel side facing opposite wall. Both hold throttle/boost with normal fuel depletion. Production game/match/split rendering, host chassis-tracking inspection cameras.8seconds640x48020fps offline encode; badge dashes. New WALL_SLIDE_PREVIEW.png visually checked. Other images/audio/render comparison are historical references. Runtime camera code unchanged.

Hardware test: natural straight/angled ascent, full throttle/boost held through crest and rollback, wheels stay on solid wall, steering/brake/reverse on descent, jumping/flipping off the wall, bank exit/landings/recovery, both humans/cameras and versus FPS. User's feel judgement remains required. Separate P2 engine voice is still pending.

---

# Resume here — v0.31 upper-wall grip candidate, October 23, 2026

**Accepted hardware baseline: v0.30.** User reported everything looks good on real hardware and accepted proposed upper-wall traction fade/ceiling-hanging fix. v0.31 is host-tested, awaiting their hardware feel/performance feedback. No target compiler/emulator here and no ELF/CDI supplied. Preserve v0.30 for comparison.

Build fresh `Vectorball_DC_Wall_Grip_v0.31`: make clean, make gdemu ->gdemu/01/disc.cdi. Same Makefile/object list/dependencies and PVR configuration. CarSurface appends one upper_wall_sliding int; Game836->844host bytes. No Input changes. Existing memset reset clears the new state for both cars.

## Implementation

vehicle.c surface_grip(height,normal): full grip below.40*ARENA_HEIGHT (~19.75units) and on upward-facing normals>=.85. For higher steep contacts, t=clamp((height/H-.40)/.38,0,1), steep=clamp((.85-normal.y)/.50,0,1), grip=1-steep*t*t*(3-2*t). Full fade at.78H (~38.51). Ceiling normals.y<-.5 return0 at all heights.

Per wheel multiply longitudinal force and lateral tire force by contact grip. Keep suspension/collision spring unchanged. Average contacted-wheel grip scales wheel-side adhesion, angular damping, normal-alignment/yaw assist and manual wall steering motor. Lower surfaces have factor1; collision response/probes/geometry remain active everywhere. No pose snapping or nonphysical position reset.

A fade alone can leave an equilibrium where uphill motor balances gravity. New slip latch prevents that: on a steep wheel contact (normal.y<.7), latch when chassis center>.70H (~34.56), or center>.45H (~22.22) and vertical speed<=.25 (high crest/descending contact). While latched, steep-wheel grip is capped.10 and any motor component that would drive upward is disabled; downhill/horizontal motor remains available with reduced grip. Keep this state until chassis center<.30H (~14.81), including through loss of contact. This lets gravity bring it lower before full grip returns, rather than relatching/hovering at one height. Continuous throttle can begin another climb after the low reset. CPU/human/P2 all use shared solver.

Airborne thrust keeps its horizontal component. Positive world-up component is zero while slipping; otherwise fades over roof clearance8->3units (ceiling-center.y), so a car cannot pin itself at the roof by losing wheel contact and immediately replacing uphill tire drive with air thrust. Ordinary low-altitude air control remains factor1. Jump impulses, flip/roll/recovery motors, ball3.4375radius/aiming/caps, acceleration, boost drain/refill, steering laws below fade and match flow unchanged. New per-contact math/state only; no new render geometry, allocations or sound channels.

## Host evidence

Full strict suite passed. New upper_wall_test includes production vehicle.c/helper and links game.c. Helper sampled through height for monotonic fade, full lower/floor grip and zero ceiling grip.64dynamic fixtures: both human slots, X/Z walls and both sides, normal/full boost, high resting uphill, high sideways, max-speed upward near-roof entry, and a lower-wall uphill approach. Boost reset full each tick, so descent cannot be attributed to empty fuel. Every fixture enters slip and returns with latch cleared below30% center height within12seconds; worst4.36seconds. Finite positions, <1unit steps and car speed cap retained. Two upside-down ceiling-wheel fixtures drop >5units while holding full boost/throttle. Reset clears both latches.

Optional tests/lower_wall_compare.c compiles against preserved v0.30 and v0.31 (-DUPPER_WALL for new) to strip the appended int and compare all836legacy bytes each tick.13fixtures: one recentered flat drive/jump/roll run and12lower-wall coast/steering runs starting6/11/16units high. All centers checked below40%H throughout.1,200ticks each=15,600records,13,041,600bytes. Files byte-identical. SHA256 both: a06cdba75890e4082d76c9ff0010bbb868d72667657284431952be2239a64a6d. This covers the tested host trajectories, not every lower-wall interaction/compiler/platform.

All accepted aiming/ball-size/flip/controller-input/double-jump/recovery/tilt/wall/carving/corner/CPU/split/match/menu/audio/camera/renderer tests passed; default render snapshots5016/2534triangles. ASan/UBSan upper-wall fixtures passed with detect_leaks=0 (LeakSanitizer task inspection unsupported in runner). Complete output in SIMULATION_RESULTS.txt. Last numeric versus34–39FPS feedback belongs to v0.26; user accepted v0.30 hardware without another number. No v0.31 hardware FPS claim.

## Preview and next test

New tests/wall_grip_preview.c uses production two-input simulation and split renderer: blue begins at12units/upward10units/s, orange at80%H/rest, both wall-oriented and holding throttle/boost with ordinary fuel depletion.8second640x48020fps offline encode; FPS badge dashes. Host inspection cameras follow chassis centers so falling/rotating cars stay visible; these are not production chase-camera recordings. New WALL_GRIP_PREVIEW.png visually checked. Other PNGs/audio/render comparison are historical references.

Hardware focus: straight/angled/sideways high climbs under throttle/boost, no top/ceiling hanging, lower-wall carving retained, steering/brake/reverse during slide, upper-wall jumps/flips/landings and roof recovery, both humans/cameras and versusFPS. Falling may leave a rolled pose; accepted B recovery remains available. Tune fade start/end/crest thresholds after real feedback. Upper-wall work is now a candidate implementation; separate P2 engine audio remains pending.

---

# Resume here — v0.30 ball / aiming candidate, October 23, 2026

**Accepted hardware baseline: v0.29.** User confirmed “it works now and works great on real hardware.” Requested another25% ball-size increase and finer/more accurate front-hit angles. v0.30 is host-tested and awaits their hardware feel/performance test. No target compiler/emulator here, no ELF/CDI supplied.

Build fresh `Vectorball_DC_Ball_Aiming_v0.30`: make clean, then make gdemu -> gdemu/01/disc.cdi. Same Makefile, dependencies, input layout, physics state layout, renderer meshes/PVR/audio setup. Last numeric versus FPS34–39 report was for v0.26; user accepted v0.29 on hardware without a new number.

## Ball size

BALL_R2.75->3.4375 (diameter5.5->6.875) is25% above current v0.29,56.25% above original2.2radius. RL_SCALE stays fixed at2.2/91.25; no arena/goal/world scaling. Radius is shared by mesh, shadow, visibility/framing, sparks, chassis contact, banks, walls/floor/ceiling and goal clearance. Ball subdivisions unchanged; visible facets are unrelated to continuous collision directions. Speed/acceleration/hit strength/gravity and timing unchanged from accepted v0.29.

## Aiming: continuous frontal direction tuning

Existing grounded circular car/ball solver already produces arbitrary float-valued contact directions, not a fixed set of bounce angles. Increasing visible ball polygons would not improve physics accuracy. User-facing explanation given; implement a mild frontal aiming response to reduce sensitivity of small offsets while retaining smooth wide-shot control.

In grounded car_ball only: after physical contact normal nx/nz and positive relative closing speed are calculated, compute forward=(sin heading,cos heading), front=dot(normal,forward), t=clamp((front-.35)/.65,0,1), blend=.35*t*t*(3-2*t). Impulse direction=normalize(normal*(1-blend)+forward*blend). Forward component means car heading, not target/goal steering: no automatic target selection. Fade reaches zero around69.5degrees from nose, smooth at both endpoints. A5degree stationary-ball contact becomes3.25degree shot; center remains straight. Wider offsets still vary continuously. Applies to human/CPU grounded frontal contacts.

Use original physical contact normal for overlap correction, relative closing speed, feedback, upward lift and car recoil. Horizontal impulse magnitude remains rel*BALL_HIT_GAIN; only its direction changes. Incoming ball velocity remains additive, then existing total cap applies. Side/rear response exact previous formula (blend0). Aerial and flip response untouched; flip15% bonus/rotating swat remains accepted. All jump/roll/recovery/input/boost/steering/match behavior retained. New extra math runs only during a closing ground contact, no per-frame allocation or extra rendering geometry.

## Verification

Full strict host suite passed. New aiming_test includes production game.c for isolated contacts:8sweeps across both car slots/four headings,1,121samples each from-70 to+70degrees in.125degree steps (8,968shot samples total). Confirms strictly monotonic distinct directions per sweep, no snapping/dead zones, normalized impulse strength unchanged, mirrored left/right, centered shot, side/rear equivalence and5->3.25degree central response. The fixtures isolate stationary ball/forward-moving car contacts, not complete match accuracy or goal success rates.

scale_speed_test updated to3.4375radius and4.75unit contact beyond previous4.40reach;3unit post clearance that fitted previous2.75radius now blocks larger ball. Full tests confirm unchanged field/caps/gravity and ceiling/max-speed shots. physics_test's old car.z=-5 fixture initially overlapped the new5.0875contact reach, eliminating its acceleration run-up. Corrected fixture to -(BALL_R+1.65+1), retaining1unit gap; tracks any upward loft during60ticks instead of a radius-sensitive single endpoint. No production loft adjustment.

Existing flips, controller vertical-stick path (both humans,120/30/15fps sampling), double-jump gates,360recoveries, tilt/wall/carving/corner20second settling/stress, CPU,10minute two-human run, matches/menus/audio/cameras/renderer all pass. ASan/UBSan aiming and scale/size fixtures pass with detect_leaks=0 (LeakSanitizer task inspection unsupported in runner). Default renderer snapshots still5016/2534triangles. Full logs in SIMULATION_RESULTS.txt. No new target FPS measurement.

New tests/aiming_preview.c produces three actual single-view ground shots: left1.35offset, center, right1.35offset. Each2.5seconds, production match/game/renderer/ball camera. New SIMULATION_PREVIEW.mp4 is7.5seconds,640x480,20fps offline encode; does not represent measured game FPS. BALL_PREVIEW.png extracted at3.1s and visually checked. Other PNGs/audio/render benchmark remain historical references.

Next hardware focus: new ball size, centered/small-offset/wider/moving-ball shots, boosted shots, flip swats, bank/goal clearance, ball camera and versus FPS. Need user's feel judgement: reduce/increase frontal bias if wide aiming becomes too difficult. Pending upper-wall traction/ceiling hanging and separate P2 engine remain outside this change.

---

# Resume here — v0.29 controller pitch / larger ball candidate, October 23, 2026

User accepted v0.28 faster speeds and stronger ball hits, but reported forward/back flips did nothing while barrel rolls still worked. Requested ball25% bigger. Preserve accepted pace/hit response and first-jump roll. v0.29 fixes missing vertical-stick decoding and enlarges the shared ball radius. Host-tested; hardware confirmation still required. No target compiler/emulator here, no ELF/CDI supplied.

Build fresh `Vectorball_DC_Flip_Input_Ball_v0.29` with `make clean` then `make gdemu`; load that folder's gdemu/01/disc.cdi. Input layout changed; clean avoids stale object ABI/build issues. Existing Makefile/dependencies/PVR configuration remain. Last user-reported versus34–39FPS belongs to v0.26.

## What was found and changed

main.c controller_input read joyx for steering and ignored joyy during gameplay (only menus used it). v0.28 chose pitch direction from trigger/reverse, so the user's forward/back stick input never reached the mechanic. Earlier tests synthesized Input directly and missed this path. The complete no-front-flip symptom was not reproduced with v0.28 synthetic neutral-B inputs: those did flip in host fixtures. Do not claim that the axis gap proves the entire hardware symptom's cause; confirm this fresh build on hardware if it persists.

Append float pitch to Input after its five original fields (Input20->24 host bytes). Production decoder uses -joyy/128 with.25deadzone; up/forward positive, down/back negative. D-pad up/down also fills pitch. Existing throttle/brake/steer mapping is preserved; pitch does not add motor force. All five-field positional initializers in source/test fixtures explicitly append zero; zero initializers remain zero. No Game/CarSurface growth: Game stays836host bytes.

vehicle_step selects flip axis from pitch when |pitch|>=.25, with trigger/reverse fallback only when neutral. Explicit forward stick overrides left brake; backward stick overrides right throttle. Direction sampled at valid second B edge. Neutral default forward, left trigger/reverse backward compatibility, charge/release/no-third gates, first-jump barrel roll, swat bonus, bounded motor/braking, contact cancellation, timeout and stable chase heading retained. No new manual CPU controls.

BALL_R2.2->2.75 gives radius and diameter25% increase. Keep RL_SCALE fixed at2.2/91.25, independent of live ball radius: do not accidentally scale the whole arena. Existing ball mesh/shadow/frustum/camera/contact/bank/wall/floor/ceiling/goal/spark code shares BALL_R, so visible and physical sizes agree. Goals/arena, speed caps37.394/60.997/159.123, ordinary hit gain1.595, flip multiplier1.15, acceleration/steering/gravity/jump/match parameters unchanged. Same mesh detail and renderer path; no hardware FPS claim.

## Validation

Full strict host suite passed. New decoder-to-physics coverage in split_loop_test uses host Dreamcast pad stand-ins, the actual controller_input function and game_step_two. Both players, forward/back stick,120/30/15fps input cadence (1/4/8 physics ticks per sample), first B/release/second B, inverted midpoint/upright completion, no repeat; explicit stick overrides opposite trigger, plus deadzone and D-pad direction. These are controller-code tests, not physical controller/hardware tests.

scale_speed_test now checks2.75radius, a4.15unit ground contact beyond the old3.85reach, and2.5unit post clearance that fitted the old radius but blocks the new ball. Existing caps/unchanged arena/gravity, ceiling contacts,24maximum-speed shots and normal jump peak2.84 pass. Enlarged corner-bank paths differ: the prior exact4second expectation of both inward velocity components no longer holds while the ball is still bouncing along a bank. Corner fixture now simulates full20second settling with finite/floor checks each tick and retains both inward-component assertions at the end; all four settle inward. No production bank-law changes.

Existing16flip heading/direction/player fixtures, rolled-pose flips, matched-speed10% ordinary/15% first-flip hit and separating/swats/caps, double-jump/first-roll gates,360recovery fixtures, tilt/steering/wall stress, CPU, two-player long simulation, matches/menus, audio and cameras pass. Standard renderer snapshots remain5016/2534triangles; ball subdivision unchanged. ASan/UBSan production decoder/main-loop and ball-size/scale checks pass with detect_leaks=0 (LeakSanitizer task inspection unsupported in runner). Full output in SIMULATION_RESULTS.txt.

New8second640x480silent20fps offline preview uses dedicated pitch inputs for opposite flips and larger ball; badge dashes. New FLIP_PREVIEW.png, BALL_PREVIEW.png and Controls PNG visually checked. Other PNG/audio/render comparison files remain older references.

Next hardware checks: fresh CDI, tap/release B then stick up/down + B with accelerator held, both humans, first roll then flip, late/wall flips/recovery, larger ball hits/rebounds/posts/crossbar and ball camera/FPS. Upper-wall traction/ceiling hanging and separate P2 engine voice remain pending.

---

# Resume here — v0.28 pace / flip-swat candidate, October 23, 2026

**Accepted baseline: v0.27.** User said it feels much nicer, then requested the game one notch faster including ball velocity when hit, and forward/backward second-jump flips for a slightly harder swat. Interpret one notch as10%; second jump flips forward by default, left trigger/reverse selects backward. Both human players. No v0.28 target build/emulator/hardware test here. Preserve accepted v0.27 for comparison.

Build `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Flip_Pace_v0.28`. Same dependencies and Makefile/PVR setup; no ELF/CDI supplied. Last user-reported versus FPS34–39 belongs to v0.26, not a v0.28 measurement.

## Pace

`GAME_PACE=1.10` multiplies forward CAR_SPEED, BOOST_SPEED, BALL_SPEED and therefore existing derived drive/boost acceleration. New caps37.394/60.997/159.123 DC units/s. `BALL_HIT_GAIN=1.45*GAME_PACE` applies to ground/aerial impulses from either car; upward lift values/ceilings likewise multiply1.10. At matched closing speed ordinary horizontal impulse is10% higher; faster cars may increase it further. Ball cap still checked after contacts. Gravity, arena, first/second jump impulse, boost drain/refill, reverse cap, braking force, timer and fixed120Hz logic remain unchanged. Full boost stopping fixture now.90s/24.42units vs v0.27 .83s/20.78units.

TURN_ACCEL_LIMIT56*GAME_PACE^2 retains v0.27 fixed-speed normal/boost radii20.64/54.91 at the faster caps. Low-speed2.15rad/s full-lock unchanged; full-lock range extends to31.52units/s. Near-flat wheel blend shares it; steep-wall law remains unchanged. CPU shares faster motors/hits and ground steering, but has no new manual flips.

## Flip implementation

CarSurface appends flip_axis(Vec3), flip_remaining/flip_time(float), flip_hit_available(int). Game grows by48 host bytes (788->836); no Input/controller ownership changes. Real first jump still earns air_jump_available. `drive` routes a valid second B edge into physical vehicle_step even with zero steer. Removed obsolete flat second-hop branch; first-jump fast path remains. Second edge retains world-up hop max(vy,0)+7.6 and consumes allowance, then captures local right with forward/backward sign. Brake>.1 or throttle<-.1 selects backward; main's left-trigger mapping produces one of these depending on forward speed.

Flip starts with zero angular momentum to take priority over barrel roll. A bounded pitch motor tracks one2pi rotation around captured local right, using signed remaining angle, target clamp(remaining*18,-10,10), torque gain60/bound120. Integrates actual angular step atan(rate*dt), retaining signed error for end braking/correction. Flip spin bound11rad/s; ordinary air roll still7.5target/8cap. Wheels cancel; time limit1.3s cancels blocked attempts; finish requires |remaining|<.012 and spin<.3. air_roll_active=-1 latch suppresses held-B barrel roll through completion until release. Recovery activation suppressed during an active flip; recovery PD/hop itself unchanged. Reset memset clears all state. If interrupted by contact, collision physics owns the resulting pose rather than snapping upright.

Planar heading is held during active flip: projecting pitched forward behind the car must not spin the chase camera180degrees. On completion/cancellation normal heading extraction resumes. Physical orientation/rendered mesh continues rotating normally. No chase-camera renderer change needed.

Aerial ellipsoid contact receives mutable CarSurface. During active unspent flip, add positive closing velocity from omega cross a chassis-sized1.65unit contact arm (not ball-center radius). First positive closing contact uses normal BALL_HIT_GAIN*1.15 and consumes flip_hit_available; separating contact does not spend it. Following contacts use ordinary response. Contact/finish/timeout/reset clears bonus. No hit without geometric contact, no unbounded additive ball velocity; existing total ball cap remains. This supports swatting with low center motion. Ordinary first-jump rolls do not receive flip bonuses.

## Checks and evidence

Full strict host suite passed. New flip_test includes production game.c to isolate static contact response plus public two-input gameplay.16fixtures across both humans, forward/backward and four headings reach inverted midpoint, settle back upright by.91s, hold camera heading, prioritize pitch over steering, land/recharge/reset. Additional rolled-pose flips return to starting pose. Checks no fall-created flip, reverse direction selection,10% matched-speed ordinary hit,15% first flip impulse, consumed/separating bonus, rotating-only swat and capped high-speed ball. Existing double-jump fixtures retain no third/held/fall hop, wall/rolled paths, both humans/recovery. Its chassis-origin peak now6.81 instead of5.33 because pitch changes the origin relative to chassis center; world-up impulses remain unchanged.

Control tuning regression retains7.5rad/s first-jump rolls for both humans/directions and v0.27 turn radii at new caps. Visual wheel-angle range intentionally updated to.135–.150rad for new steering factor; free-driving boosted90degree timing expectation now1.8–2.1x slow turn (measured1.417/.733s), rather than old >2x. Wall stress speed bound now configured BOOST_SPEED+.01 rather than stale60; its angular/displacement/settling checks retained. Stress max61.00speed,.524units/step,16rad/s collision spin. Existing360recovery fixtures still worst.76s, flat tilt audit has zero inverted steps in its tested fixtures. Complete logs in SIMULATION_RESULTS.txt.

ASan/UBSan flip and double-jump tests passed with detect_leaks=0 (LeakSanitizer task inspection unsupported in this runner). No new heap allocation/assets/audio channels. Lean versus renderer unchanged except Controls text. Production physical solver now runs during second-jump flips; target frame-rate impact requires hardware test.

New eight-second640x480silent20fps offline split preview: opposing forward/backward flips, then faster boosted turns. Badge dashes. New FLIP_PREVIEW.png and Controls PNG; all other images/audio and old renderer benchmark are historical references. Next test: both flip directions at speed, first-roll into flip, late/wall flips and recovery, swat feel, stopping distance/turning, cameras and versus FPS. Pending upper-wall traction/ceiling hanging and separate P2 engine remain outside this change.

---

# Resume here — v0.27 roll / steering candidate, October 24, 2026

**Accepted baseline: v0.26 double jump / lean versus.** User reported 34–39 FPS and that double jump works. Requested faster manual barrel roll and tighter high-speed steering. Clarification selected showing current speeds; keep caps unchanged. v0.27 is host-tested, awaiting user hardware feedback. No target compiler/emulator here and no ELF/CDI supplied.

Build `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Roll_Steering_v0.27`. Same build, renderer, audio, memory layout and controller/menu logic as v0.26.

## Implementation

- `vehicle.c`: manual air-roll target 4.5 -> 7.5 rad/s, rate gain14 ->18, torque bound35 ->55. Feed-forward drag compensation and existing8 rad/s total spin cap remain. Center/release commands target zero and brakes through the same motor. Analog speed and direction reversal remain. Applies to blue and human orange; CPU manual roll remains disabled. Separate recovery PD target5 / torque40, hop impulse and held-B gate unchanged.
- `game.h`: TURN_ACCEL_LIMIT42 ->56. Existing steering scale gives 25% smaller theoretical/fixed-speed radius at normal/boost speed caps. Maximum2.15 rad/s low-speed yaw unchanged (full-lock range now extends to about26.05 units/s instead of19.53). Near-flat wheel steer/yaw blends use the shared factor; steep-wall law unchanged. This shared ground tuning also affects the CPU. No new per-frame work.
- CAR_SPEED=33.99452 and BOOST_SPEED=55.45205 DC units/s, matching1410/2300 RL units/s through RL_SCALE=2.2/91.25. No acceleration/brake/boost/jump/gravity/collider changes.

## Verification and previews

Full strict portable suite passed, including new `control_tuning_test.c`: both players and roll directions, >one full turn in1 second from rest, steady rate7.50 rad/s, release below.15 rad/s in.2 seconds, counter-roll, unchanged slow yaw and normal/boost radii20.64/54.91 (25% below v0.26 fixed-speed reference). Tests deliberately recenter positions to isolate the motor/steering from wall contact; these are not complete match trajectories.

Common isolated host roll probe against unchanged v0.26: at.25s rate4.436 ->7.457 rad/s; at1s accumulated roll4.206 ->7.022 rad; after.2s release residual.14856 ->.12194 rad/s. Same signs mirrored. Full suite confirms caps33.99/55.45, first-hop peak2.84, double-hop fixture5.33,360 recovery fixtures worst.76s, zero inverted steps in existing flat tilt fixtures, wall/carving stress, CPU, split/main/menu/match/audio/camera and finite renderer. Boosted 90-degree free-driving turn-span fixture now78.70 units; fixed-speed radius is the cleaner comparison because that fixture also includes acceleration/drag.

Wheel visual regression deliberately updated from the old <.12-rad high-speed wheel-angle bound to .125–.135 rad, matching tighter steering while retaining finite/read-only/phase checks. No renderer production edits. AddressSanitizer/UndefinedBehaviorSanitizer passed control-tuning and double-jump tests with leak detection disabled: LeakSanitizer cannot inspect /proc tasks in this runner. No new heap allocation exists in this change. Full results in SIMULATION_RESULTS.txt.

New SIMULATION_PREVIEW.mp4 uses production split renderer and real input streams: opposite held rolls/double hops then boosted turns. 640x480,8seconds,20fps offline encoding; badge dashes. AIR_ROLL_PREVIEW.png extracted at1.3s. Other PNGs/audio and RENDER_COMPARISON.txt remain historical references. Production renderer unchanged; do not claim a new hardware FPS measurement. v0.26 versus performance report34–39 FPS is user feedback, not a v0.27 test here.

Next hardware focus: roll response, release/counter-roll/double jump, boosted cornering/bank transitions and physical landing tilt; both players and versus FPS. Pending upper-wall traction/ceiling hanging and separate P2 engine voice remain outside this change.

---

# Resume here — v0.26 double jump / lean versus candidate, October 23, 2026

**Accepted hardware baseline: v0.25 recovery and air roll.** User reported “it works great.” They now requested double jump from a kid tester and explicitly asked to restore 1v1 multiplayer with performance optimization, accepting that 60fps is not required. This supersedes the earlier shelving of v0.24. Keep v0.25 as the accepted comparison; v0.26 is host-tested and awaits emulator/hardware feedback.

Build: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_DoubleJump_Versus_v0.26`. Same object list/Makefile/ip.txt and KOS/CDI tools; no new assets/libraries. Reuse the v0.24 tested explicit PVR setup: 2 MiB vertex buffer, opaque bins only, three overflow sets. No v0.26 target compiler/emulator available here; no ELF/CDI supplied.

## Double jump and accepted recovery

Start from v0.25 physical solver and add one air_jump_available int at the end of each CarSurface (Game grows from 780 to 788 host bytes). Actual floor/wall jump earns one extra airborne B edge. B must be released between presses. Extra hop sets vertical speed = max(current vertical speed,0) + 7.6, using world up even while rolled; shared max-speed/ceiling constraints still apply. It consumes no boost. Third press/holding cannot add lift. Landing clears the old charge; next real ground/wall jump earns another. Falling without first jumping does not grant an air hop. Goal/reset clears via existing surface memset.

Both flat fast path and physical vehicle solver implement the same allowance. Flat first jump now starts the existing .22-second wheel-probe cooldown, and flat ticks decay it: otherwise switching to dynamic air roll immediately after takeoff could see the still-near floor as fresh wheel contact and erase the air-hop charge. This prevents false grounding/repeated takeoff. Ordinary neutral-steer first-jump impulse and peak remain unchanged.

Recovery retains the v0.25 B edge/hop/PD upright assist, with its allowance explicitly cleared; recovery cannot manufacture another airborne jump. Air-roll B+steer remains, and centering/releasing brakes roll. Both human cars have manual recovery/roll/double jump; single-player CPU keeps accepted normal jump/throttle recovery, with manual controls disabled. No new Input fields/buttons, boost/speed/collider changes, or sound samples.

## Versus integration

Port A blue/top and B orange/bottom, separate inputs/cameras and release gates, either Start pauses, P1 Y shared reset and B-on-pause Title. Disconnect either required port freezes world/goal timer/clock/audio without reassignment. Results Rematch preserves mode; Title resets it. Match wrapper selects game_step_two for every gameplay/celebration tick. Existing five-minute/OT/final-goal-celebration/results behavior remains.

v0.24 shared simulation/controller/menu/viewport logic restored on top of v0.25 recovery, including explicit human enable for orange. No orange AI decision is executed in versus; CPU tactical fields stay zero in long two-input tests. Title has Start Match / Controls / 2P Versus; full-detail single player remains available.

## Lean split renderer

Previous v0.24 removed only windows and still rendered almost the full scene twice; user estimated 10–15fps even in emulator. v0.26 adds a dedicated low-cost split path:

- Gradient sky, four coarse skyline blocks and simple stands replace stars/moon/detailed city/windows/lamps in split rendering.
- Floor uses 2x4 panels instead of 8x14; center ring uses half the segments. Bank collision-matching mesh stays unchanged; perimeter fence has sparse posts and fewer rails.
- Cars use a compact body/cabin/glass/stripe/lamps/spoiler shell with five-segment spinning wheels, retaining body suspension, team colors and boost jets. Full-detail original car/wheels are used in single player.
- Ball uses 4x8 surface cells instead of 6x10; fireworks use six rather than twelve rays per burst in split views. Gameplay pool/timers are unchanged.
- Conservative sphere-frustum tests reject unseen cars/ball/arena blocks, triangle view-frustum tests reject before projection, and screen-clip trivial accept/reject bypasses polygon clipping for most triangles.
- Split quads transform four shared corners once instead of six independently across two triangles; projected vertices use one reciprocal depth and multiplication instead of repeated division. Per-view frustum coefficients are cached.
- Split HUD merges contiguous glyph row pixels into rectangles and uses compact scores rather than repeating team names. Character pixels/legibility are retained. One opaque pass, fixed stack arrays, no per-frame allocation or new textures.

Full single-player path bypasses low-detail meshes/culling/merged text and keeps original look; default snapshots still submit 5016 / 2534 finite triangles. Perspective remains isotropic in each 640x240 view, with all geometry/HUD clipped to its half and reciprocal-depth interpolation at edges. Viewport globals reset after drawing, preventing effects on title/results/single view.

A small divider FPS badge measures actual recent drawn frames / raw elapsed time in main.c, before dt is clamped for fixed-step simulation. It updates about once per second and shows dashes during pause/controller loss/first sample. Never displays the offline preview's encode rate as hardware FPS. Main-loop harness tests 250ms raw frames produce about4 FPS, not the capped simulation-derived10 FPS, and suppresses badge during pause/loss.

## Workload evidence (not target FPS)

Common native benchmark compiles original v0.24 renderer against the same current Game data/header and uses the same 240 split frames across four fixtures with a minimal CPU backend. Five interleaved runs: v0.24 median 208.86ms vs v0.26 median33.62ms. This includes CPU backend work, not actual PVR submission; it does not model SH-4 or predict hardware FPS. RENDER_COMPARISON.txt retains runs/context; tests/render_bench.c supplies the common fixture. Baseline render source came from preserved v0.24 source.

Common benchmark kickoff: 8118 ->2979 submitted triangles, about63% fewer, before the small FPS badge. Separate split regression fixture: 8231 ->3082, with top1508/bottom1572 plus divider, likewise before badge. Fixture counts are not worst-case limits. Split stress fixtures check finite non-leaking viewports and configured vertex-byte budget. The restored2 MiB PVR buffer has ample headroom for those tested submissions; hardware is still the required performance test.

## Verification

Full strict portable suite passed after final production changes, including new double-jump tests, restored two-pad main harness, split rendering/stress, 360 accepted recovery fixtures, and all match/menu/physics/handling/wall/CPU/audio/camera/world-render checks. New hop tests cover exactly one extra jump, no third/held/fall hop, 5.33-unit peak in the timed double-hop fixture, landing recharge, air-roll and wall paths, independent orange hop, orange recovery and resets. Existing no-double-jump expectation updated intentionally to new one-extra/no-third rule. First-hop peak remains2.84.

ASan/UBSan passed double-jump, split physics/render and actual two-pad main-loop harness (leak detector disabled). Final FPS raw-time harness extension separately passed strict warnings. Two-human physics stress runs72,000 ticks with no AI tactical activity. New orange controls use the same v0.25 recovery solver as blue.

72,000 B-disabled single-player steps compare all780 v0.25 legacy Game bytes (serialize CarSurface prefixes before new air_jump_available plus unchanged Game suffix): byte-identical,56,160,000 bytes total. SHA256 `c7db9b48bb3c46c093eb6ae5e7d2f98d0174795130843ae3aa8c1fb2fb47882d`. tests/normal_compare.c uses -DDOUBLE_JUMP for the candidate. This verifies behavior outside the new B controls; intentional extra hops/versus are expected to differ. No new claim of identical jump-enabled whole-match state.

Audio and sound bank unchanged; one shared mix/engine follows P1, with both human cars' action/impact/goal events. No second engine channel was added. Main menus/results retain approved art and texture lifecycle/fallback.

New eight-second silent20fps host preview shows actual scripted two-human input simulation with first/second jumps, chase and ball cameras, lean meshes and FPS dashes. Inspected split chase/ball and Controls images; refreshed title image includes 2P Versus. Historical v0.25 recovery and v0.23 match/results PNGs, plus v0.19 WAV retained as labeled references. No image/video is a target capture or FPS claim.

## Hardware follow-up

Test one/second/third B presses, holds, landing recharge, wall jump -> air hop, B+steer roll and roof recovery in single player first. Then connect ports A/B, choose2P Versus, verify individual controls/cameras/recovery/hops and compare real FPS badge against the previous rough10–15 FPS report. Sample ordinary driving, both boosting/jumping, impacts, high ball camera, banks/walls and fireworks. Test pause/disconnect/reconnect, shared resets, full match/OT/results/Rematch/Title. Record FPS range and playability; do not infer improvement from native-host numbers alone.

Upper-wall traction/ceiling hanging remains the next separate physics task after this feedback. Preserve lower-wall carving; proposed upper-quarter upward-motor/adhesion fade is still not implemented. Keep v0.25 as fallback if the kid testers prefer its single-player behavior.

---

# Resume here — v0.25 recovery / air-roll candidate, October 23, 2026

**Accepted Dreamcast baseline: v0.23 timed matches.** User completed a full five-minute match without problems. v0.24 top/bottom split-screen versus was tried and rejected: user estimated roughly 10–15 FPS even in the emulator. Do not treat this as a measured real-hardware FPS figure. User reverted to v0.23; split screen/co-op can remain future web/three.js ideas like their Neon Cycles/Seed City projects. v0.25 starts directly from the v0.23 source, with no split mode, viewport code, extra PVR buffers, controller-port changes or three-option menu.

User next reported rare overturns that leave the player stranded until a goal, and straight-up wall driving that reaches the ceiling and hangs while throttle is held. We agreed to stage recovery/air roll first, then upper-wall traction. This v0.25 candidate implements the first step only and awaits hardware feedback. Upper-wall grip/ceiling behavior is explicitly still pending.

Build: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Recovery_v0.25`. Makefile, main.c, match logic, native KOS/PVR initialization and CDI workflow unchanged from accepted v0.23. No target compiler/emulator available here; no compiled ELF/CDI supplied.

## Recovery

Append recovery_time, recovery_direction and air_roll_active to CarSurface (12 host bytes per car; Game grows from 756 to 780 bytes). Existing CarSurface memset in init/reset clears all state. No allocations or new object files/assets.

Only the player's vehicle has deliberate recovery/air roll enabled via an explicit allow_air_roll parameter through drive -> vehicle_step. CPU retains accepted driving, normal jump and old throttle-only recovery; no accidental AI air rolls from its existing steer+jump combinations. Input struct/controller mapping unchanged.

A new B edge can start recovery if no wheel contacts, chassis up.y < .35, speed < 15, flat arena_floor < .05, actual lower body-sphere extent is at/below floor + .14 and physical body center is within 2.1 units of floor. This requires slow physical floor contact, not merely inverted flight, so there is no aerial second recovery jump. Bank/steep-wall contacts retain normal jump handling; deliberate recovery targets the flat playing floor.

Launch sets world vertical velocity to at least 6.5 (not inverted chassis-up), recovery_time=1.3, probe cooldown .30, clears transported contact/arc frame and reuses the existing jump sound event. No positional teleport, score/time reset or invulnerability. During recovery, bounded torque drives body up toward world up through the existing angular integration and collision solver. Target angular rate up to 5rad/s, slowing with angular error; torque magnitude capped at 40. Direction degeneracy at perfect inversion chooses forward axis with steer-biased/deterministic sign. Three-dimensional pitched/rolled poses recover using cross(chassis_up, world_up), not roll-only correction.

Assist ends on sufficiently upright multiwheel contact or timeout. air_roll_active=-1 prevents a held recovery B from starting another roll. Releasing B clears this latch even in the accepted flat fast path (regression check added so a later jump can air roll normally). Jump remains edge-triggered, so holding B does not repeat the recovery launch or jump.

## Air roll

B + steering magnitude > .15 in flight opts the central flat driver into the physical body solver. Launch alone/neutral steer keeps the accepted flat jump path; grounded/bank/wall steering remains unchanged while wheels are in contact. Physical dynamic mode already existed near walls; this exposes it deliberately in central airborne play.

In air, held B+steer commands roll about physical forward at up to +/-4.5rad/s. Rate motor is bounded to +/-35 torque, with damping compensation. Normal air yaw torque is replaced while roll control is active, avoiding unintended simultaneous yaw. Center steering or release B commands zero roll rate so the car does not spin indefinitely. Wheel contact clears the air-roll state; the existing settled-upright dynamic-to-flat transition remains. Recoveries take priority over manual air roll, preventing held recovery input from immediately flipping the car again.

No pitch controls were added; automatic recovery can correct pitch/roll as required. Camera laws, renderer geometry, wheel animation and cosmetic suspension remain accepted; existing physical orientation naturally renders rolls. Controls page adds the two B actions and slightly extends its panel.

## Verification

Full strict portable suite passed: new recovery checks plus every accepted match/menu/physics/handling/wall/carving/CPU/jump/audio/camera/world-render test. ASan/UBSan passed recovery checks with host leak detection disabled.

Recovery test runs 360 resting roof/side/pitch fixtures across eight headings, five roll poses, three input directions and three initial pitch offsets. Every case recovers with one B edge; worst time to up.y>.98 and >=3 grounded wheels is .76 seconds in these fixtures. Also verifies both flat-driver air-roll directions, rate braking on release, held-B no repeated hop, release-after-recovery allowing a fresh air roll, no recovery jump while inverted at height, and reset state. These timings are fixture results, not universal promises under impacts or on banks.

72,000 B-disabled player-input steps compare all 756 legacy Game bytes (serialize old CarSurface prefixes, omit new state fields, then serialize the unchanged Game suffix) against accepted v0.23: byte-identical. Total 54,432,000 bytes; SHA256 `04fc67efe94a86e0f5a27df9f51d7d0ad908773fc2ed00f7ab951093a3ce94de`. Script retains AI and steering/brake/boost/reset/event-clears; player B is disabled specifically to verify behavior outside the new controls. This is not a claim that intentional B+steer flight remains identical. Existing neutral-steer jump and wall regression checks pass separately.

Default no-effect snapshots remain 5016 / 2534 finite triangles. No gameplay triangles, new render pass, samples or voices added. A recovering car briefly uses acosf/normalization and bounded vector torque; central air roll uses the existing dynamic wheel/body solver instead of the flat fast path. Real-hardware cost remains to be tested.

New SIMULATION_PREVIEW.mp4 is silent, 16 seconds at 20fps, real simulation time, fixed host close-up: roof recovery, side recovery, left air roll and right air roll with later B recovery taps. Inspected inverted, rolling, recovered and airborne frames plus updated Controls UI. This is not a Dreamcast recording or FPS measurement. Original title art remains intact; older match/results PNGs and impact WAV retained as labeled references.

## Next steps

Hardware test tap recovery from roof/both sides, release/hold B, a second jump after recovery, both roll directions/braking, floor/bank/wall jumps, cameras, pause/reset, normal match and smoothness. If recovery is accepted, proceed with the already-discussed upper-wall/ceiling pass: preserve lower-wall carving; gradually reduce upward motor force and adhesion through the upper quarter; prevent sustained upward drive into the ceiling while allowing momentum to reach it briefly. Current code can climb because wall motor can overpower gravity; do not assume adding gravity alone fixes it. Do not reintroduce split-screen changes from v0.24.

---

# Resume here — v0.23 timed-match candidate, October 23, 2026

**Accepted real-hardware baseline: v0.22 illustrated title/menu.** User reported “nice. works great.” They approved the next proposed step: timed matches, overtime and a results screen. This separate v0.23 full-source candidate is host-tested and awaits hardware feedback. No numerical FPS supplied. Preserve v0.22 for comparison.

Build: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Matches_v0.23`. Add match.o and match.h dependencies to the existing Makefile. Existing native C/KOS pipeline and title artwork embedding unchanged. No target compiler/emulator available here; no ELF/CDI supplied.

## Rules and state

New portable match.c/h wraps the existing game_step. Match has phase, ticks_left and overtime_ticks (12 host bytes), separate from unchanged 756-byte Game. MATCH_TICKS = 5*60*120 = 36,000 active fixed ticks; no accumulated floating-point clock drift. Clock uses simulation time, including the existing main-loop dt clamp/fixed-step cadence. Pause, absent controller, menus/results and the existing two-second goal celebration do not consume active playing time.

REGULATION advances game_step then consumes a tick; this ordering admits a final-tick goal. At zero with unequal scores, transition directly to OVER if there is no goal celebration, or FINISHING if the last tick scored. No extra play for an airborne ball at zero. At zero tied, enter OVERTIME and game_reset for a fair kickoff, preserving scores. A last-tick equalizer's active goal timer is allowed to finish normally before that reset. OVERTIME counts active ticks upward; the next goal enters FINISHING. FINISHING allows the original celebration to finish (including game_reset) before OVER. No extra physics after OVER. Score retention and original sound/fireworks timers remain in Game.

main.c initializes Match for Start Match, Rematch and paused Y whole-match reset. Live Y keeps the clock and scores and resets positions, except during FINISHING where that input does not skip the winning celebration. Existing Start pause/X camera/B-on-pause return remain. match_step runs only in the original connected/unpaused accumulator loop. Stop substeps when OVER; stop audio, set FRONT_RESULTS, zero accumulator and upload the original title texture after GPU readiness. No match runs behind results. Rematch uses the same new-match path and A/Start release gate as Start Match; scores/time/camera reset. Result entry uses pressed-edge controls and neutral stick latch to avoid an existing held gameplay button selecting a result action.

## Presentation

Clock overlay sits between the existing logo and scoreboard: MM:SS regulation (ceil remaining ticks so 05:00 initially), last ten seconds orange; OT MM:SS counts upward in gold. Game/camera renderer otherwise stays accepted. No artificial camera motion or physics tuning.

FRONT_RESULTS added to portable frontend. Two options: Rematch / Title, with the accepted D-pad/stick navigation and A/Start confirmation. B also returns to Title. Winner and final score rendered over the approved art; plain fallback works if texture allocation fails. Source image, texture packing and VRAM release-on-match lifecycle unchanged. No new sounds/assets; existing goal cheer is reused.

## Verification

Full strict portable suite passed: match rules plus all existing menu/physics/handling/wall/CPU/impact/exhaust/fireworks/audio/camera/world-render tests. Match tests run a complete 36,000-tick tied regulation, check exact expiry, normal-goal clock freeze, fair overtime kickoff, OT elapsed time, sudden-death celebration completion, regulation winner, last-tick winner/equalizer, score retention/reset/frozen finish, results actions and bounded finite HUD geometry. A 1,200-tick active-play comparison against direct game_step is byte-identical for all Game bytes.

Actual main.c host harnesses passed separately after the full suite: existing title/Controls/start/pause/return/restart harness extended to verify clock freeze on pause and an active-match controller disconnect/reconnect; new accelerated-expiry harness exercises results winner/score, repeated A/Start rematches, fresh clocks/scores, Title/B escape, held-input isolation, texture allocate/free counts and allocation-failure fallback. The accelerated harness changes duration and seeds a winner only through test wrappers; production duration is still five minutes. Harness is not an emulator or target-header validation.

ASan/UBSan passed match rules/render test and actual-main accelerated match loop (host leak detector disabled). game.c/game.h/vehicle.c/audio.c/sound_bank.c and title assets remain byte-identical to accepted v0.22; simulation forces, collision and AI are unchanged. No new 72,000-step whole-match equivalence is claimed because expiry/reset flow intentionally changes after five minutes. Default world snapshots still submit 5016 / 2534 finite triangles; not hardware FPS measurements.

Visually inspected 640x480 regulation, overtime and both winner results previews from actual production rendering plus packed title data. Corrected a host-preview-only issue where drawing the same HUD twice left stale score pixels at equal depth; final preview draws the tied OT score once. Target rendering already clears every frame. Existing v0.21 video/v0.19 sound preview are retained and explicitly identified as historical.

## Hardware follow-up

Play full regulation with a lead and a tie. Check clock freeze on pause, goals, absent controller; live/paused Y reset rules; tied overtime kickoff; first OT goal celebration; results; Rematch/Title/B and repeated texture transitions. Verify score/time reset and confirm no unintended boost/pause on start. Check hardware smoothness and legibility. If accepted, promote v0.23 to baseline; preserve accepted physics if adjusting match flow.

---

# Resume here — v0.22 illustrated title-screen candidate, October 23, 2026

**Accepted real-hardware baseline: v0.21 goal fireworks.** User reported “everything is working well.” They supplied a Rocket League image as a title-art reference, approved the original generated Vectorball DC artwork (“thats sick. lets use it”), and authorized use as the title screen. No numeric FPS supplied. This separate full-source v0.22 candidate is host-tested and awaits hardware feedback. Preserve v0.21 for comparison.

Build: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Title_v0.22`. Existing elf2bin/scramble/makeip/makedisc flow unchanged. Add frontend.o/title_art.o and header dependencies to the existing Makefile object list. No new runtime library, disc file lookup, Python build dependency, or alternate CDI tool. No target KallistiOS compiler/emulator here; no ELF/CDI supplied.

## Art and GPU path

Approved built-in generated image copied unchanged to `assets/title_art.png`. Optional Python/Pillow `tools/pack_title.py` downsamples to 512×512 and encodes RGB565 in committed `src/title_art.c` (32-byte aligned uint16_t array). Native C texture data is already supplied, so DreamSDK builds need no image library/converter. `title_art.h` declares the sized array.

main.c uploads 524,288 bytes via pvr_txr_load into pvr_mem_malloc memory and uses RGB565 | NONTWIDDLED plus bilinear filtering. Full UV range on a 640×480 quad restores the original 4:3 artwork proportions. Existing opaque pass carries image at reciprocal depth 1 and colored menu geometry at depth 100+. Culling disabled as in the original renderer. pvr_wait_ready precedes allocation changes/free so the previous frame has finished; the texture's VRAM copy is freed on match entry and reallocated on return. The embedded system-RAM copy remains, about 512 KiB. No per-frame allocation or asset loading. Allocation failure gives a plain colored title/menu fallback instead of blocking navigation. Cast on pvr_txr_load source supports older KOS headers whose input type lacks const; loader only reads the data.

Original prompt/style/provenance recorded in assets/TITLE_ART_NOTES.md. PNG source and previews are reference artifacts, not target runtime dependencies. Target uses bilinear filtering; host menu preview decodes the packed RGB565 texture using nearest sampling.

## Flow and controls

New portable frontend.c/h separates TITLE, CONTROLS and MATCH from Game. Both D-pad axes and the dominant stick axis navigate two horizontal options. One navigation step per stick deflection, .55 engage / .25 neutral hysteresis; return/controller loss requires neutral before analog navigation. A/Start activates highlighted option. Controls returns via B, A or Start; selection remains Controls. Match start resets scores, camera mode and positions; no simulation/audio runs behind the menu.

main.c adapts pressed KOS button edges into portable menu events. A release gate suppresses match face-button/trigger actions until A/B/Start/X/Y and triggers return to neutral, avoiding A-as-boost or Start-as-pause on match entry. Existing active match input/physics loop remains. Start pauses/resumes, X toggles camera, Y resets positions (or whole match while paused). New B while paused returns to title, stops sounds and clears the accumulator. Starting again is a fresh 0–0 match. Pause panel adds B TO TITLE. Menus mute audio through existing audio_update paused path; no new audio assets/channels/music.

Rendering uses live code text/selection borders over the approved image's dark lower area. Start Match/Controls are the only main-menu labels. Controls overlay has eight input rows and B TO RETURN. No-controller prompt appears only when needed. Gameplay renderer only changes by adding front-end UI functions and the pause-return text.

## Verification

Full strict portable suite passed, including frontend navigation/hysteresis/controller-loss/Controls/back/new-match state tests and all prior fireworks/exhaust/physics/handling/wall/CPU/audio/camera/world-render tests. After adding the target-loop harness, it separately passed strict -Wall -Wextra -Werror and ASan/UBSan with host leak detection disabled. Harness includes actual main.c, uses host-only KOS/PVR/audio/render stand-ins, and checks title->Controls->back->match->pause->title->restart with A and Start held, no unintended boost/pause, muted menus, controller absence, GPU allocation/free counts and allocation-failure fallback. These are not emulator/hardware checks or validation of the installed DreamSDK headers.

Game structure and game.c/vehicle.c/audio.c/sound_bank.c are byte-identical to accepted v0.21. No game-state additions or gameplay tuning. Full default renderer snapshots still submit 5016 / 2534 finite triangles with no menu active. Existing v0.21 72,000-step state comparison remains historical; v0.22 gameplay preservation is established by unchanged production simulation bytes and regression checks, not a newly claimed raw-state comparison.

Inspected 640×480 TITLE_PREVIEW.png and CONTROLS_PREVIEW.png made with the actual packed texture and production UI geometry. Original title art remains intact; menu text is live, centered in the lower dark area and legible. SIMULATION_PREVIEW.mp4 remains v0.21 fireworks; WAV remains v0.19 impacts. No new movie/audio claimed.

## Hardware follow-up

Check title texture upload/orientation/color/legibility, menu controls with stick/D-pad, Controls/back and controller reconnect. Start with A/Start and hold them, release then play. Pause/return/restart repeatedly, including after scoring, to check audio, VRAM lifecycle and fresh score. Test original play smoothness. If accepted, promote v0.22 to hardware baseline. If the picture path fails, fallback navigation should still permit play; inspect target output before changing accepted game physics.

---

# Resume here — v0.21 goal fireworks candidate, October 23, 2026

**Accepted real-hardware baseline: v0.20 boost exhaust.** User reported “everything works great on real hardware,” then approved goal feedback and suggested fireworks. No numerical FPS supplied. This separate full-source v0.21 candidate is host-tested and awaits hardware feedback. Keep v0.20 for comparison.

Build unchanged: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Fireworks_v0.21`. No target compiler/emulator is available here; no compiled ELF/CDI bundled. Makefile/ip.txt and all dependencies unchanged.

## Goal fireworks and scoring cue

Only production `src/render.c` changes. No new Game members, physics/event hooks, assets, allocation, RNG, sound channels or render pass. Existing SoundEvents goal cheer remains untouched. Existing two-second scoring pause and kickoff resume are identical.

`goal_fireworks` is a read-only analytic effect driven by age = 2 - reset_timer and last_goal. Three bursts start at 0, .32 and .64 seconds; each lasts 1.08 seconds, so all sparks disappear by 1.72 seconds, before kickoff. The existing reset timer stops on pause/controller loss, so fireworks freeze; reset immediately removes the effect. Frame sound-event clearing does not remove the fireworks or retrigger audio.

Burst centers sit inside the scoring goal plane at z = sign * (ARENA_Z - 3), x = [-.55,+.55,0] * GOAL_W, y = .48*GOAL_H + [0,1.2,2.4]. Initial higher placement was revised after preview inspection because too much was above the chase view. Lower placement keeps the expanding bursts around the upper goal mouth without altering cameras. Actual visibility still depends on the camera facing the goal and world occlusion.

Each burst has 12 deterministic radial sparks with velocity 9, 10.2 or 11.4 units/s in the goal's x/y plane, plus small depth variation. Gravity-shaped travel uses y -= 3*t*t. A trailing triangle spans .13 seconds of analytic travel and a camera-facing diamond (two triangles) shrinks from radius .38 to zero. Team blue `61cfff` or orange `ff944d`, gold `ffd166`, and pale `fff3cf`. Opaque depth testing handles goal/car/world occlusion. Max 36 sparks / 108 source triangles; near-plane clipping can split each source triangle into two, bounding submissions at 216. Ordinary play submits none of this geometry.

The original GOAL panel becomes a centered BLUE SCORES / ORANGE SCORES panel. No new helper labels, camera motion, flashes across the screen, or changes to accepted match flow.

## Verification

Full strict C99 portable suite passed. New fireworks test triggers both actual scoring paths, verifies correct scoring side/colors, sweeps the entire lifetime with <=108 submitted triangles in its unclipped view, checks frame-clear persistence, repeat paused/read-only rendering, reset/kickoff/score retention and finite near-plane-clipped geometry. Final lowered geometry reran the strict fireworks check; ASan/UBSan also passed (host leak detector disabled). All prior physics/handling/wall/CPU/audio/impact/exhaust/camera/world-render tests passed. Default no-celebration snapshot submissions remain 5016 and 2534.

72,000 scripted simulation ticks compared the entire 756-byte Game structure against accepted v0.20 at every step: byte-identical. Comparison file size 54,432,000 bytes; SHA256 `1cd70af94715e046f9434c63d9d6eb0132182e3ddcf1f4c41a417d291c093e84`. game.c, game.h, vehicle.c, main.c, audio.c, sound_bank.c and render.h are byte-identical to v0.20. Existing gameplay_compare defaults to sizeof(Game); no new macro needed.

`SIMULATION_RESULTS.txt` records checks. `SIMULATION_PREVIEW.mp4`: silent 9.6-second 20fps host rendering at real simulation time, actual blue/orange scoring in chase and wider views, including kickoff. Representative final chase and wider burst frames inspected; initial orange palette also inspected. This is not a Dreamcast recording or FPS measurement. Retained WAV is unchanged contact audio from v0.19, not new fireworks audio.

## Hardware follow-up

Score at both goals in both cameras, from close/far/side and after high aerials. Check team colors, banner, readability, smoothness, pause/resume, resets and clean kickoff. Existing goal cheer should play once. The fireworks are anchored at the scoring goal, not forced into view when the camera faces elsewhere. If accepted, promote v0.21 to the hardware baseline. Preserve accepted handling while tuning visuals if required.

---

# Resume here — v0.20 boost exhaust candidate, October 23, 2026

**Accepted real-hardware baseline: v0.19 impact feedback.** User reported “everything seems to be working well on real hardware,” then approved the proposed boost-exhaust step. No numerical FPS was supplied. This separate full-source v0.20 candidate has passed host checks and awaits hardware feedback. Preserve v0.19 for comparison.

Build unchanged: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Exhaust_v0.20`. Makefile, ip.txt, main.c, vehicle.c, audio.c and sound_bank.c are unchanged. No target compiler/emulator is available here; no ELF/CDI is bundled.

## Exhaust implementation

Append two `BoostExhaust {float strength,phase;}` observers to Game, growing the host structure from 740 to 756 bytes. Each reads its own car's input, pre-drive fuel, remaining fuel and forward speed. Both pre/post fuel must be positive, preventing a false flash on the recharge tick or when the last fuel is spent. Positive throttle plus boost is required; brake/reverse/release suppress the visual immediately. Braking suppresses this visual even where the accepted dynamic vehicle solver continues consuming boost: that solver is intentionally unchanged.

Strength = clamped throttle times (.65 + .35 * clamped absolute forward speed / BOOST_SPEED). A fixed-tick 4Hz triangular phase modulates length by just 10 percent. No RNG or renderer-driven updates. Pause/controller-loss freezes simulation and therefore the effect. Reset and goals clear both effects; disabled CPU clears its effect. Existing strongest-contact events and spark lifetimes are untouched.

Renderer attaches two tapered jets to the existing tailpipes at local x +/- .48, y .41, z -2.055. Each uses a short pale four-sided base (8 triangles) joined to a warm orange tip (4 triangles). Total 24 opaque depth-tested triangles per active car, 48 max with both active. Length about 1.29–1.85 units at full throttle depending on speed/pulse. Uses the same cached body/suspension transform, so banks, wall orientation, airborne rotation and shell flex carry the exhaust with the car. CPU render copy explicitly selects CPU exhaust. No world transform changes, transparent pass, textures, allocation, extra lights or audio.

## Validation

`sh tests/run.sh`: strict portable suite passed, including new exhaust checks and every prior physics/handling/wall/CPU/impact/audio/camera/render test. New checks cover throttle/speed sizing, release/brake/reverse, starting empty, final fuel depletion, aerial operation, actual CPU decision, disabled CPU, bounded phase/strength, resets/goals, finite geometry, read-only rendering and exactly 24 added submitted triangles per visible car in the fixture. AddressSanitizer/UndefinedBehaviorSanitizer passed the exhaust test (host leak detector disabled).

72,000 scripted simulation ticks compared all 740 existing v0.19 Game bytes at every tick, including fuel, AI, physics, sounds, wheels, suspension and sparks: byte-identical. Comparison file size 53,280,000 bytes; SHA256 `59f2e822075eb906c8a1113eb7945b662eac289497fa46c51f9481efd1c6a5dc`. `tests/gameplay_compare.c` selects `-DBOOST_EXHAUST` for the v0.20 prefix. Renderer is read-only. These checks do not establish Dreamcast FPS.

`SIMULATION_RESULTS.txt` records the suite. `SIMULATION_PREVIEW.mp4` replaces the prior impacts movie with a silent 12-second, 20fps host preview at real simulation time: 3 seconds each of chase boost/release, rear close-up, aerial boost and CPU boost/brake cutoff. Inspected representative chase, rear, aerial and CPU frames; effects attach cleanly and remain visible. Preview is not a hardware capture or FPS measurement. Retained WAV is unchanged v0.19 impact audio, no boost sound was added.

## Hardware follow-up

Test boost taps, launch/sustained boost, braking, reverse, empty tank/recharge, jumps, banks/walls and both cameras. Watch player and CPU together for readability and smoothness. Check cutoff, body attachment, goals, pause and reset. If successful, accept v0.20 as the next hardware baseline; otherwise preserve the accepted v0.19 gameplay while tuning only the visual.

---

# Resume here — v0.19 ball-contact feedback candidate, October 23, 2026

**Accepted real-hardware baseline: v0.18 suspension.** User reported “everything works well on real hardware”; no numeric FPS supplied. They approved the next step: brief ball-contact sparks and clearer impact sound scaling, preserving the approved physics. This separate full-source v0.19 candidate is host-tested, awaiting user/hardware feedback.

Build unchanged: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Impacts_v0.19`. Keep v0.18 for comparison. No compiled ELF/CDI. Makefile/ip.txt and target dependencies unchanged; ZIP timestamps fixed in the past.

## Contact feedback

Only actual approaching car-to-ball contacts trigger the new profile: grounded, aerial and CPU contacts. Existing normal impulses, lift, separation and recoil are unchanged. Legacy SoundEvents values remain identical. Append96 bytes of `ImpactFeedback` to Game: four fixed contact glints, strongest car-contact strength and original impact position. No allocation or new asset load.

Each glint lasts.14 simulated seconds. Closing speed must exceed5 units/s for a visual glint; softer approaching touches can still make a quiet hit sound above the existing2-unit audio threshold. Each contact normal identifies the struck ball-surface patch. The camera-facing glint follows that patch while the ball moves, preventing a fast car from leaving its flash behind. A small.20-unit camera-facing offset improves visibility against the ball surface; ordinary world depth testing still applies. Six tapered yellow/orange rays and a small pale core use eight opaque triangles per glint, at most32 total. Rays thin and disappear with age. No transparency/glow pass, textures, particles allocated on the heap, or random generator use.

Pool replacement uses a free slot or the oldest active glint. Life advances only on simulation ticks, so pause freezes visuals. Frame event clearing resets the strongest car-contact strength but preserves living glints. Goals clear glints immediately and reset clears all feedback. Wall, bank, floor and ceiling rebounds keep their existing feedback/audio path and do not create these car-contact glints.

## Impact sound

Existing SOUND_HIT sample/channel are reused. For a car contact, `power=clamp(closingSpeed/BOOST_SPEED,0,1)`, close-range volume=`45+195*power`, playback frequency=`SOUND_RATE*(1.12-.22*power)`. Hard hits are louder and lower pitched than soft touches. Strength scaling now spans the accepted55.45-unit boosted speed rather than saturating the old impact volume at35. Contact position drives3D distance attenuation `clamp(22/(distance+12),.20,1)`, including height. Strongest car strike is retained across all fixed substeps in a rendered frame. The existing.07-second hit cooldown and one owned impact channel remain; no additional simultaneous sound channels.

Ordinary rebounds retain the previous sound curve. Engine, jump, landing and goal behavior, samples, PCM bank and channel ownership remain. The original58,336-byte sound bank is unchanged. Actual Dreamcast mixing/listening still needs user feedback; host stand-ins validate calls and scaling rather than AICA output.

`IMPACT_AUDIO_PREVIEW.wav` is an optional11025Hz mono PCM16 host preview: soft5, medium25 and hard55.45 closing-speed examples, with silence between them. It resamples the original hit sample using the new close-range volume/pitch settings, without engine/mixer/distance effects; illustrative, not recorded hardware. Peak19430, below PCM16 full scale. Source helper is `tests/audio_preview.c`.

## Scope and checks

Production changes: game.h/game.c (feedback observation and lifetime), render.c (glints), audio.c (car-contact playback curve). Vehicle solver, main/controller, render.h/camera settings, sound bank, Makefile and ip.txt remain byte-identical to v0.18. No handling, collision, gravity, ball speed/impulse, wheel, suspension, AI or scoring changes.

All644 original Game bytes compare byte for byte against v0.18 over72,000 identical scripted fixed steps, INCLUDING all prior wheel/suspension state and legacy sound events. New96-byte feedback suffix excluded. Both46,368,000-byte dumps SHA256 `8a10b13f4365b083fea1fac792e3355484725a29df2e0c654b325f2542f5ca36`. Optional comparison helper: `-DIMPACT_FEEDBACK` for v0.19, no macro against v0.18 sources; baseline package required separately.

Full strict host suite passes. `impact_test.c` checks actual ground/aerial/CPU contacts, separating and soft touches, no wall glint, strongest-substep retention, lifetime/frame-clear/reset/goal handling, bounded pool, finite/read-only rendering and32-triangle maximum. Audio stand-ins now check harder hits are louder/deeper, cooldown and vertical distance attenuation. Existing suspension/tilt/wheel/handling/speed/wall/AI/jump/scoring/audio/camera/world checks remain. ASan/UBSan passes contact/render checks. Final visual contrast/depth-bias tuning was retested with the contact renderer test.

`SIMULATION_PREVIEW.mp4`: about12s20fps silent host reference, HALF SPEED for visibility, v0.18 renderer left/v0.19 right using identical simulation/camera scripts. Four1.5s simulation scenes: weak ground, boosted ground, aerial and CPU hits. Soft/strong/aerial frames inspected; fast-shot flash placement corrected to follow the struck patch. Not hardware footage or target FPS evidence. Reference scene counts without active glints remain5016 kickoff/2534 corner.

**Next user test:** compile and compare with v0.18 on real hardware. Try gentle taps, normal and boosted shots, airborne hits and CPU contacts. Judge spark visibility/duration and whether hard shots sound more powerful without becoming harsh or repetitive. Check distant/high hits, normal wall/floor bounces, goals, pause/reset and hardware smoothness. Tune effect size/duration and car-hit volume/pitch from feedback before adding further features. No v0.19 target build, hardware-audio/FPS result or user acceptance yet.

Prior history follows; this section supersedes older status and preview descriptions.

---

# Resume here — v0.18 visual suspension candidate, October 23, 2026

**Accepted real-hardware baseline: v0.17 animated wheels.** User reported “everything looks and works good on real hardware.” No numeric FPS supplied. They approved subtle body suspension and asked to check the slight high-speed tilt, noting a possible low chance of flipping (“flow” interpreted as “flip”; no confirmation supplied). v0.18 is a separate host-tested full-source candidate, awaiting user/hardware testing.

Same build: `make gdemu` -> `gdemu/01/disc.cdi` in `Vectorball_DC_Suspension_v0.18`. Keep v0.17 for comparison. No compiled ELF/CDI. Makefile/ip.txt and target dependencies are unchanged. ZIP timestamps fixed in the past.

## Physical tilt investigation

Central flat fast-path driving resets physical up to world-up each tick. The wheel-contact solver can retain actual body roll/tilt after bank interaction and until it settles back into the flat path. This can account for observed physical tilt; no hardware capture was supplied to tie a particular observed frame to that mode.

`tests/tilt_audit.c` isolates18 continuous hard-turn cases: left/right at16,33.99,55.45 speed starts; ordinary flat path, initially settled dynamic mode, and dynamic mode seeded with a small5.74-degree lean. Horizontal position is recentered each step to provide an infinite-flat fixture; boost is refilled. This isolates turns from bank/collision encounters. Settled cases returned to upright driving; retained contact-mode cases peaked at16.42–17.54 degrees total tilt. No inverted steps in these isolated cases, before or after this change.

These tests do NOT establish a real-match rollover probability, rule out flips from bank/collision/airborne momentum, or reproduce the user's exact observed turn. No random flip mechanic, physical spring, center-of-mass shift, anti-roll torque or rollover threshold was added or changed. The new suspension is mesh-only, so it cannot itself make the physical car flip. Further physical rollover tuning should be a separate handling experiment if requested.

## Cosmetic suspension

- Append independent `CarSuspension` state for each car after the existing Game fields (56 extra bytes total). Damped springs update at the existing120Hz simulation rate, using velocity changes projected into the physical forward/right basis. They never feed into driving, wheel contacts, AI, collisions, audio or wheel animation.
- Small outward turn lean: bank bounded to +/-.03rad (~1.72deg). Acceleration/brake pitch bounded to +/-.02rad (~1.15deg). The shell rotates about a local.85-height pivot. Landing triggers a bounded compression/rebound; heave is bounded to[-.04,+.025]. The isolated jump test measures-.0261 compression and+.0035 rebound, then settles.
- Targets fade with physical up.y over.94..1, reducing added flex when the contact body already tilts. Wall/air targets return smoothly toward neutral. Landing impulses require an actual near-flat contact transition and inward previous velocity. Cosmetic spring rates cannot rotate the physical up vector.
- Wheels remain on the accepted physical frame; the shell, wheel arches, glass, lights and spoiler share the flexed body frame. Renderer computes the body basis once per car and restores the physical frame only while drawing wheels. Four additional sine/cosine calls per drawn car; no per-vertex trig, assets, extra triangles, transparency or new allocations.
- Wheel openings grow modestly from radius.56 to.62, and adjacent side panels follow the opening edges, providing clearance for the moving shell. Original car proportions and wheel dimensions retained.
- Ball-camera conservative bounds expand to1.60 half-width,1.90 height,2.15 half-length so the visual shell/steered wheels remain covered. Camera distance, aim/orbit, chase projection and smoothing parameters remain; the framing safeguard can widen slightly sooner in constrained views. No physical orientation is replaced by a visual one in the camera or physics.
- Both cars have separate cosmetic states. Pause/goal breaks freeze updates and resets clear springs. Wheel rolling/steering remains as accepted v0.17.

Production changes: game.h/game.c (cosmetic state only), render.h/render.c (body frame and conservative framing). Vehicle solver, main/controller, audio and Makefile remain byte-identical to v0.17. Field/ball/goal sizes, gravity, top speeds, handling and AI rules unchanged.

## Verification and next test

Full strict C99 host checks pass, including new suspension checks and tilt audit, plus prior wheel, handling, camera, eight carving cases,48 wall steering cases,28 stress/recovery cases and ten-minute match checks. `suspension_test.c` verifies mirrored lean, pitch direction, landing rebound/settling, wall fade, bounded values, goal freeze/reset and read-only rendering. It hashes the actual rendered wheel hubs to confirm body flex does not move them. ASan/UBSan passes the suspension/render test.

The72,000-step v0.17/v0.18 comparison is byte-for-byte identical for all588 original Game bytes, INCLUDING v0.17 wheel animation. Only the new56-byte spring suffix differs. Each binary stream42,336,000 bytes, SHA256 `771f0ddef004841c8e9971c5d5c2b09e80e44717a3202b9996ab7b0a16268f37`. Optional `tests/gameplay_compare.c`: build with `-DSUSPENSION_VISUAL` for v0.18, no macro against v0.17 sources. Baseline required separately; not part of run.sh.

`SIMULATION_PREVIEW.mp4`:10s20fps host reference, v0.17 renderer left / v0.18 right with identical simulation and close-up camera script. Four2.5s scenes: boosted slalom, jump/landing, braking, orange CPU jump. Turn, compression and braking frames inspected. Old renderer compiled against the current header for this comparison; no hardware footage/FPS claim. Reference kickoff5016 and corner2534 submitted triangles; sampled clipping/geometry counts, not target performance.

**Next user test:** compile and compare with v0.17 on real hardware. Judge turn lean, brake/acceleration pitch and short landing rebound. Check wheel-arch clearance, CPU body flex, banks/wall steering and physical tilting, jumps/recovery, pause/reset, camera comfort and frame smoothness. If there is an actual unwanted flip, obtain the setup (speed, location, input, previous bank/jump contact) before changing the physical solver. If the user instead wants physically induced rare rollovers, clarify that as a separate handling feature, not a random visual effect. No v0.18 target build, hardware performance or user acceptance yet.

Prior history follows; this section supersedes older status and preview descriptions.

---

# Resume here — v0.17 wheel animation candidate, October 23, 2026

**Accepted real-hardware baseline: v0.16 handling.** User reported: “everything seems to work andfeel good on real hardware.” No numeric FPS was supplied. They approved the next visual step: front-wheel steering and wheel rolling, preserving the accepted handling. v0.17 is a separate full-source candidate; target/user testing is pending.

Build unchanged: in `Vectorball_DC_Wheels_v0.17`, run `make gdemu` -> `gdemu/01/disc.cdi`. Keep v0.16 for comparison. No ELF/CDI included. Makefile/ip.txt/vehicle.o linkage unchanged; timestamps in ZIP are fixed in the past.

## Wheel visuals

- `CarVisual {roll,steer}` is appended for player and CPU in `Game` (16 extra bytes total). These cosmetic fields never feed into forces, contacts, AI, sound, cameras or scoring. `game_step` advances them once per120Hz tick using each car's own input and existing body-forward vector. Rendering consumes state read-only; extra draws or camera changes cannot speed up animation. Pause stops physics ticks and goal breaks hold the cosmetic state. Reset clears both cars' wheel state.
- Signed longitudinal3D speed advances phase as `speed*dt/.50`, wrapped with `fmodf` to keep it bounded. Positive travel rotates forward; reverse rotates backward. This naturally includes wall climbing and descending when facing along travel. It is a cosmetic travel-based estimate, including airborne travel, not independently simulated wheel RPM, differential rotation, traction slip or free-wheel inertia.
- Front wheels pivot about their own hub centers around local up. Rear wheels stay straight. Target angle is input*.28rad, reduced by the accepted speed-sensitive floor steer factor and blended to the existing wall angle over up.y .85..1. A12/s approach smooths steering changes, including steering while parked. The body is not steered by this animation.
- Existing ten-segment wheels and wheel openings are retained. Contrasting rim sectors and hubs rotate about the axle; tire silhouettes remain steady so polygon facets do not wobble as they spin. Spoke contrast softens toward a steel-gray average with longitudinal speed (full contrast at rest,6% minimum by20 units/s) to reduce high-speed strobing/wagon-wheel appearance. This is simple opaque color blending, not a new blur/transparency pass.
- Roll/steer sine/cosine values are cached once per drawn car, not per wheel segment. Cosmetic updates reuse the basis already maintained by the driver. No new assets, textures, meshes, wheel triangles, per-frame allocations or dependencies. CPU renderer now copies its own cosmetic state AND velocity into its existing temporary render view, so orange wheels do not borrow blue wheel phase/contrast.

Production changes: `src/game.h`, `src/game.c`, `src/render.c`. Vehicle solver, main/controller, audio, camera parameters, dimensions, motor/braking/steering laws, banks, jumps and ball physics remain as v0.16. Front wheels can protrude slightly farther sideways while turned; existing conservative camera bounds still pass. No body suspension/lean animation added.

## Verification

The full strict host suite passed. `wheel_visual_test.c` adds checks for stationary steering, smooth reversal, signed distance/radius phase, high-speed steer reduction, wall rolling, independent CPU animation, goal freeze/reset, bounded phase, repeatable read-only rendering and wheel triangle count. ASan/UBSan also passes this test; details in `SIMULATION_RESULTS.txt`.

A cross-version run compared all original gameplay bytes for72,000 identical scripted fixed steps against accepted v0.16, including player, CPU, contact frames, ball, scores, timers, input-held state and sound events. Original `Game` prefix572 bytes is byte-for-byte identical at every step; only the new16-byte visual suffix differs. Aggregate SHA256 of the41,184,000-byte dumps: `dd1f53e69b8df63881e4760ad391131a169b90c0abe762d55e876474097e3890`. The optional `tests/gameplay_compare.c` emits the comparison stream; build with `-DCOSMETIC_WHEELS` for v0.17 or without it against the v0.16 sources. Previous release required separately; this optional comparison is not part of run.sh.

Full host tests retain handling, scale/speeds, eight neutral carves,48 manual wall steering cases,28 vehicle stress cases, ten-minute matches, jumps, scoring/ceiling, audio and camera checks. Kickoff reference5022 triangles, corner2534, matching v0.16 sample counts. These are geometry counts, not target FPS. A final visual-only optimization reuses the stored driver basis; wheel/render tests and sanitizers were rerun afterward.

`SIMULATION_PREVIEW.mp4` is an8s20fps host close-up: v0.16 static wheel rendering left / v0.17 animated wheels right, with the same simulation and camera script. Four2s scenes: stationary steering, low forward throttle, reverse, orange CPU. The old renderer is compiled against the new header for the comparison so both sides use identical accepted gameplay. Inspected front-steer and moving-wheel frames. This is not Dreamcast footage.

**Next user test:** build v0.17 and check wheel direction/steering while parked, accelerating, braking, reversing and on walls; watch orange CPU wheels too. Compare normal chase/ball-camera appearance with v0.16. Check for clipping into arches, flicker at high speed, pause/resume and resets, and hardware smoothness. Approved v0.16 gameplay should feel the same. No v0.17 hardware performance or user-acceptance result yet. Tune visual steering/smoothing/contrast from feedback before expanding features.

Prior history follows; this section supersedes older acceptance/next-step/video status.

---

# Resume here — v0.16 handling candidate, October 23, 2026

**Accepted real-hardware baseline: v0.15 scale/speed.** The user reported: “things are working well. tested on real hardware. I dont see any issues. looks good so far.” No numeric FPS was supplied. User approved the next step: acceleration, braking and turning appropriate to the faster pace, making slowing for tight corners useful while preserving accepted wall handling.

This separate v0.16 full-source candidate is host-tested, not yet user/hardware tested. Keep v0.15 for comparison. Same build: `make gdemu` -> `gdemu/01/disc.cdi`; no ELF/CDI included. The original working Makefile, ip.txt and vehicle.o linkage are unchanged.

## Changed handling

Production changes are confined to `src/game.c`, `src/game.h` and `src/vehicle.c`.

- Ground steering keeps full low-speed lock, then reduces with speed using `clamp(42/(2.15*max(abs(speed),1)), .30, 1)`. Below about19.53 units/s, the prior low-speed steering is retained. At55.45 units/s, intended floor yaw is about.757rad/s instead of2.15. This is original speed-sensitive understeer tuning, not Rocket League's exact steering curve or a full tire simulation. Existing side grip is unchanged; no new handbrake, drift button or oversteer system.
- Ground motor force tapers by12% at the selected speed cap (`1-.12*(speed/cap)^2`), alongside existing .65 rolling drag. Both existing speed caps are still reached. Near-flat wheel contacts share this force calculation. Air motor scaling remains as before.
- Braking uses50 units/s^2 times trigger pressure PLUS existing rolling drag. It takes priority over throttle and clamps the change at zero so brake input cannot itself reverse the car. Opposite throttle brakes first, then can reverse. Light braking always decelerates more than coasting. From55.45 units/s, measured full/half brake stop times are.83/1.38s, distances20.78/32.17 units. These are isolated flat straight-line checks, not universal stopping distances on banks or during impacts.
- Releasing boost or running out no longer clamps existing forward speed immediately from55.45 to33.99. Overspeed carries through and coasts down; ordinary throttle does not add acceleration above its cap. One-step boost release from55.45 yields55.15 instead of v0.15's33.99. Existing airborne steering and jump impulse remain; airborne boost release also benefits from the carry fix.
- Near-flat contact motor forces, front wheel steer angle and transported heading rate blend into the new floor rules for normal.y .85..1. The established steep bank/wall motor, carving and manual yaw laws are unchanged below that range. Existing dynamic-body orientation/solver remains. Changed approach acceleration slightly changes wall entry speed and resulting arc metrics; the behavioral carving/steering checks still pass. Do not claim the entire wall trajectory is numerically identical.

Arena/goal/ball/car dimensions, gravity, ceiling, camera parameters, car/stadium meshes, ball physics, sound code and controller mapping are unchanged from v0.15. No new production files, assets, build dependencies, per-frame allocations or render geometry. CPU uses the same new ground handling; its tactical/braking threshold rules are unchanged. Deterministic kickoff score now10.59s versus10.70s; this is one test scenario, not an overall AI difficulty measure.

## Checks and limits

`sh tests/run.sh` includes the new `handling_test.c`. It checks full/half braking against straight-line stopping expectations, analog/light brake priority, tight vs boosted90-degree turns, speed carry when lifting boost, reverse, tapered acceleration and analog/rest steering. A slow16-unit/s coasting turn has7.69-unit displacement from start at90 degrees versus100.00 units while boosting. Those are two scripted turn spans, not circle radii. Slowing materially tightens the turn.

Direct v0.15/v0.16 comparison: after one simulated second of full-speed full-stick turn, heading changes2.150/.757rad respectively. v0.15 still provides the accepted fallback if the new high-speed turn radius feels too wide.

Full strict C99 checks and sanitizer results are in `SIMULATION_RESULTS.txt`: original eight unsteered carves,48 wall steering/countersteering/release cases,28 vehicle stress/recovery cases including max speed, ten-minute solo/CPU/jump matches, scoring, ceiling/speed, sound, camera and world rendering checks. The isolated flat jump remains2.84. Host results do not establish controller feel or target performance.

The new5s20fps `SIMULATION_PREVIEW.mp4` compares v0.15 left with v0.16 right under identical scripts. First2.5s: full boost while steering to90 degrees; second2.5s: brake to16, then coast through a90-degree turn. Chase camera from the actual renderer. It is a host reference, not emulator/hardware footage. Earlier video descriptions below apply to their historical ZIPs.

**Next user test:** compile and compare with v0.15 on real hardware. Drive fast through a tight turn, then try braking before it. Judge whether high-speed turning is too wide, braking too weak/strong, analog steering/trigger response, acceleration, and the speed carry after boost. Recheck floor-to-bank transition, deliberate wall steering/neutral carve, jump landings and camera comfort. Tune from feedback before adding drift, wheel animations or features. No new hardware FPS result or user acceptance for v0.16 yet.

The older handoffs below are retained for history; acceptance and next-step status above supersede them.

---

# Resume here — v0.15 converted scale/speed candidate, October 23, 2026

**Accepted baseline: v0.14 wall steering.** The user tested v0.14 and said “ok thats perfect. well done.” Device/FPS were not specified. User then supplied a Rocket League specification image, approved a conversion anchored to our current ball diameter, and authorized trying it. This v0.15 is a separate full-source candidate; user testing is pending.

## Conversion and scope

One Rocket League uu maps to `RL_SCALE = 2.2/91.25 = .024109589` Vectorball units, with seconds unchanged. Distances, speeds and gravity all use this same multiplier. Dimensions are taken from RLBot's documented game values: https://wiki.rlbot.org/v5/botmaking/useful-game-values/ . The screenshot's width8240 is corrected to8192uu (walls +/-4096). Reference values are converted; no external game code/assets imported.

| Quantity | Accepted v0.14 | v0.15 target |
|---|---:|---:|
| Full field width x length |120 x160|197.50575 x246.88219|
| Ceiling height |None|49.37644|
| Goal width x height |24 x8|43.04791 x15.49704|
| Ball diameter |4.4|4.4|
| Throttle speed cap |25|33.99452|
| Boosted speed cap |37|55.45205|
| Ball speed cap |52 horizontal only|144.65753 total 3D|
| Car / ball gravity |22 /18|15.67123 both|
| Kickoff distance from center |38|111.097 (4608uu)|

The shared header names speed, acceleration, gravity and conversion settings. Flat and wheel-contact motor paths use the same new speed/acceleration values. Drive acceleration is20*(new throttle cap/25), boost acceleration34*(new boost cap/37), preserving the earlier style of response. These are original first-pass tuning choices, not Rocket League's actual acceleration/braking curves. Reverse cap10, grip, steering, boost recharge/depletion, CPU decision rules, ball restitution/drag and hit response stay as before. CPU shares the new motor settings but its tactical/braking thresholds have not been retuned; deterministic kickoff scoring takes10.70s versus5.53s. Treat that as a single scenario, not overall difficulty.

Current car mesh and collision dimensions are retained. Car-to-ball proportions are therefore NOT yet matched to a Rocket League hitbox. Original car models, current 6x3 curved banks, corner radius14, camera parameters and jump impulse9.5 are retained. Lower gravity raises the measured flat jump from2.01 to2.84 and produces higher/wider passive wall arcs. This is a field/speed/gravity experiment, not a complete Rocket League physics recreation. Choose a specific car/hitbox reference and tune jump/acceleration/ball hits later after this test.

The existing open mesh fence now reaches the converted height; side wheel probes reach that height too. A collision ceiling returns balls (same .64 vertical restitution as floor) and bounds flat/dynamic car bodies; no opaque roof or ceiling-driving controls. Dynamic car ceiling contact uses the existing body spheres. The larger scene retains the existing floor tile count, skyline/building count and car models. Longer perimeter fencing adds posts under the existing per-distance post rule: kickoff reference5022 triangles versus4290 previously; no FPS result implied.

New total ball speed limit runs before integration and after contacts. Wheel-body velocity is capped before integration and after body collisions. Higher-speed stress exposed a post-collision spin spike above20rad/s; post-collision spin now has a16rad/s bound, while the existing pre-integration8rad/s bound remains. A rare goal-side seam residual left the car root .051 units beyond the rectangular end boundary during a ten-minute match; an inward .06-unit convex root backstop after the finite sphere passes closes that gap without replacing wheel/body contacts. Existing below-crossbar car goal restriction remains.

## Validation and next step

Run `sh tests/run.sh`. Full strict C99 output and sanitizer scope are in `SIMULATION_RESULTS.txt`. New scale/speed checks exercise actual top speeds, shared gravity, diagonal/vertical total ball limits, ball and car ceiling contacts, high wall probing, 24 maximum-speed shots and flat jump height. Stress speeds now include33.99 and55.45; original eight unsteered arcs and48 manual steering/countersteering/release cases remain. Jump height expectation follows gravity; landing sound test now waits for the longer complete flight instead of checking before touchdown. No test was removed.

Reference renders of kickoff, ball camera and corner were inspected. `SIMULATION_PREVIEW.mp4` is a12s,20fps host render of two wall steering/countersteering cases under the new settings, not hardware footage. The previous handoff's video description applies only to v0.14.

**Next user test:** extract beside v0.14, run `make gdemu` in `Vectorball_DC_Scale_Speed_v0.15`, load `gdemu/01/disc.cdi`. Compare field/goal scale, acceleration/boost pace, travel time, gravity/jump hang time, ball speed/ceiling rebounds, wall steering/arcs, cameras, CPU pacing and hardware FPS. Keep v0.14 for comparison. No Dreamcast compiler/emulator here; no compiled ELF/CDI, controller-feel acceptance or hardware performance claim for v0.15.

Do not claim exact Rocket League feel or proportions. Tune from this test before adding features. Prior implementation/history follows, superseded by this section where status differs.

---

# Resume here — v0.14 manual wall steering candidate, October 23, 2026

**User-accepted baseline remains v0.13's neutral-stick carving.** This session implements the outstanding deliberate wall steering correction as a separate v0.14 source candidate. No user playtest of v0.14 has happened yet. Build with the unchanged `make gdemu` -> `gdemu/01/disc.cdi` workflow. No ELF/CDI or target performance result is supplied.

## Findings and change

Reproduced steering/countersteering against a live ascent, crest and descent on both long walls. In v0.13, disabling velocity-follow assist alone does not give the player priority: the transported target can retain an old assisted heading, while tire torque, existing spin and longitudinal-speed scaling oppose the requested turn. Near a crest, longitudinal speed can drop or change sign even while the car still travels along the wall, making countersteering stall or reverse sense.

Only `src/vehicle.c` changes production behavior. With at least two contacts, a steep contact normal (normal.y < .7) and stick magnitude >= .1, manual steering rebases the target on the actual body heading. A bounded yaw-rate motor replaces the net torque about the contact normal, retaining pitch/roll contact forces and the existing integrated rigid-body orientation. Desired rate uses tangent-plane speed, existing 2.15 rad/s full-rate constant and speed/7 scaling. Explicit reverse input with negative forward speed retains reverse steering sense. This is an original arcade wall-control policy: passive backward slip during a crest does not invert the player's steering. The torque command is clamped to +/-65, with gain25 and angular-drag compensation3.04. No pose snapping or direct velocity rotation is added.

At neutral, the existing carving logic resumes from the updated target through its existing bounded heading torque; there is no new release timer. The existing eight neutral-stick carving cases retain exactly their previous reported entry/descent/peak metrics. Flat driving, low-bank steering, jumps, sound, cameras, models, field/ball sizes and build recipe are unchanged. No new state fields or dependencies.

## Validation and next playtest

New `tests/wall_steering_test.c` runs 48 cases: both walls, both travel directions, ascent/crest/descent, left/right, full/35% input, opposite input then three seconds at neutral. It requires signed turn response, wheel contact during the first turn, and bounded orientation/position changes through countersteering/release. Final observed minimum signed turn/input .360 radians over .4 seconds; maximum orientation step2.04 degrees. The same test compiled against v0.13 fails its countersteering response assertion, confirming the new check detects the earlier defect. Tests check behavior, not controller feel.

All strict host checks and sanitizer status are recorded in `SIMULATION_RESULTS.txt`. Host comparison video `SIMULATION_PREVIEW.mp4` shows v0.13 left / v0.14 right, using the same new input script and renderer. First scene: mirrored travel direction +1, right input at crest for .4s then left for .4s; second scene: direction -1, same input sequence during descent. Approach drive/boost ends above height3; otherwise coast. This is a software reference, not Dreamcast footage.

**Next:** compile and playtest v0.14. Check deliberate left/right steering while climbing, steering against the descent, quick countersteering, partial stick and release to neutral. Confirm the accepted unsteered carve still feels right, then jumps, corners, camera comfort and hardware smoothness. Tune response from that feedback before expanding features. Only user playtesting can establish the correction's feel and target performance.

The earlier v0.13 handoff below is retained for history. Its 'no steering fix implemented' statement describes the starting point of this session, superseded by this v0.14 candidate.

---

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
