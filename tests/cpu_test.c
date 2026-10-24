#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const Input idle={0};
static void run(Game *g,int n){int i;for(i=0;i<n;i++)game_step(g,idle,STEP);}
int main(void){
    Game g;int i,saw_reverse=0;float first_goal;
    game_init(&g);assert(g.cpu_enabled&&g.cpu.z==SPAWN_Z&&g.cpu_boost==1);
    g.car.x=18;
    for(i=0;i<120*12&&!g.orange_score;i++)game_step(&g,idle,STEP);
    assert(g.orange_score==1&&g.blue_score==0);first_goal=i*STEP;
    run(&g,250);assert(g.reset_timer==0);assert(g.orange_score==1);
    g.cpu_reverse=.5f;g.cpu_attack=.5f;g.cpu_boost=0;game_reset(&g);
    assert(g.cpu.z==SPAWN_Z&&g.cpu_velocity.x==0&&g.cpu_velocity.z==0);
    assert(g.cpu_reverse==0&&g.cpu_attack==0&&g.cpu_stuck==0&&g.cpu_boost==1&&g.orange_score==1);
    /* Wrong side of ball: recover around it instead of shooting at own goal. */
    game_init(&g);g.car.x=18;g.cpu=(Vec3){0,0,-5};g.cpu_heading=0;run(&g,120*35);
    assert(g.blue_score==0);assert(g.ball.z< -5||g.orange_score>0);
    /* Facing a side wall: reversing recovery must engage and escape. */
    game_init(&g);g.car.x=-18;g.cpu=(Vec3){ARENA_X-1.8f,0,10};g.cpu_heading=1.5707963f;
    for(i=0;i<120*15;i++){game_step(&g,idle,STEP);if(g.cpu_reverse>0)saw_reverse=1;}
    assert(saw_reverse);assert(g.cpu.x<ARENA_X-6);
    /* Opposing cars exchange impulse instead of passing through each other. */
    game_init(&g);g.ball=(Vec3){18,BALL_R,25};g.car=(Vec3){0,0,0};g.cpu=(Vec3){0,0,3};
    g.velocity.z=15;g.cpu_velocity.z=-15;game_step(&g,idle,STEP);
    assert(g.velocity.z<10&&g.cpu_velocity.z> -10);
    assert(hypotf(g.cpu.x-g.car.x,g.cpu.z-g.car.z)>3.29f);
    /* Exact overlapping cars have a deterministic finite separation. */
    g.cpu=g.car;game_step(&g,idle,STEP);assert(isfinite(g.cpu.x)&&hypotf(g.cpu.x-g.car.x,g.cpu.z-g.car.z)>3.29f);
    game_init(&g);
    for(i=0;i<120*600;i++){
        Input in={sinf(i*.003f),1,(i%900<50)?1:0,i%500<200,0,0};game_step(&g,in,STEP);
        assert(isfinite(g.cpu.x)&&isfinite(g.cpu.z)&&isfinite(g.cpu_heading));
        assert(isfinite(g.ball.x)&&isfinite(g.ball.z)&&isfinite(g.ball.y));
        assert(fabsf(g.cpu.x)<=ARENA_X+.05f&&fabsf(g.cpu.z)<=ARENA_Z+.05f);
        assert(fabsf(g.car.x)<=ARENA_X+.05f&&fabsf(g.car.z)<=ARENA_Z+.05f);
        assert(g.cpu_boost>=0&&g.cpu_boost<=1);
    }
    printf("PASS: CPU scored in %.2fs; reset, wrong-side approach, wall escape, bumps and 10-minute two-car stability\n",first_goal);
    return 0;
}
