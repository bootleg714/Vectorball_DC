#include "render.h"
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static uint64_t hash;static int triangles;
static void number(float f){union {float f;uint32_t bits;} u;u.f=f;hash^=u.bits;hash*=UINT64_C(1099511628211);}
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    assert(isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z)&&a.z>0);
    assert(isfinite(b.x)&&isfinite(b.y)&&isfinite(b.z)&&b.z>0);
    assert(isfinite(c.x)&&isfinite(c.y)&&isfinite(c.z)&&c.z>0);
    number(a.x);number(a.y);number(a.z);number(b.x);number(b.y);number(b.z);number(c.x);number(c.y);number(c.z);
    hash^=color;hash*=UINT64_C(1099511628211);triangles++;
}
static uint64_t frame(const Game *g){
    Camera c={0};Game before=*g;camera_update(&c,g,0,0,1);
    hash=UINT64_C(1469598103934665603);triangles=0;render_frame(g,&c,0,0);
    assert(triangles>1000&&memcmp(&before,g,sizeof(*g))==0);return hash;
}
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0,0,0};g->ball=(Vec3){70,BALL_R,0};}
int main(void){
    Game g,a,b;int i;uint64_t still,rolled,steered;int count;
    solo(&g);
    for(i=0;i<60;i++)game_step(&g,(Input){1,0,0,0,0,0},STEP);
    assert(g.car_visual.steer>.275f&&g.car_visual.steer<=.28f&&g.car_visual.roll==0&&g.heading==0);
    {float previous=g.car_visual.steer;
        game_step(&g,(Input){-1,0,0,0,0,0},STEP);
        assert(g.car_visual.steer<previous&&g.car_visual.steer>-.28f);
    }
    solo(&a);b=a;a.velocity.z=5;b.velocity.z=-5;
    game_step(&a,(Input){0},STEP);game_step(&b,(Input){0},STEP);
    assert(a.car_visual.roll>0&&b.car_visual.roll<0);
    assert(fabsf(a.car_visual.roll-a.velocity.z*STEP/.5f)<.001f);
    solo(&g);g.velocity.z=BOOST_SPEED;
    for(i=0;i<30;i++){g.car=(Vec3){0,0,0};g.heading=0;g.velocity=(Vec3){0,0,BOOST_SPEED};game_step(&g,(Input){1,1,0,1,0,0},STEP);}
    assert(g.car_visual.steer>.135f&&g.car_visual.steer<.150f&&fabsf(g.car_visual.roll)<6.28319f);
    /* The wheel phase uses actual 3D body-forward motion on walls too. */
    solo(&g);g.car=(Vec3){ARENA_X-.06f,6,0};g.car_surface.dynamic=1;
    g.car_surface.up=(Vec3){-1,0,0};g.car_surface.forward=(Vec3){0,1,0};g.velocity=(Vec3){0,10,0};
    game_step(&g,(Input){1,0,0,0,0,0},STEP);assert(g.car_visual.roll>0&&g.car_visual.steer>0);
    game_init(&g);g.car.x=40;
    for(i=0;i<120;i++)game_step(&g,(Input){0},STEP);
    assert(g.car_visual.roll==0&&fabsf(g.cpu_visual.roll)>.01f);
    g.reset_timer=1;{CarVisual player=g.car_visual,cpu=g.cpu_visual;
        game_step(&g,(Input){1,1,0,1,0,0},STEP);
        assert(memcmp(&player,&g.car_visual,sizeof(player))==0&&memcmp(&cpu,&g.cpu_visual,sizeof(cpu))==0);
    }
    game_reset(&g);assert(g.car_visual.roll==0&&g.car_visual.steer==0&&g.cpu_visual.roll==0&&g.cpu_visual.steer==0);
    /* Rendering consumes state without advancing it; CPU state is independent. */
    solo(&g);still=frame(&g);count=triangles;assert(frame(&g)==still);
    g.car_visual.roll=.31f;rolled=frame(&g);assert(rolled!=still&&triangles==count);
    g.car_visual.steer=.28f;steered=frame(&g);assert(steered!=rolled&&triangles==count);
    g.cpu_visual.roll=.7f;assert(frame(&g)==steered);
    g.cpu_enabled=1;g.cpu=(Vec3){4,0,2};still=frame(&g);
    g.cpu_visual.roll=1.1f;assert(frame(&g)!=still);
    puts("PASS: stationary front-wheel steer, smooth countersteer, signed travel/radius roll, high-speed steer, wall rolling, independent CPU wheels, goal freeze/reset, bounded phase, repeatable read-only finite rendering and unchanged wheel triangle count");
    return 0;
}
