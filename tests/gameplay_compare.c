#include "game.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>
int main(void){
    Game g;int i;
#if defined(BOOST_EXHAUST)
    size_t bytes=offsetof(Game,exhaust);
#elif defined(IMPACT_FEEDBACK)
    size_t bytes=offsetof(Game,feedback);
#elif defined(SUSPENSION_VISUAL)
    size_t bytes=offsetof(Game,car_suspension);
#elif defined(COSMETIC_WHEELS)
    size_t bytes=offsetof(Game,car_visual);
#else
    size_t bytes=sizeof(Game);
#endif
    game_init(&g);fprintf(stderr,"gameplay bytes per step: %lu\n",(unsigned long)bytes);
    for(i=0;i<72000;i++){
        Input in={sinf(i*.008f),i%1300<100?-1:1,i%900<35?.5f:0,i%500<200,i%240<8,0};
        if(i%2==0)game_clear_events(&g);
        if(i%11003==0)game_reset(&g);
        game_step(&g,in,STEP);
        if(fwrite(&g,1,bytes,stdout)!=bytes)return 1;
    }
    return 0;
}
