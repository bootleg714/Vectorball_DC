#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int outside_count;
static float dotv(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
void backend_triangle(Screen a,Screen b,Screen c,uint32_t col){(void)a;(void)b;(void)c;(void)col;}
static void check_car(const Camera *c,const Game *g){
    int i;Vec3 right,up,forward;game_car_basis(g,0,&right,&up,&forward);
    assert(isfinite(c->focal)&&c->focal>0&&c->focal<=390.01f);
    assert(fabsf(dotv(c->forward,c->right))<.001f&&fabsf(dotv(c->forward,c->up))<.001f);
    for(i=0;i<8;i++){
        float x=i&1?CAR_FRAME_WIDTH:-CAR_FRAME_WIDTH,z=i&4?CAR_FRAME_LENGTH:-CAR_FRAME_LENGTH;
        float y=i&2?CAR_FRAME_HEIGHT:0;
        Vec3 d={g->car.x+x*right.x+y*up.x+z*forward.x-c->eye.x,g->car.y+x*right.y+y*up.y+z*forward.y-c->eye.y,g->car.z+x*right.z+y*up.z+z*forward.z-c->eye.z};
        float depth=dotv(d,c->forward),px=320+c->focal*dotv(d,c->right)/depth,py=240-c->focal*dotv(d,c->up)/depth;
        if(depth<=.25f||px<27||px>613||py<81||py>453)outside_count++;
        assert(depth>.25f&&px>=27&&px<=613&&py>=81&&py<=453);
    }
}
int main(void){
    Game g;Camera c={0};int i,k;float previous_yaw,previous_focal;
    game_init(&g);g.car=(Vec3){0,0,0};
    {Vec3 balls[]={{0,BALL_R,25},{0,20,8},{0,35,0},{0,BALL_R,-25},{20,8,0}};
    for(k=0;k<5;k++){g.ball=balls[k];camera_update(&c,&g,1,0,1);check_car(&c,&g);
        if(k==0)assert(fabsf(c.distance-BALL_CAM_DISTANCE)<.001f&&c.distance<13);}}
    for(k=0;k<4;k++){
        g.car=(Vec3){k&1?ARENA_X-1.8f:-ARENA_X+1.8f,0,k&2?ARENA_Z-1.8f:-ARENA_Z+1.8f};g.ball=(Vec3){0,18,0};
        camera_update(&c,&g,1,0,1);check_car(&c,&g);
    }
    /* Fast ball crossings, overhead arcs, jumping and a camera toggle. */
    game_init(&g);g.car=(Vec3){0,0,0};camera_update(&c,&g,1,0,1);
    for(i=0;i<3600;i++){
        float t=i/60.0f;g.car=(Vec3){(ARENA_X-4)*sinf(t*.31f),fmaxf(0,2*sinf(t*4)),(ARENA_Z-4)*sinf(t*.21f)};g.heading=t*.7f;
        g.ball=(Vec3){(ARENA_X-2)*sinf(t*1.8f),BALL_R+28*fmaxf(0,sinf(t*.9f)),(ARENA_Z-2)*cosf(t*1.1f)};
        previous_yaw=c.orbit_yaw;previous_focal=c.focal;camera_update(&c,&g,1,1.0f/60,0);check_car(&c,&g);
        assert(fabsf(c.orbit_yaw-previous_yaw)<=3.5f/60+.001f);
        assert(isfinite(c.focal-previous_focal));
    }
    /* Real physics match including scoring resets and repeated jumping. */
    game_init(&g);camera_update(&c,&g,1,0,1);
    for(i=0;i<120*120;i++){
        Input in={sinf(i*.003f),1,(i%900<50)?1:0,i%500<200,i%200<15,0};
        game_clear_events(&g);game_step(&g,in,STEP);camera_update(&c,&g,1,STEP,0);check_car(&c,&g);
    }
    /* Actual wall climb, tilted bounds and falling back through camera tracking. */
    game_init(&g);g.cpu_enabled=0;g.car=(Vec3){ARENA_X-12,0,0};g.heading=1.57079633f;
    g.velocity=(Vec3){20,0,0};camera_update(&c,&g,1,0,1);
    for(i=0;i<120*8;i++){
        Input in={0,i<180?1:0,0,i<180, i==85,0};
        game_step(&g,in,STEP);camera_update(&c,&g,1,STEP,0);check_car(&c,&g);
    }
    camera_update(&c,&g,0,1.0f/60,0);assert(c.focal==390);
    camera_update(&c,&g,1,1.0f/60,0);check_car(&c,&g);
    puts("PASS: ball-camera car framing, overhead/near/wall cases, bounded orbit, 60s moving targets, 2-minute physics match and toggle");
    return outside_count;
}
