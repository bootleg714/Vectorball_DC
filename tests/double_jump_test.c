#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const Input idle={0},jump={0,0,0,0,1,0};
static void setup(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0};g->ball=(Vec3){30,BALL_R,30};}
int main(void){
    Game g;int i;float peak=0,vy;
    setup(&g);game_step(&g,jump,STEP);assert(g.car_surface.air_jump_available==1);
    for(i=0;i<60;i++)game_step(&g,jump,STEP);
    game_step(&g,idle,STEP);vy=g.velocity.y;game_clear_events(&g);game_step(&g,jump,STEP);
    assert(g.velocity.y>vy+7&&g.sounds.jump==1&&!g.car_surface.air_jump_available);
    game_step(&g,idle,STEP);vy=g.velocity.y;game_step(&g,jump,STEP);assert(g.velocity.y<vy);
    for(i=0;i<360;i++){game_step(&g,jump,STEP);peak=fmaxf(peak,g.car.y);}
    assert(peak>5&&g.car.y==0&&g.velocity.y==0&&!g.car_surface.air_jump_available);
    game_step(&g,idle,STEP);game_step(&g,jump,STEP);assert(g.car_surface.air_jump_available);
    /* Rolled body path starts a pitch flip with the same one extra WORLD-UP hop. */
    for(i=0;i<20;i++)game_step(&g,(Input){1,0,0,0,1,0},STEP);
    assert(g.car_surface.dynamic&&g.car_surface.air_roll_active>0);
    game_step(&g,idle,STEP);vy=g.velocity.y;game_step(&g,jump,STEP);assert(g.velocity.y>vy+7&&!g.car_surface.air_jump_available);
    /* Wall jump can also earn one air hop. */
    setup(&g);g.car=(Vec3){ARENA_X-12,0,0};g.heading=1.5707963268f;g.velocity.x=20;
    for(i=0;i<360;i++){game_step(&g,(Input){0,1,0,1,0,0},STEP);if(g.car.y>5&&g.car_surface.grounded_wheels>=2)break;}
    assert(i<360);game_step(&g,jump,STEP);assert(g.car_surface.air_jump_available);
    game_step(&g,idle,STEP);vy=g.velocity.y;game_step(&g,jump,STEP);assert(g.velocity.y>fmaxf(vy,0)+7&&!g.car_surface.air_jump_available);
    /* A fall without a first jump cannot mint a free hop. */
    setup(&g);g.car.y=12;game_step(&g,jump,STEP);assert(g.velocity.y<0&&!g.car_surface.air_jump_available);
    /* Player two has independent jumps and recovery, no AI input. */
    game_init(&g);game_step_two(&g,idle,jump,STEP);assert(g.cpu_surface.air_jump_available&&!g.car_surface.air_jump_available);
    game_step_two(&g,idle,idle,STEP);vy=g.cpu_velocity.y;game_step_two(&g,idle,jump,STEP);assert(g.cpu_velocity.y>vy+7&&!g.cpu_surface.air_jump_available&&g.car.y==0);
    game_step_two(&g,idle,idle,STEP);vy=g.cpu_velocity.y;game_step_two(&g,idle,jump,STEP);assert(g.cpu_velocity.y<vy);
    game_reset(&g);assert(!g.car_surface.air_jump_available&&!g.cpu_surface.air_jump_available);
    g.car.x=40;g.cpu=(Vec3){0,1.5f,0};g.cpu_surface.dynamic=1;g.cpu_surface.up=(Vec3){0,-1,0};g.cpu_surface.forward=(Vec3){0,0,-1};
    for(i=0;i<120;i++)game_step_two(&g,idle,idle,STEP);
    game_step_two(&g,idle,jump,STEP);assert(g.cpu_surface.recovery_time>0&&!g.cpu_surface.air_jump_available);
    for(i=0;i<240;i++)game_step_two(&g,idle,idle,STEP);
    assert(g.cpu_surface.up.y>.98f&&g.cpu.y<.1f);
    printf("PASS: single extra edge-triggered hop, no held repeat/third/fall hop, %.2f peak, landing recharge, physical flip/air-roll/wall paths, independent P2 hop/recovery and reset\n",peak);return 0;
}
