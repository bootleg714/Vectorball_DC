#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static uint64_t hash,hub_hash;static int hubs;
static void number(uint64_t *h,float f){union {float f;uint32_t bits;} u;u.f=f;*h^=u.bits;*h*=UINT64_C(1099511628211);}
void backend_triangle(Screen a,Screen b,Screen c,uint32_t col){
    assert(isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z)&&a.z>0);
    assert(isfinite(b.x)&&isfinite(b.y)&&isfinite(b.z)&&b.z>0);
    assert(isfinite(c.x)&&isfinite(c.y)&&isfinite(c.z)&&c.z>0);
    number(&hash,a.x);number(&hash,a.y);number(&hash,b.x);number(&hash,b.y);number(&hash,c.x);number(&hash,c.y);hash^=col;
    if(col==0xff263b51){number(&hub_hash,a.x);number(&hub_hash,a.y);number(&hub_hash,a.z);number(&hub_hash,b.x);number(&hub_hash,b.y);number(&hub_hash,b.z);number(&hub_hash,c.x);number(&hub_hash,c.y);number(&hub_hash,c.z);hubs++;}
}
static uint64_t frame(Game *g){Camera c={0};Game before=*g;camera_update(&c,g,0,0,1);hash=hub_hash=UINT64_C(1469598103934665603);hubs=0;render_frame(g,&c,0,0);assert(memcmp(&before,g,sizeof(*g))==0);return hash;}
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0,0,0};g->ball=(Vec3){70,BALL_R,0};}
static void bounded(CarSuspension s){assert(isfinite(s.bank)&&isfinite(s.pitch)&&isfinite(s.heave));assert(fabsf(s.bank)<=.03001f&&fabsf(s.pitch)<=.02001f&&s.heave>=-.04001f&&s.heave<=.02501f);}
int main(void){
    Game a,b,g;int i,landed=0;float compression=0,rebound=0;
    solo(&a);solo(&b);a.velocity.z=b.velocity.z=BOOST_SPEED;
    for(i=0;i<120;i++){
        game_step(&a,(Input){1,1,0,1,0,0},STEP);game_step(&b,(Input){-1,1,0,1,0,0},STEP);
        a.car.x=a.car.z=b.car.x=b.car.z=0;a.boost=b.boost=1;bounded(a.car_suspension);bounded(b.car_suspension);
        assert(a.car_surface.up.y==1&&b.car_surface.up.y==1);
    }
    assert(a.car_suspension.bank>.015f&&b.car_suspension.bank<-.015f);
    assert(fabsf(a.car_suspension.bank+b.car_suspension.bank)<.001f);
    solo(&a);solo(&b);a.velocity.z=BOOST_SPEED;
    for(i=0;i<30;i++){game_step(&a,(Input){0,0,1,0,0,0},STEP);game_step(&b,(Input){0,1,0,0,0,0},STEP);}
    assert(a.car_suspension.pitch>.005f&&b.car_suspension.pitch<-.002f);
    solo(&g);game_step(&g,(Input){0,0,0,0,1,0},STEP);
    for(i=0;i<360;i++){
        game_step(&g,(Input){0},STEP);bounded(g.car_suspension);
        if(g.car.y==0)landed=1;
        if(landed){compression=fminf(compression,g.car_suspension.heave);rebound=fmaxf(rebound,g.car_suspension.heave);}
    }
    assert(landed&&compression<-.01f&&rebound>.0005f&&fabsf(g.car_suspension.heave)<.0001f);
    solo(&g);g.car=(Vec3){ARENA_X-.06f,10,0};g.car_surface.dynamic=1;g.car_surface.up=(Vec3){-1,0,0};g.car_surface.forward=(Vec3){0,1,0};
    g.car_suspension.bank=.02f;g.car_suspension.pitch=.01f;
    for(i=0;i<60;i++)game_step(&g,(Input){0},STEP);
    assert(fabsf(g.car_suspension.bank)<.001f&&fabsf(g.car_suspension.pitch)<.001f);
    solo(&g);g.car_visual.roll=.31f;g.car_visual.steer=.28f;
    {uint64_t baseline=frame(&g),wheel=hub_hash;assert(hubs==40);
        g.car_suspension.bank=.03f;g.car_suspension.pitch=.02f;g.car_suspension.heave=-.04f;
        assert(frame(&g)!=baseline&&hub_hash==wheel&&hubs==40);
        g.reset_timer=1;{CarSuspension held=g.car_suspension;game_step(&g,(Input){1,1,0,1,0,0},STEP);assert(memcmp(&held,&g.car_suspension,sizeof(held))==0);}
    }
    game_reset(&g);assert(g.car_suspension.bank==0&&g.car_suspension.heave==0&&g.cpu_suspension.heave==0);
    printf("PASS: mirrored bounded turn lean, acceleration/brake pitch, landing compression %.4f/rebound %.4f and settling, wall fade, fixed physical up/wheel hubs, read-only renderer and goal freeze/reset\n",compression,rebound);
    return 0;
}
