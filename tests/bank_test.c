#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const Input idle={0};
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car.x=-20;}
int main(void){
    Game g;int i,sx,sz;float raised;
    assert(arena_floor(0,0)==0);
    assert(fabsf(arena_floor(ARENA_X-3,0)-BANK_H*(1-cosf(1.5707963268f/3)))<.001f);
    assert(arena_floor(0,ARENA_Z)==0);
    /* Both side banks turn a low, outward shot upward and back into play. */
    for(sx=-1;sx<=1;sx+=2){
        solo(&g);g.ball=(Vec3){sx*(ARENA_X-7),BALL_R,0};g.ball_velocity.x=sx*20;
        raised=0;
        for(i=0;i<90;i++){game_step(&g,idle,STEP);raised=fmaxf(raised,g.ball.y);}
        assert(raised>BALL_R+1&&g.ball_velocity.x*sx<0);
    }
    /* Resting on a bank must roll inward under gravity instead of sticking. */
    solo(&g);g.ball=(Vec3){ARENA_X-3,4,0};
    for(i=0;i<120;i++)game_step(&g,idle,STEP);
    assert(g.ball.x<ARENA_X-5&&g.ball.y>=BALL_R);
    /* All four corners deflect a diagonal shot and settle inward.
       Larger balls follow a different bank trajectory; allow full settling. */
    for(sx=-1;sx<=1;sx+=2)for(sz=-1;sz<=1;sz+=2){
        solo(&g);g.ball=(Vec3){sx*(ARENA_X-9),BALL_R,sz*(ARENA_Z-9)};
        g.ball_velocity=(Vec3){sx*20,0,sz*20};
        for(i=0;i<2400;i++){game_step(&g,idle,STEP);assert(isfinite(g.ball.y)&&g.ball.y>=BALL_R);}
        assert(g.ball_velocity.x*sx<0&&g.ball_velocity.z*sz<0);
        assert(isfinite(g.ball.y)&&g.ball.y>=BALL_R);
    }
    /* A car aimed diagonally at each corner stays within every wall plane. */
    for(sx=-1;sx<=1;sx+=2)for(sz=-1;sz<=1;sz+=2){
        solo(&g);g.car=(Vec3){sx*(ARENA_X-8),0,sz*(ARENA_Z-8)};
        g.heading=atan2f((float)sx,(float)sz);
        for(i=0;i<120;i++){Input gas={0,1,0,1,0,0};game_step(&g,gas,STEP);}
        for(i=0;i<ARENA_EDGES;i++){
            Vec3 a,b;float length,d;arena_edge(i,&a,&b);length=hypotf(b.x-a.x,b.z-a.z);
            d=((g.car.x-a.x)*(b.z-a.z)+(g.car.z-a.z)*(a.x-b.x))/length;
            assert(d<=.05f);
        }
        assert(g.car.y>=0&&g.car.y<WALL_TOP+15);
    }
    /* Flat goal lanes still admit low shots at both ends. */
    for(sz=-1;sz<=1;sz+=2){
        solo(&g);g.ball=(Vec3){0,BALL_R,sz*(ARENA_Z-10)};g.ball_velocity.z=sz*20;
        for(i=0;i<180&&!g.reset_timer;i++)game_step(&g,idle,STEP);
        assert(g.reset_timer>0&&(g.blue_score+g.orange_score)==1);
    }
    puts("PASS: side-bank lift/roll-back, four rounded rebounds, bank driving/jump, corner containment and open goal lanes");
    return 0;
}
