/* Arena preview renders through the real renderer, with a reference rasteriser
   that honours the depth-ALWAYS floor-marking pass like the PVR does.
   Usage: arena_views <output dir>  -> kickoff/overview/goal/side/center/low/split .ppm
   Host software output only: no anti-aliasing, not Dreamcast colours/filtering or FPS. */
#define main reference_snapshot_main
#define ARENA_VIEWS_DEPTH_ALWAYS
#include "snapshot.c"
#undef main
#include "match.h"
static void save(const char *path){FILE *f=fopen(path,"wb");fprintf(f,"P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),f);fclose(f);}
static void clear(void){int x,y;for(y=0;y<480;y++)for(x=0;x<640;x++){rgb[y][x][0]=6;rgb[y][x][1]=9;rgb[y][x][2]=19;depth[y][x]=0;}count=0;}
static void look(Camera *c,Vec3 eye,Vec3 at){c->eye=eye;c->forward=unit((Vec3){at.x-eye.x,at.y-eye.y,at.z-eye.z});c->right=unit((Vec3){c->forward.z,0,-c->forward.x});
    c->up=(Vec3){c->forward.y*c->right.z,-c->forward.x*c->right.z+c->forward.z*c->right.x,-c->forward.y*c->right.x};c->initialized=1;c->focal=390;}
int main(int argc,char **argv){
    Game g;Camera cam={0};char path[512];const char *dir=argv[1];
    game_init(&g);
    clear();camera_update(&cam,&g,0,0,1);render_frame(&g,&cam,0,0);snprintf(path,512,"%s/kickoff.ppm",dir);save(path);fprintf(stderr,"kickoff %d\n",count);
    clear();look(&cam,(Vec3){-62,34,-100},(Vec3){0,0,0});render_frame(&g,&cam,0,0);snprintf(path,512,"%s/overview.ppm",dir);save(path);fprintf(stderr,"overview %d\n",count);
    clear();look(&cam,(Vec3){0,9,60},(Vec3){0,8,ARENA_Z});render_frame(&g,&cam,0,0);snprintf(path,512,"%s/goal.ppm",dir);save(path);fprintf(stderr,"goal %d\n",count);
    clear();look(&cam,(Vec3){-20,6,-20},(Vec3){ARENA_X,4,30});render_frame(&g,&cam,0,0);snprintf(path,512,"%s/side.ppm",dir);save(path);fprintf(stderr,"side %d\n",count);
    clear();look(&cam,(Vec3){0,10,-40},(Vec3){0,0,0});render_frame(&g,&cam,0,0);snprintf(path,512,"%s/center.ppm",dir);save(path);fprintf(stderr,"center %d\n",count);
    {Game h;game_init(&h);h.car=(Vec3){ARENA_X-14,0,-ARENA_Z*.5f};h.heading=.6f;h.cpu=(Vec3){ARENA_X-24,0,-ARENA_Z*.5f+6};h.ball=(Vec3){ARENA_X-9,BALL_R,-ARENA_Z*.5f+9};
     clear();look(&cam,(Vec3){ARENA_X-30,2.2f,-ARENA_Z*.5f-14},(Vec3){ARENA_X-8,1,-ARENA_Z*.5f+6});render_frame(&h,&cam,0,0);snprintf(path,512,"%s/low.ppm",dir);save(path);fprintf(stderr,"low %d\n",count);}
    {Match m;Camera cams[2]={0};int modes[2]={0,0};match_init(&m);game_init(&g);
     camera_update_player(&cams[0],&g,0,0,STEP,1);camera_update_player(&cams[1],&g,1,0,STEP,1);
     clear();render_split_frame(&g,cams,&m,modes,0,0);snprintf(path,512,"%s/split.ppm",dir);save(path);fprintf(stderr,"split %d\n",count);}
    (void)argc;return 0;
}
