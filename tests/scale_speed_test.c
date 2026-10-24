#include "game.h"
#include "vehicle.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0,0,0};g->ball=(Vec3){20,BALL_R,0};}
static float speed(Vec3 v){return sqrtf(v.x*v.x+v.y*v.y+v.z*v.z);}
int main(void){
    Game g;Input in={0,1,0,0,0,0};int i;float ordinary,boosted,jump_peak=0;
    assert(fabsf(2*ARENA_X-197.50575f)<.001f&&fabsf(2*ARENA_Z-246.8822f)<.001f);
    assert(fabsf(BALL_R-3.4375f)<.00001f);
    /* A contact beyond the v0.29 4.40-unit reach now meets the enlarged ball. */
    solo(&g);g.ball=(Vec3){0,BALL_R,4.75f};g.velocity.z=10;
    game_step(&g,(Input){0},STEP);assert(g.ball_velocity.z>10);
    /* A 3.0-unit post clearance fitted the v0.29 radius, but not the new one. */
    solo(&g);g.ball=(Vec3){GOAL_W-3.0f,BALL_R,ARENA_Z-BALL_R+.1f};g.ball_velocity.z=20;
    game_step(&g,(Input){0},STEP);assert(g.ball_velocity.z<0&&!g.blue_score);
    solo(&g);
    /* Translate back to center every step so a long acceleration run cannot hit a wall. */
    for(i=0;i<120*10;i++){game_step(&g,in,STEP);g.car.z=0;assert(g.velocity.z<=CAR_SPEED+.001f);}
    ordinary=g.velocity.z;assert(fabsf(ordinary-CAR_SPEED)<.01f);
    in.boost=1;
    for(i=0;i<120*3;i++){game_step(&g,in,STEP);g.car.z=0;assert(g.velocity.z<=BOOST_SPEED+.001f);}
    boosted=g.velocity.z;assert(fabsf(boosted-BOOST_SPEED)<.01f);
    solo(&g);in=(Input){0};g.car.y=10;g.ball.y=10;g.ball_velocity=(Vec3){0,0,0};game_step(&g,in,STEP);
    assert(fabsf(g.velocity.y+CAR_GRAVITY*STEP)<.001f);
    assert(fabsf(g.ball_velocity.y+BALL_GRAVITY*STEP)<.001f);
    /* Enforce the new total speed cap, including vertical and diagonal motion. */
    for(i=0;i<3;i++){
        solo(&g);g.ball=(Vec3){20,20,0};
        g.ball_velocity=i==0?(Vec3){0,500,0}:i==1?(Vec3){300,300,300}:(Vec3){500,0,0};
        game_step(&g,in,STEP);assert(speed(g.ball_velocity)<=BALL_SPEED+.002f&&speed(g.ball_velocity)>BALL_SPEED-1);
    }
    solo(&g);g.ball=(Vec3){20,ARENA_HEIGHT-BALL_R-.1f,0};g.ball_velocity.y=100;
    game_step(&g,in,STEP);assert(g.ball.y<=ARENA_HEIGHT-BALL_R&&g.ball_velocity.y<0);
    solo(&g);g.car.y=ARENA_HEIGHT-1.9f;g.velocity.y=50;
    game_step(&g,in,STEP);assert(g.car.y<=ARENA_HEIGHT-1.8f+.001f&&g.velocity.y<=0);
    solo(&g);g.car=(Vec3){ARENA_X-2,ARENA_HEIGHT-1,0};g.car_surface.dynamic=1;g.velocity.y=30;
    game_step(&g,in,STEP);assert(g.car.y<ARENA_HEIGHT-1);
    {WheelContact hit;assert(vehicle_probe((Vec3){ARENA_X-1,ARENA_HEIGHT-3,0},(Vec3){1,0,0},2,&hit));}
    /* Fast shots meet side/corner walls without tunneling or escaping the roof. */
    {int angle,height,step;
        for(angle=0;angle<8;angle++)for(height=0;height<3;height++){
            float a=angle*.7853981634f;
            solo(&g);g.car=(Vec3){-40,0,-40};g.ball=(Vec3){0,height==0?BALL_R:height==1?20:ARENA_HEIGHT-4,0};
            g.ball_velocity=(Vec3){sinf(a)*BALL_SPEED,0,cosf(a)*BALL_SPEED};
            for(step=0;step<120*8;step++){
                game_step(&g,in,STEP);
                assert(isfinite(g.ball.x)&&isfinite(g.ball.y)&&isfinite(g.ball.z));
                assert(g.ball.y>=BALL_R&&g.ball.y<=ARENA_HEIGHT-BALL_R+.001f);
                assert(fabsf(g.ball.x)<=ARENA_X+.001f&&fabsf(g.ball.z)<=ARENA_Z+BALL_R+1.3f);
                assert(speed(g.ball_velocity)<=BALL_SPEED+.002f);
            }
        }
    }
    solo(&g);in.jump=1;for(i=0;i<240;i++){game_step(&g,in,STEP);jump_peak=fmaxf(jump_peak,g.car.y);}assert(g.car.y==0);
    assert(jump_peak>2.7f&&jump_peak<2.95f);
    printf("PASS: ball radius3.4375 (another +25%%), enlarged contact/post clearance; field %.2fx%.2f, height %.2f, goals %.2fx%.2f, throttle %.2f, boost %.2f, total ball cap %.2f, gravity %.2f, ceiling ball/car contacts, high wall probe, 24 max-speed shots, jump peak %.2f\n",2*ARENA_X,2*ARENA_Z,ARENA_HEIGHT,2*GOAL_W,GOAL_H,ordinary,boosted,BALL_SPEED,CAR_GRAVITY,jump_peak);
    return 0;
}
