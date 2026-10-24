#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static Game solo(float speed){Game g;game_init(&g);g.cpu_enabled=0;g.car=(Vec3){0,0,0};g.ball=(Vec3){70,BALL_R,0};g.velocity.z=speed;return g;}
static float stop(float amount,float *distance){
    Game g=solo(BOOST_SPEED);Input in={0,1,amount,1,0,0};int i;
    for(i=0;i<120*4&&g.velocity.z>.01f;i++){
        game_step(&g,in,STEP);assert(g.velocity.z>=-.001f);assert(g.boost==1);
    }
    assert(g.velocity.z<.01f);*distance=g.car.z;return i*STEP;
}
static float turn(float initial,int sign,int boost,float *span){
    Game g=solo(initial);Input in={(float)sign,boost?1:0,0,boost,0,0};int i;
    for(i=0;i<120*6&&sign*g.heading<1.5707963268f;i++)game_step(&g,in,STEP);
    assert(sign*g.heading>=1.5707963268f);*span=hypotf(g.car.x,g.car.z);return i*STEP;
}
int main(void){
    Game g;Input in={0};int i,sign;float full_distance,partial_distance,full,partial,min_fast=100,max_slow=0;
    full=stop(1,&full_distance);partial=stop(.5f,&partial_distance);
    assert(fabsf(full-logf(1+.65f*BOOST_SPEED/BRAKE_DECEL)/.65f)<STEP*2);
    assert(fabsf(full_distance-(BOOST_SPEED/.65f-BRAKE_DECEL/(.65f*.65f)*logf(1+.65f*BOOST_SPEED/BRAKE_DECEL)))<.5f);
    assert(partial>full*1.4f&&partial_distance>full_distance*1.4f);
    for(sign=-1;sign<=1;sign+=2){float slow,fast,slow_time,fast_time;
        slow_time=turn(16,sign,0,&slow);fast_time=turn(BOOST_SPEED,sign,1,&fast);
        assert(fast_time>slow_time*1.8f&&fast_time<slow_time*2.1f&&fast>slow*3);min_fast=fminf(min_fast,fast);max_slow=fmaxf(max_slow,slow);
    }
    /* Lifting boost must not erase existing speed at the normal drive cap. */
    g=solo(BOOST_SPEED);in.throttle=1;game_step(&g,in,STEP);
    assert(g.velocity.z>BOOST_SPEED-1&&g.velocity.z<BOOST_SPEED);
    for(i=0;i<120;i++){game_step(&g,in,STEP);g.car.z=0;}
    assert(g.velocity.z<=CAR_SPEED+.01f&&g.velocity.z>CAR_SPEED-1);
    /* Opposite throttle brakes first, then reverses; brake alone cannot reverse. */
    g=solo(20);in=(Input){0,-1,0,0,0,0};
    for(i=0;i<180;i++)game_step(&g,in,STEP);
    assert(g.velocity.z< -9&&g.velocity.z>=-10);
    /* Accelerating from speed remains positive but adds less speed near the cap. */
    {Game low=solo(5),high=solo(CAR_SPEED*.9f);float lo=low.velocity.z,hi=high.velocity.z;
        in=(Input){0,1,0,0,0,0};game_step(&low,in,STEP);game_step(&high,in,STEP);
        assert(low.velocity.z-lo>high.velocity.z-hi&&high.velocity.z>hi);
    }
    /* Light trigger input must always slow more than simply lifting off. */
    {Game coast=solo(BOOST_SPEED),brake=coast;
        game_step(&coast,(Input){0},STEP);game_step(&brake,(Input){0,0,.1f,0,0,0},STEP);
        assert(brake.velocity.z<coast.velocity.z);
    }
    /* Rest and analog steering: no pivot at rest; partial stick gives a smaller turn. */
    g=solo(0);in=(Input){1,0,0,0,0,0};game_step(&g,in,STEP);assert(g.heading==0);
    {Game a=solo(BOOST_SPEED),b=a;in.steer=.35f;game_step(&a,in,STEP);in.steer=1;game_step(&b,in,STEP);
        assert(a.heading>0&&a.heading<b.heading*.4f);
    }
    printf("PASS: full/half braking %.2f/%.2fs, stopping distance %.2f/%.2f; 90-degree turn span slow %.2f vs boosted %.2f; boost-release carry, brake priority, reverse, tapered pull and analog steering\n",full,partial,full_distance,partial_distance,max_slow,min_fast);
    return 0;
}
