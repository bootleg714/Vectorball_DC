#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const Input idle={0},jump={0,0,0,0,1,0};
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car.x=15;}
int main(void){
    Game g;float peak=0;int i,landed=0;
    solo(&g);game_step(&g,jump,STEP);assert(g.car.y>0&&g.velocity.y>9);
    for(i=0;i<240;i++){
        game_step(&g,jump,STEP);if(g.car.y>peak)peak=g.car.y;
        if(i>10&&g.car.y==0)landed=1;
        if(landed)assert(g.car.y==0&&g.velocity.y==0);
    }
    /* Lower gravity intentionally raises the unchanged launch impulse. */
    assert(fabsf(peak-JUMP_SPEED*JUMP_SPEED/(2*CAR_GRAVITY))<.08f&&landed);
    game_step(&g,idle,STEP);game_step(&g,jump,STEP);assert(g.car.y>0);
    /* A fresh airborne press adds one extra hop; a third cannot add lift. */
    game_step(&g,idle,STEP);{float vy=g.velocity.y;game_step(&g,jump,STEP);assert(g.velocity.y>vy+7&&g.car_surface.air_jump_available==0);}
    game_step(&g,idle,STEP);{float vy=g.velocity.y;game_step(&g,jump,STEP);assert(g.velocity.y<vy);}
    /* Airborne contact can strike a ball too high for the grounded car. */
    solo(&g);g.car=(Vec3){0,2,0};g.velocity=(Vec3){0,3,16};
    g.ball=(Vec3){0,BALL_R+1.85f,3};game_step(&g,idle,STEP);
    assert(g.ball_velocity.z>10&&g.ball_velocity.y>0);
    solo(&g);g.car=(Vec3){0,0,0};g.velocity.z=16;g.ball=(Vec3){0,BALL_R+1.85f,3};
    game_step(&g,idle,STEP);assert(g.ball_velocity.z==0);
    /* Clear an upright opponent instead of taking a horizontal bump in midair. */
    game_init(&g);g.car=(Vec3){0,2.2f,0};g.velocity=(Vec3){0,0,15};g.cpu=(Vec3){0,0,3};g.cpu_velocity.z=-15;
    g.ball=(Vec3){18,BALL_R,20};game_step(&g,idle,STEP);assert(g.velocity.z>14&&g.cpu_velocity.z< -14);
    /* AI uses the same jump impulse for an aligned, nearby airborne ball. */
    game_init(&g);g.car.x=18;g.cpu=(Vec3){0,0,5};g.ball=(Vec3){0,BALL_R+1.85f,0};
    game_step(&g,idle,STEP);assert(g.cpu.y>0&&g.cpu_velocity.y>9);
    g.blue_score=2;game_reset(&g);assert(g.car.y==0&&g.cpu.y==0&&g.velocity.y==0&&g.cpu_velocity.y==0&&g.blue_score==2);
    assert(!g.jump_held&&!g.cpu_jump_held);
    /* Repeated jumping and airborne challenges remain bounded in a full match. */
    game_init(&g);
    for(i=0;i<120*600;i++){
        Input in={sinf(i*.003f),1,(i%900<50)?1:0,i%500<200,i%200<15,0};game_step(&g,in,STEP);
        assert(isfinite(g.car.y)&&isfinite(g.cpu.y)&&isfinite(g.ball.y));
        assert(isfinite(g.ball.x)&&isfinite(g.ball.z));
        assert(g.car.y>=-.05f&&g.car.y<100&&g.cpu.y>=-.05f&&g.cpu.y<100);
        assert(g.ball.y>=BALL_R&&g.ball.y<100);
    }
    printf("PASS: hop peak %.2f, landing/held button, one double jump/no third hop, airborne shot, car clearance, CPU jump/reset, 10-minute jump match\n",peak);
    return 0;
}
