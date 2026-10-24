/* Host-only reference rasterizer: checks the real game's submitted geometry. */
#include "render.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned char rgb[480][640][3];
static float depth[480][640];
static int count;
static Vec3 unit(Vec3 a){float n=sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);return (Vec3){a.x/n,a.y/n,a.z/n};}
static void close_camera(Camera *c,const Game *g,const char *name){
    float x=4,z=5.5f;
    if(strcmp(name,"rear")==0){x=-4;z=-5.5f;}
    if(strcmp(name,"side")==0){x=6.5f;z=.1f;}
    c->eye=(Vec3){g->car.x+x,3.4f,g->car.z+z};
    c->forward=unit((Vec3){-x,.9f-c->eye.y,-z});
    c->right=unit((Vec3){c->forward.z,0,-c->forward.x});
    c->up=(Vec3){c->forward.y*c->right.z,-c->forward.x*c->right.z+c->forward.z*c->right.x,-c->forward.y*c->right.x};
}
static float edge(Screen a,Screen b,float x,float y){return (x-a.x)*(b.y-a.y)-(y-a.y)*(b.x-a.x);}
void backend_triangle(Screen a,Screen b,Screen c,uint32_t col){
    float area;int x,y,x0,x1,y0,y1;
    assert(isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z)&&a.z>0);
    assert(isfinite(b.x)&&isfinite(b.y)&&isfinite(b.z)&&b.z>0);
    assert(isfinite(c.x)&&isfinite(c.y)&&isfinite(c.z)&&c.z>0);
    area=edge(a,b,c.x,c.y);if(fabsf(area)<.0001f)return;
    x0=(int)fmaxf(0,floorf(fminf(a.x,fminf(b.x,c.x))));x1=(int)fminf(639,ceilf(fmaxf(a.x,fmaxf(b.x,c.x))));
    y0=(int)fmaxf(0,floorf(fminf(a.y,fminf(b.y,c.y))));y1=(int)fminf(479,ceilf(fmaxf(a.y,fmaxf(b.y,c.y))));
    for(y=y0;y<=y1;y++)for(x=x0;x<=x1;x++){
        float u=edge(b,c,x+.5f,y+.5f)/area,w=edge(a,b,x+.5f,y+.5f)/area,t=1-u-w;
        float z=u*a.z+t*b.z+w*c.z;
        if(u>=0&&t>=0&&w>=0&&z>=depth[y][x]){
            depth[y][x]=z;rgb[y][x][0]=(col>>16)&255;rgb[y][x][1]=(col>>8)&255;rgb[y][x][2]=col&255;
        }
    }
    count++;
}
int main(int argc,char **argv){
    Game g;Camera cam={0};int x,y;int ball_mode=argc>1&&strncmp(argv[1],"ballcam",7)==0;game_init(&g);
    if(argc>1&&strcmp(argv[1],"corner")==0){g.car.x=ARENA_X-8;g.car.z=ARENA_Z-11;g.car.y=arena_floor(g.car.x,g.car.z);g.heading=.8f;g.ball.x=ARENA_X-5;g.ball.z=ARENA_Z-9;g.ball.y=BALL_R+arena_floor(g.ball.x,g.ball.z)+.5f;}
    if(argc>1&&strcmp(argv[1],"match")==0){int i;Input gas={0,1,0,0,0,0};g.car.x=5;for(i=0;i<120;i++)game_step(&g,gas,STEP);}
    if(argc>1&&strcmp(argv[1],"jump")==0){int i;Input in={0,1,0,0,1,0};g.car.x=4;for(i=0;i<45;i++)game_step(&g,in,STEP);}
    if(argc>1&&strcmp(argv[1],"banks")==0){g.car=(Vec3){ARENA_X-11,0,0};g.heading=1.5707963f;g.ball=(Vec3){ARENA_X-4,4,3};}
    if(ball_mode){
        g.car=(Vec3){0,0,0};g.ball=(Vec3){0,BALL_R,25};g.cpu.x=10;
        if(strcmp(argv[1],"ballcam-high")==0)g.ball=(Vec3){0,20,8};
        if(strcmp(argv[1],"ballcam-overhead")==0)g.ball=(Vec3){0,35,0};
        if(strcmp(argv[1],"ballcam-wall")==0){g.car.z=-ARENA_Z+2;g.ball=(Vec3){0,BALL_R,-ARENA_Z+30};}
    }
    if(argc>1&&(strcmp(argv[1],"wallride")==0||strcmp(argv[1],"bank-tilt")==0)){
        int i;int wall=strcmp(argv[1],"wallride")==0;Input gas={0,1,0,1,0,0};
        game_init(&g);g.cpu_enabled=0;g.car=(Vec3){ARENA_X-12,0,0};g.heading=1.57079633f;g.velocity=(Vec3){20,0,0};
        for(i=0;i<240;i++){
            game_step(&g,gas,STEP);
            if(g.car_surface.attached&&(wall?g.car.y>5:g.car.y>.8f))break;
        }
        g.ball=(Vec3){ARENA_X-15,BALL_R,12};
        cam.eye=(Vec3){g.car.x-12,g.car.y+5,g.car.z-9};
        cam.forward=unit((Vec3){g.car.x-cam.eye.x,g.car.y+.6f-cam.eye.y,g.car.z-cam.eye.z});
        cam.right=unit((Vec3){cam.forward.z,0,-cam.forward.x});
        cam.up=(Vec3){cam.forward.y*cam.right.z,-cam.forward.x*cam.right.z+cam.forward.z*cam.right.x,-cam.forward.y*cam.right.x};
        cam.initialized=1;cam.focal=390;
    }
    for(y=0;y<480;y++)for(x=0;x<640;x++){rgb[y][x][0]=6;rgb[y][x][1]=9;rgb[y][x][2]=19;}
    if(!(argc>1&&(strcmp(argv[1],"wallride")==0||strcmp(argv[1],"bank-tilt")==0)))camera_update(&cam,&g,ball_mode,0,1);
    if(argc>1&&(strcmp(argv[1],"front")==0||strcmp(argv[1],"rear")==0||strcmp(argv[1],"side")==0))close_camera(&cam,&g,argv[1]);
    render_frame(&g,&cam,0,ball_mode);
    printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);
    fprintf(stderr,"snapshot: %d triangles with finite coordinates\n",count);return 0;
}
