/* Optional comparison of legacy fields with B disabled, across v0.23/v0.25. */
#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>
int main(void){
    Game g;int i;size_t surface;
#ifdef RECOVERY_CONTROL
    surface=offsetof(CarSurface,recovery_time);
#else
    surface=sizeof(CarSurface);
#endif
    game_init(&g);
    for(i=0;i<72000;i++){
        Input in={sinf(i*.008f),i%1300<100?-1:1,i%900<35?.5f:0,i%500<200,0,0};
        if(i%2==0)game_clear_events(&g);
        if(i%11003==0)game_reset(&g);
        game_step(&g,in,STEP);
        if(fwrite(&g.car_surface,1,surface,stdout)!=surface)return 1;
        if(fwrite(&g.cpu_surface,1,surface,stdout)!=surface)return 1;
        if(fwrite(&g.sounds,1,sizeof(Game)-offsetof(Game,sounds),stdout)!=sizeof(Game)-offsetof(Game,sounds))return 1;
    }
    return 0;
}
