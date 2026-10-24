#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
/* Recenter the isolated flat fixture to measure yaw without hitting a wall. */
static float yaw_rate(float speed,int dir){
    Game g;int i;game_init(&g);g.cpu_enabled=0;g.car=(Vec3){0,0,0};g.ball=(Vec3){70,BALL_R,0};
    for(i=0;i<60;i++){
        g.car=(Vec3){0,0,0};g.velocity=(Vec3){sinf(g.heading)*speed,0,cosf(g.heading)*speed};
        game_step(&g,(Input){(float)dir,1,0,speed>CAR_SPEED,0,0},STEP);
    }
    return dir*g.heading/(60*STEP);
}
int main(void){
    int player,dir,i;float normal=0,boosted=0,peak=0;
    for(dir=-1;dir<=1;dir+=2){
        float slow=yaw_rate(16,dir),normal_rate=yaw_rate(CAR_SPEED,dir),boost_rate=yaw_rate(BOOST_SPEED,dir);
        normal=CAR_SPEED/normal_rate;boosted=BOOST_SPEED/boost_rate;
        assert(fabsf(slow-2.15f)<.002f);
        /* Preserve accepted v0.27 radii while raising both speed caps 10 percent. */
        assert(fabsf(normal-20.6362f)<.03f);
        assert(fabsf(boosted-54.908f)<.03f);
    }
    for(player=0;player<2;player++)for(dir=-1;dir<=1;dir+=2){
        Game g;CarSurface *s;float last=0,angle=0;
        game_init(&g);g.car=(Vec3){-20,20,0};g.cpu=(Vec3){20,20,0};g.ball=(Vec3){70,BALL_R,0};
        /* High-altitude fixture isolates the manual motor from wheel contact. */
        g.cpu_heading=0;g.cpu_surface.forward=(Vec3){0,0,1};
        s=player?&g.cpu_surface:&g.car_surface;
        for(i=0;i<120;i++){
            float a,delta;Input in={(float)dir,0,0,0,1,0},idle={0};
            game_step_two(&g,player?idle:in,player?in:idle,STEP);g.car.y=g.cpu.y=20;
            a=atan2f(s->up.x,s->up.y);delta=a-last;
            if(delta>3.14159265f)delta-=6.2831853f;
            if(delta< -3.14159265f)delta+=6.2831853f;
            angle+=delta;last=a;
            assert(isfinite(s->up.y)&&fabsf(dot(s->angular_velocity,s->forward))<8.001f);
        }
        peak=fabsf(dot(s->angular_velocity,s->forward));
        assert(peak>7.4f&&peak<7.6f&&dir*angle>6.9f&&dir*angle<7.1f);
        for(i=0;i<24;i++){game_step_two(&g,(Input){0},(Input){0},STEP);g.car.y=g.cpu.y=20;}
        assert(fabsf(dot(s->angular_velocity,s->forward))<.15f);
        /* Opposite input must reverse the roll without releasing the motor latch. */
        for(i=0;i<45;i++){
            Input in={-(float)dir,0,0,0,1,0},idle={0};
            game_step_two(&g,player?idle:in,player?in:idle,STEP);g.car.y=g.cpu.y=20;
        }
        assert(dot(s->angular_velocity,s->forward)*dir>7.3f);
    }
    printf("PASS: both players/directions roll %.2f rad/s, >one rotation in 1s, release below .15 rad/s in .2s and counter-roll; slow yaw unchanged, normal/boost radii %.2f/%.2f (v0.27 radii retained); speed caps +10%% %.2f/%.2f\n",peak,normal,boosted,CAR_SPEED,BOOST_SPEED);
    return 0;
}
