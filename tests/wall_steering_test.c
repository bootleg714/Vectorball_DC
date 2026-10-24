#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 cross(Vec3 a,Vec3 b){return (Vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static Game approach(int wall,int direction,int phase){
    Game g;int i,released=0,found=0;
    game_init(&g);g.cpu_enabled=0;g.ball=(Vec3){-wall*25,BALL_R,0};
    g.car=(Vec3){wall*(ARENA_X-15),0,-direction*20};
    g.heading=atan2f(wall*cosf(.55f),direction*sinf(.55f));
    g.velocity=(Vec3){sinf(g.heading)*20,0,cosf(g.heading)*20};
    for(i=0;i<1200;i++){
        Input in={0};if(g.car.y>3)released=1;
        if(!released){in.throttle=1;in.boost=1;}game_step(&g,in,STEP);
        if(g.car_surface.grounded_wheels>=2&&g.car.y>3.5f&&
           ((phase==0&&g.velocity.y>3)||(phase==1&&fabsf(g.velocity.y)<.3f)||(phase==2&&g.velocity.y< -2))){found=1;break;}
    }
    assert(found);return g;
}
static float advance(Game *g,float steer,int frames,float *max_turn){
    float yaw=0;int i;
    for(i=0;i<frames;i++){
        Vec3 before=g->car,forward=g->car_surface.forward,up=g->car_surface.up;
        Input in={0};float turn,step;in.steer=steer;game_step(g,in,STEP);
        turn=acosf(fmaxf(-1,fminf(1,dot(forward,g->car_surface.forward))));
        *max_turn=fmaxf(*max_turn,turn);assert(isfinite(turn)&&turn<.10f);
        yaw+=atan2f(dot(up,cross(forward,g->car_surface.forward)),dot(forward,g->car_surface.forward));
        step=hypotf(hypotf(g->car.x-before.x,g->car.z-before.z),g->car.y-before.y);
        assert(isfinite(step)&&step<1);
        assert(fabsf(g->car.x)<=ARENA_X+.05f&&fabsf(g->car.z)<=ARENA_Z+.05f);
    }
    return yaw;
}
int main(void){
    int wall,direction,phase,sign,level,cases=0;float max_turn=0,minimum=10;
    for(wall=-1;wall<=1;wall+=2)for(direction=-1;direction<=1;direction+=2)for(phase=0;phase<3;phase++){
        Game seed=approach(wall,direction,phase);
        for(sign=-1;sign<=1;sign+=2)for(level=0;level<2;level++){
            Game g=seed;float amount=level?.35f:1,first,second;
            first=sign*advance(&g,sign*amount,48,&max_turn);
            assert(first>(level?.04f:.25f));minimum=fminf(minimum,first/amount);
            assert(g.car_surface.grounded_wheels>=2);
            second=-sign*advance(&g,-sign*amount,48,&max_turn);
            assert(second>(level?.04f:.15f));
            /* Let the arc resume without a pose/position correction or spin spike. */
            advance(&g,0,360,&max_turn);cases++;
        }
    }
    printf("PASS: %d mirrored ascent/crest/descent wall turns, full/35%% analog, countersteering and neutral release; minimum signed turn/input %.3f rad per .4s, max orientation step %.2f degrees\n",cases,minimum,max_turn*57.2957795f);
    return 0;
}
