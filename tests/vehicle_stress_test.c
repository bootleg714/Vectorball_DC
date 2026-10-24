#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float length(Vec3 a){return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);}
int main(void){
    Game g;int axis,side,speed_index,angle_index,i,cases=0,turned=0;
    float speeds[3]={6,CAR_SPEED,BOOST_SPEED},maximum_speed=0,maximum_step=0,maximum_spin=0;
    for(axis=0;axis<2;axis++)for(side=-1;side<=1;side+=2)for(speed_index=0;speed_index<3;speed_index++)for(angle_index=0;angle_index<2;angle_index++){
        game_init(&g);g.cpu_enabled=0;g.ball=(Vec3){-25,BALL_R,-25};
        g.car=axis?(Vec3){30,0,side*(ARENA_Z-15)}:(Vec3){side*(ARENA_X-15),0,0};
        g.heading=(axis?(side>0?0:3.14159265f):side*1.57079633f)+(angle_index?.55f:0);
        g.velocity=(Vec3){sinf(g.heading)*speeds[speed_index],0,cosf(g.heading)*speeds[speed_index]};
        for(i=0;i<120*12;i++){
            Input input={0,i<180?1:0,0,i<180,0,0};Vec3 before=g.car;float step,speed,spin;
            if(angle_index&&g.car.y>4&&g.car_surface.grounded_wheels>=2&&i<180)input.steer=.25f;
            if(i==180&&speed_index==2)input.jump=1;
            game_step(&g,input,STEP);
            step=length((Vec3){g.car.x-before.x,g.car.y-before.y,g.car.z-before.z});speed=length(g.velocity);spin=length(g.car_surface.angular_velocity);
            maximum_step=fmaxf(maximum_step,step);maximum_speed=fmaxf(maximum_speed,speed);maximum_spin=fmaxf(maximum_spin,spin);
            assert(isfinite(step)&&isfinite(speed)&&isfinite(spin));
            assert(fabsf(g.car.x)<ARENA_X+.1f&&fabsf(g.car.z)<ARENA_Z+.1f&&g.car.y>-.1f&&g.car.y<60);
            assert(step<1&&speed<=BOOST_SPEED+.01f&&spin<20);
            if(g.car_surface.up.y<.5f)turned++;
        }
        assert(g.car.y<2.5f);cases++;
    }
    /* Drive into each rounded corner, then let contact and gravity settle the body. */
    for(axis=-1;axis<=1;axis+=2)for(side=-1;side<=1;side+=2){
        game_init(&g);g.cpu_enabled=0;g.ball=(Vec3){0,BALL_R,-25};
        g.car=(Vec3){axis*(ARENA_X-17),0,side*(ARENA_Z-17)};g.heading=atan2f((float)axis,(float)side);
        g.velocity=(Vec3){sinf(g.heading)*20,0,cosf(g.heading)*20};
        for(i=0;i<120*12;i++){
            Input in={0,i<180?1:0,0,i<180,0,0};Vec3 before=g.car;float step;
            game_step(&g,in,STEP);step=length((Vec3){g.car.x-before.x,g.car.y-before.y,g.car.z-before.z});
            assert(isfinite(step)&&step<1&&length(g.velocity)<=BOOST_SPEED+.01f);
            assert(fabsf(g.car.x)<ARENA_X+.1f&&fabsf(g.car.z)<ARENA_Z+.1f&&g.car.y>-.1f&&g.car.y<60);
        }
        assert(g.car.y<2.5f);cases++;
    }
    /* A low-speed overturned car can recover under throttle, without a reset. */
    game_init(&g);g.cpu_enabled=0;g.ball.x=25;g.car=(Vec3){0,1.5f,0};g.car_surface.dynamic=1;
    g.car_surface.up=(Vec3){0,-1,0};g.car_surface.forward=(Vec3){0,0,1};
    for(i=0;i<120*3;i++)game_step(&g,(Input){0,1,0,0,0,0},STEP);
    assert(g.car_surface.up.y>.95f&&g.car_surface.grounded_wheels>=3&&g.car.y<.15f);
    assert(turned>0);
    printf("PASS: %d low/medium/high-speed straight/angled approaches, wall steering, rounded corners, release/jump, 12s settling and overturned recovery; max speed %.2f, displacement %.3f/step, spin %.2f rad/s\n",cases,maximum_speed,maximum_step,maximum_spin);
    return 0;
}
