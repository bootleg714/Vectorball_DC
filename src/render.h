#ifndef VECTORBALL_RENDER_H
#define VECTORBALL_RENDER_H
#include "game.h"
#include "frontend.h"
#include "match.h"
#include <stdint.h>
#define BALL_CAM_DISTANCE 11.5f
#define CAR_FRAME_WIDTH 1.60f
#define CAR_FRAME_HEIGHT 1.90f
#define CAR_FRAME_LENGTH 2.15f
typedef struct {float x,y,z;} Screen;
typedef struct {
    Vec3 eye,right,up,forward,previous_car;
    float focal,orbit_yaw,distance,height;
    int initialized,mode;
} Camera;
/* Backend receives clipped, projected triangles. z is reciprocal camera depth. */
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color);
/* Set by the renderer around single-screen floor markings; the Dreamcast backend
   then submits them with a depth-compare ALWAYS header (host backends ignore it). */
extern int render_depth_always;
void camera_update(Camera *c,const Game *g,int ball_camera,float dt,int snap);
void render_match_clock(const Match *m);
void render_results(const Game *g,int selection,int connected,int artwork);
void render_frontend(FrontPage page,int selection,int connected,int artwork);
void camera_update_player(Camera *c,const Game *g,int player,int ball_camera,float dt,int snap);
void render_split_fps(float fps);
void render_split_frame(const Game *g,const Camera cameras[2],const Match *m,const int ball_camera[2],int paused,int missing);
void render_frame(const Game *g,const Camera *c,int paused,int ball_camera);
#endif
