Vectorball DC 0.8 - Night city stadium

Extract into your existing Vectorball_DC folder, merge src and replace matching
files. Keep your working Makefile, ip.txt and other game sources.
Only src/render.c changes the game; other files are updated notes.

In DreamSDK:

    touch src/render.c
    make gdemu

Run: gdemu/01/disc.cdi

Added:
- Navy night sky, fixed stars and a moon.
- City skyline with sparse lit windows and landmark towers.
- Spectator terraces, perimeter fencing and floodlight banks.
- Greener pitch, stronger blue/orange goal interiors and painted goal areas.

Field size, camera, physics, controls, cars and audio are unchanged.
Lighting appearance uses opaque colored geometry, without a new lighting or
transparency system. No new files need to be copied onto the disc.

Host reference views and regression checks passed. A 600-view rendering sweep
submitted 2,580-4,356 triangles with finite coordinates. That is geometry count,
not a measured hardware frame rate. Dreamcast build/visual/FPS testing is next.

Please check that the ball stays easy to see and that corners/high ball-camera
views remain smooth on real hardware.
