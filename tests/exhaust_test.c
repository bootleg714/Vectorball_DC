#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int triangles;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    (void)color;assert(isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z)&&a.z>0);assert(isfinite(b.x)&&isfinite(b.y)&&isfinite(b.z)&&b.z>0);assert(isfinite(c.x)&&isfinite(c.y)&&isfinite(c.z)&&c.z>0);triangles++;
}
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0};g->ball=(Vec3){30,BALL_R,30};}
int main(void){
    Game g;Camera c={0};Input boost={0,1,0,1,0,0};int i,base;float slow;
    solo(&g);game_step(&g,boost,STEP);slow=g.exhaust.strength;assert(slow>=.65f&&slow<.67f);
    g.velocity.z=BOOST_SPEED;game_step(&g,boost,STEP);assert(g.exhaust.strength>slow&&g.exhaust.strength<=1);
    game_step(&g,(Input){0,1,0,0,0,0},STEP);assert(g.exhaust.strength==0&&g.exhaust.phase==0);
    game_step(&g,boost,STEP);game_step(&g,(Input){0,1,1,1,0,0},STEP);assert(g.exhaust.strength==0);
    game_step(&g,(Input){0,-1,0,1,0,0},STEP);assert(g.exhaust.strength==0);
    g.boost=0;game_step(&g,boost,STEP);assert(g.exhaust.strength==0);
    g.boost=.001f;game_step(&g,boost,STEP);assert(g.exhaust.strength==0);
    solo(&g);g.car.y=8;game_step(&g,boost,STEP);assert(g.exhaust.strength>0);
    for(i=0;i<300;i++){game_step(&g,boost,STEP);assert(g.exhaust.phase>=0&&g.exhaust.phase<1&&g.exhaust.strength>=0&&g.exhaust.strength<=1);}
    game_reset(&g);assert(g.exhaust.strength==0&&g.cpu_exhaust.strength==0);
    /* Real CPU decision activates its own jet; it is not coupled to player input. */
    game_init(&g);g.car.x=50;g.cpu=(Vec3){0,0,35};g.cpu_attack=1;
    game_step(&g,(Input){0},STEP);assert(g.cpu_exhaust.strength>0&&g.exhaust.strength==0);
    g.cpu_enabled=0;game_step(&g,(Input){0},STEP);assert(g.cpu_exhaust.strength==0);
    solo(&g);g.ball=(Vec3){0,BALL_R,10};camera_update(&c,&g,0,0,1);
    triangles=0;render_frame(&g,&c,0,0);base=triangles;
    g.exhaust=(BoostExhaust){1,.5f};
    {Game before=g;triangles=0;render_frame(&g,&c,0,0);assert(triangles==base+24&&memcmp(&before,&g,sizeof(g))==0);}
    g.cpu_enabled=1;g.cpu=(Vec3){4,0,0};g.cpu_exhaust=(BoostExhaust){1,.5f};
    triangles=0;render_frame(&g,&c,0,0);base=triangles;g.cpu_exhaust.strength=0;
    triangles=0;render_frame(&g,&c,0,0);assert(triangles==base-24);
    g.exhaust.strength=1;g.ball=(Vec3){0,BALL_R,ARENA_Z+BALL_R+.1f};game_step(&g,boost,STEP);
    assert(g.sounds.goal&&g.exhaust.strength==0&&g.cpu_exhaust.strength==0);
    g.exhaust.strength=1;game_step(&g,boost,STEP);assert(g.exhaust.strength==0);
    puts("PASS: boost exhaust throttle/speed response, immediate release/brake/reverse/empty/depletion cut, aerial/CPU operation, bounded fixed-tick pulse, reset/goal clear and 24 finite read-only triangles per boosting car");return 0;
}
