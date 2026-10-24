/* Native-host CPU-work comparison only: never a Dreamcast FPS prediction. */
#include "render.h"
#include <stdio.h>
#include <time.h>
static unsigned count;static volatile float sink;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){count++;sink+=a.x*.000001f+b.y*.000001f+c.z*.000001f+(color&255)*.00000001f;}
int main(void){
    Game g;Match m;Camera cameras[2]={0};int modes[2]={0},scene,i;clock_t start;unsigned kickoff=0,total;
    game_init(&g);match_init(&m);start=clock();
    for(i=0;i<240;i++){
        scene=i%4;g.car=(Vec3){scene==1?ARENA_X-10:0,scene==2?12:0,scene==3?ARENA_Z-14:-SPAWN_Z};g.heading=scene*.8f;
        g.cpu=(Vec3){scene==1?-ARENA_X+10:4,scene==2?8:0,SPAWN_Z};g.cpu_heading=3.14159265f-scene*.5f;
        g.ball=(Vec3){0,scene==2?24:BALL_R,0};modes[0]=scene%2;modes[1]=(scene/2)%2;
        camera_update_player(&cameras[0],&g,0,modes[0],STEP,1);camera_update_player(&cameras[1],&g,1,modes[1],STEP,1);
        count=0;render_split_frame(&g,cameras,&m,modes,0,0);if(i==0)kickoff=count;
    }
    total=count;
    printf("240 split frames (4 fixtures), native CPU %.2f ms, kickoff %u triangles, last frame %u triangles, sink %.3f; NOT target FPS\n",1000.0*(clock()-start)/CLOCKS_PER_SEC,kickoff,total,(double)sink);return 0;
}
