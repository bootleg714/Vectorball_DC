#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void run(Game *g,Input in,int n){int i;for(i=0;i<n;i++)game_step(g,in,STEP);}
static void init_solo(Game *g){game_init(g);g->cpu_enabled=0;}
int main(void){
    Game g;Input idle={0},gas={0,1,0,0,0,0},boost={0,1,0,1,0,0};float ordinary;
    init_solo(&g);run(&g,idle,1200);assert(fabsf(g.ball.y-BALL_R)<.001f);assert(g.blue_score==0);
    init_solo(&g);g.car.x=12;run(&g,gas,100);ordinary=g.velocity.z;
    init_solo(&g);g.car.x=12;run(&g,boost,100);assert(g.velocity.z>ordinary);assert(g.boost<1);
    /* Keep a run-up gap when ball size changes; the car must not start overlapping. */
    init_solo(&g);g.car.z=-(BALL_R+1.65f+1);
    {int i;float peak=BALL_R;for(i=0;i<60;i++){game_step(&g,gas,STEP);peak=fmaxf(peak,g.ball.y);}
        assert(g.ball_velocity.z>0&&peak>BALL_R+.01f);
    }
    init_solo(&g);g.ball=(Vec3){0,BALL_R,ARENA_Z+BALL_R+.1f};game_step(&g,idle,STEP);
    assert(g.blue_score==1&&g.reset_timer>0);run(&g,idle,300);assert(g.blue_score==1&&g.car.z==-SPAWN_Z);
    init_solo(&g);g.ball=(Vec3){0,BALL_R,-ARENA_Z-BALL_R-.1f};game_step(&g,idle,STEP);assert(g.orange_score==1);
    init_solo(&g);g.ball=(Vec3){GOAL_W-1,BALL_R,ARENA_Z-BALL_R+.2f};g.ball_velocity.z=20;
    game_step(&g,idle,STEP);assert(g.blue_score==0&&g.ball_velocity.z<0);
    init_solo(&g);g.ball=(Vec3){0,GOAL_H,ARENA_Z-BALL_R+.2f};g.ball_velocity.z=20;
    game_step(&g,idle,STEP);assert(g.blue_score==0&&g.ball_velocity.z<0);
    init_solo(&g);g.car.x=ARENA_X-1.9f;g.heading=1.5707963f;run(&g,boost,600);assert(g.car.x<=ARENA_X-.05f);
    init_solo(&g);g.car=g.ball;g.car.y=0;game_step(&g,idle,STEP);assert(isfinite(g.ball.x)&&isfinite(g.ball.z));
    init_solo(&g);
    {int i;for(i=0;i<120*600;i++){
        Input in={sinf(i*.003f),1,(i%900<50)?1:0,i%500<200,0,0};game_step(&g,in,STEP);
        assert(isfinite(g.car.x)&&isfinite(g.ball.x)&&isfinite(g.ball.y)&&isfinite(g.ball.z));
        assert(fabsf(g.car.x)<=ARENA_X+.05f&&fabsf(g.car.z)<=ARENA_Z+.05f);
    }}
    /* The old end line and side limit are now playable interior space. */
    init_solo(&g);g.ball=(Vec3){0,BALL_R,64};g.ball_velocity.z=5;
    game_step(&g,idle,STEP);assert(g.blue_score==0&&g.reset_timer==0&&g.ball_velocity.z>0);
    init_solo(&g);g.car.x=37;g.heading=1.5707963f;run(&g,gas,30);assert(g.car.x>37);
    /* A shot near the new wider post opening scores without a phantom old post. */
    init_solo(&g);g.ball=(Vec3){GOAL_W-BALL_R-.1f,BALL_R,ARENA_Z+BALL_R+.1f};
    game_step(&g,idle,STEP);assert(g.blue_score==1);
    puts("PASS: rest, throttle/boost, impact, both goals/reset, posts/crossbar, enlarged interior/wider goal, walls, overlap, 10-minute stability");return 0;
}
