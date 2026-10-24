#include "game.h"
#include <math.h>
#include <assert.h>
/* Isolated infinite-flat fixture: recenter horizontal position after each tick.
   Mode2 starts with a small physical lean to retain the contact solver.
   These cases do not estimate rollover probability during real matches. */
#include <stdio.h>
int main(void){int mode,level,sign,i;float speeds[3]={16,CAR_SPEED,BOOST_SPEED};
for(mode=0;mode<3;mode++)for(level=0;level<3;level++)for(sign=-1;sign<=1;sign+=2){Game g;float minimum=1,peak=0;int flips=0;game_init(&g);g.cpu_enabled=0;g.car=(Vec3){0,0,0};g.ball=(Vec3){70,BALL_R,0};g.velocity.z=speeds[level];g.car_surface.dynamic=mode!=0;if(mode==2){g.car_surface.up=(Vec3){.1f,.9949874f,0};g.car.y=.12f;}
for(i=0;i<720;i++){game_step(&g,(Input){(float)sign,1,0,1,0,0},STEP);g.car.x=g.car.z=0;g.boost=1;assert(isfinite(g.car_surface.up.y)&&isfinite(g.car.y));minimum=fminf(minimum,g.car_surface.up.y);peak=fmaxf(peak,g.car.y);if(g.car_surface.up.y<0)flips++;}
assert(flips==0);printf("mode%d speed%.2f steer%d: tilt %.2fdeg inverted_steps%d peakY%.3f\n",mode,speeds[level],sign,acosf(fmaxf(-1,minimum))*57.2957795f,flips,peak);}
return 0;}
