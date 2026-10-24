#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static Vec3 cross(Vec3 a,Vec3 b){return (Vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0,0,0};g->ball=(Vec3){30,BALL_R,30};}
int main(void){
    Game g;int yaw,pose,dir,pitch_index,i,cases=0;float max_time=0;
    for(yaw=0;yaw<8;yaw++)for(pose=0;pose<5;pose++)for(dir=-1;dir<=1;dir++)for(pitch_index=-1;pitch_index<=1;pitch_index++){
        float heading=yaw*.7853981634f,angle=(.55f+pose*.225f)*3.14159265f,pitch=pitch_index*.6f;
        solo(&g);g.heading=heading;g.car_surface.dynamic=1;g.car_surface.forward=(Vec3){sinf(heading)*cosf(pitch),sinf(pitch),cosf(heading)*cosf(pitch)};
        g.car_surface.up=(Vec3){-sinf(heading)*sinf(pitch)*cosf(angle)-cosf(heading)*sinf(angle),cosf(pitch)*cosf(angle),-cosf(heading)*sinf(pitch)*cosf(angle)+sinf(heading)*sinf(angle)};
        g.car.y=1.5f;
        for(i=0;i<120;i++)game_step(&g,(Input){0},STEP);
        if(g.car_surface.up.y>.9f)continue;
        game_step(&g,(Input){(float)dir,0,0,0,1,0},STEP);
        assert(g.car_surface.recovery_time>0&&g.velocity.y>5&&g.jump_held);
        for(i=0;i<360;i++){
            game_step(&g,(Input){0},STEP);
            assert(isfinite(g.car.y)&&g.car.y<6&&g.car.y>-.1f);
            if(g.car_surface.up.y>.98f&&g.car_surface.grounded_wheels>=3)break;
        }
        if(i>=360){fprintf(stderr,"FAIL yaw%d pose%d dir%d up%.3f y%.3f wheels%d recovery%.3f\n",yaw,pose,dir,g.car_surface.up.y,g.car.y,g.car_surface.grounded_wheels,g.car_surface.recovery_time);return 1;}
        max_time=fmaxf(max_time,i*STEP);cases++;
    }
    /* Flat-driver air-roll activation, both directions, release braking. */
    for(dir=-1;dir<=1;dir+=2){
        solo(&g);game_step(&g,(Input){0,0,0,0,1,0},STEP);
        for(i=0;i<30;i++)game_step(&g,(Input){(float)dir,0,0,0,1,0},STEP);
        assert(g.car_surface.dynamic&&g.car_surface.air_roll_active>0);
        assert(g.car_surface.up.x*dir>0.1f&&dot(g.car_surface.angular_velocity,g.car_surface.forward)*dir< -2);
        for(i=0;i<24;i++)game_step(&g,(Input){0},STEP);
        assert(fabsf(dot(g.car_surface.angular_velocity,g.car_surface.forward))<.3f);
    }
    /* Tap cannot become repeated launch while B stays held. */
    solo(&g);g.car.y=1.5f;g.car_surface.dynamic=1;g.car_surface.up=(Vec3){0,-1,0};g.car_surface.forward=(Vec3){0,0,1};
    for(i=0;i<90;i++)game_step(&g,(Input){0},STEP);
    game_step(&g,(Input){0,0,0,0,1,0},STEP);
    for(i=0;i<480;i++)game_step(&g,(Input){0,0,0,0,1,0},STEP);
    assert(g.car_surface.up.y>.98f&&g.car.y<.1f&&fabsf(g.velocity.y)<.2f);
    game_step(&g,(Input){0},STEP);assert(g.car_surface.air_roll_active==0);
    for(i=0;i<8;i++)game_step(&g,(Input){1,0,0,0,1,0},STEP);
    assert(g.car_surface.air_roll_active>0); /* New jump after recovery can roll normally. */
    /* Inverted at height cannot use recovery as a second jump. */
    solo(&g);g.car.y=12;g.car_surface.dynamic=1;g.car_surface.up=(Vec3){0,-1,0};g.car_surface.forward=(Vec3){0,0,1};
    game_step(&g,(Input){0,0,0,0,1,0},STEP);assert(g.car_surface.recovery_time==0&&g.velocity.y<0);
    game_reset(&g);assert(g.car_surface.recovery_time==0&&g.car_surface.air_roll_active==0);
    assert(dot(cross(g.car_surface.up,g.car_surface.forward),cross(g.car_surface.up,g.car_surface.forward))>.99f);
    printf("PASS: %d resting side/roof recovery fixtures across headings/directions, worst %.2fs; both airborne roll directions, release braking, held-B no repeat, no aerial recovery jump and reset state\n",cases,max_time);return 0;
}
