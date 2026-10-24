/* Versus must stay exactly as accepted (34-39 FPS on hardware). Prints a hash of
   every triangle (screen position, depth, colour) the split renderer submits over
   varied layouts. Compare against the value recorded in HANDOFF.md, or against a
   build of the previous render.c, after any render change. Also fails if any
   versus triangle uses the single-screen depth-ALWAYS marking pass. */
#include "render.h"
#include <stdio.h>
static unsigned long long h=1469598103934665603ULL;static unsigned n,flagged;
static void mixb(const void *p,size_t k){const unsigned char *b=p;while(k--){h^=*b++;h*=1099511628211ULL;}}
void backend_triangle(Screen a,Screen b,Screen c,uint32_t col){if(render_depth_always)flagged++;mixb(&a,sizeof a);mixb(&b,sizeof b);mixb(&c,sizeof c);mixb(&col,sizeof col);n++;}
int main(void){
    Game g;Match m;Camera cams[2]={0};int modes[2]={0},i;
    game_init(&g);match_init(&m);
    for(i=0;i<400;i++){
        int s=i%8;
        g.car=(Vec3){s==1?ARENA_X-10:(s-4)*9.0f,s==2?12:0,s==3?ARENA_Z-14:-SPAWN_Z+s*20};g.heading=s*.8f;
        g.cpu=(Vec3){s==1?-ARENA_X+10:4-s*7.0f,s==5?8:0,SPAWN_Z-s*15};g.cpu_heading=3.14159265f-s*.5f;
        g.ball=(Vec3){s*5.0f-15,s==2?24:BALL_R,(s-3)*12.0f};modes[0]=s%2;modes[1]=(s/2)%2;
        camera_update_player(&cams[0],&g,0,modes[0],STEP,1);camera_update_player(&cams[1],&g,1,modes[1],STEP,1);
        render_split_frame(&g,cams,&m,modes,0,0);render_split_fps(37);
        if(s==6)render_split_frame(&g,cams,&m,modes,1,0);
    }
    printf("versus: %u triangles, hash %016llx, depth-always %u\n",n,h,flagged);
    return flagged?1:0;
}
