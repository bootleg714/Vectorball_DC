/* Host texture decode + production UI; target uses bilinear filtering. */
#define main reference_snapshot_main
#include "snapshot.c"
#undef main
#include "title_art.h"
int main(int argc,char **argv){
    int x,y,page=argc>1?atoi(argv[1]):0;memset(depth,0,sizeof(depth));
    for(y=0;y<480;y++)for(x=0;x<640;x++){
        unsigned pixel=title_pixels[(y*512/480)*512+x*512/640];
        rgb[y][x][0]=(unsigned char)(((pixel>>11)&31)*255/31);
        rgb[y][x][1]=(unsigned char)(((pixel>>5)&63)*255/63);
        rgb[y][x][2]=(unsigned char)((pixel&31)*255/31);
    }
    render_frontend(page==2?FRONT_CONTROLS:FRONT_TITLE,page==1,1,1);
    printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);return 0;
}
