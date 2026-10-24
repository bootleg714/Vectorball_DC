/* Host previews of production match HUD and results over approved packed art. */
#define main reference_snapshot_main
#include "snapshot.c"
#undef main
#include "title_art.h"
int main(int argc,char **argv){
    Game g;Match m;Camera c={0};int mode=argc>1?atoi(argv[1]):0,x,y;
    game_init(&g);match_init(&m);g.blue_score=3;g.orange_score=2;
    memset(rgb,0,sizeof(rgb));memset(depth,0,sizeof(depth));
    if(mode<2){
        if(mode==1)g.orange_score=3;
        g.car=(Vec3){0,0,-30};camera_update(&c,&g,0,0,1);render_frame(&g,&c,0,0);
        m.ticks_left=65*120;if(mode==1){m.phase=MATCH_OVERTIME;m.overtime_ticks=12*120;}
        render_match_clock(&m);
    }else{
        for(y=0;y<480;y++)for(x=0;x<640;x++){
            unsigned pixel=title_pixels[(y*512/480)*512+x*512/640];
            rgb[y][x][0]=(unsigned char)(((pixel>>11)&31)*255/31);rgb[y][x][1]=(unsigned char)(((pixel>>5)&63)*255/63);rgb[y][x][2]=(unsigned char)((pixel&31)*255/31);
        }
        if(mode==3)g.orange_score=4;
        render_results(&g,mode==3,1,1);
    }
    printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);return 0;
}
