/* Optional byte comparison against v0.30; compile new source with -DUPPER_WALL. */
#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>
static int record(const Game *g){
#ifdef UPPER_WALL
    size_t prefix=offsetof(CarSurface,upper_wall_sliding);
#else
    size_t prefix=sizeof(CarSurface);
#endif
    return fwrite(&g->car_surface,1,prefix,stdout)!=prefix||fwrite(&g->cpu_surface,1,prefix,stdout)!=prefix||fwrite(&g->sounds,1,sizeof(Game)-offsetof(Game,sounds),stdout)!=sizeof(Game)-offsetof(Game,sounds);
}
int main(void){
    Game g;int scene,i;
    for(scene=0;scene<13;scene++){
        game_init(&g);g.cpu_enabled=0;g.ball=(Vec3){-35,BALL_R,-35};g.car=(Vec3){0,0,0};
        if(scene){int side=scene&1?1:-1;g.car=(Vec3){side*(ARENA_X-.06f),6+(scene%3)*5.0f,0};
            g.car_surface.dynamic=1;g.car_surface.up=(Vec3){-side,0,0};g.car_surface.forward=(Vec3){0,.70710678f,.70710678f};g.velocity=(Vec3){0,8,8};}
        for(i=0;i<1200;i++){
            Input in={scene?((scene%3)-1)*.3f:sinf(i*.01f),scene?0:1,0,!scene&&i%300<140,!scene&&i%240<10,0};
            game_step(&g,in,STEP);
            if(!scene)g.car.x=g.car.z=0;
            if(g.car.y+g.car_surface.up.y*.85f>=ARENA_HEIGHT*.40f)return 2;
            if(record(&g))return 1;
        }
    }
    return 0;
}
