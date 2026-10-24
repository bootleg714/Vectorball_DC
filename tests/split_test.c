#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int triangles,split_check,top,bottom;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    Screen p[3]={a,b,c};int i;
    for(i=0;i<3;i++)assert(isfinite(p[i].x)&&isfinite(p[i].y)&&isfinite(p[i].z)&&p[i].z>0);
    if(split_check){
        float lo=fminf(a.y,fminf(b.y,c.y)),hi=fmaxf(a.y,fmaxf(b.y,c.y));
        for(i=0;i<3;i++)assert(p[i].x>=-.001f&&p[i].x<=640.001f&&p[i].y>=-.001f&&p[i].y<=480.001f);
        if(color!=0xff6c8793)assert(hi<=240.001f||lo>=239.999f);
        if(hi<=240.001f)top++;
        if(lo>=239.999f)bottom++;
    }
    triangles++;
}
static void finite_game(const Game *g){assert(isfinite(g->car.x)&&isfinite(g->car.y)&&isfinite(g->car.z)&&isfinite(g->cpu.x)&&isfinite(g->cpu.y)&&isfinite(g->cpu.z));assert(g->boost>=0&&g->boost<=1&&g->cpu_boost>=0&&g->cpu_boost<=1);}
int main(void){
    Game g,before;Camera cameras[2]={0};Match m;Input coast={0},gas={0,1,0,0,0,0};int modes[2]={0,0},i,single,split;
    game_init(&g);match_init(&m);
    for(i=0;i<120;i++)game_step_two(&g,coast,coast,STEP);
    assert(fabsf(g.car.z+SPAWN_Z)<.001f&&fabsf(g.cpu.z-SPAWN_Z)<.001f&&g.cpu_attack==0&&g.cpu_stuck==0);
    game_init(&g);for(i=0;i<120;i++)game_step_two(&g,gas,gas,STEP);
    assert(g.velocity.z>15&&g.cpu_velocity.z< -15&&fabsf(g.velocity.z+g.cpu_velocity.z)<.001f);
    game_init(&g);game_step_two(&g,coast,(Input){0,1,0,1,1,0},STEP);assert(g.cpu.y>0&&g.car.y==0&&g.cpu_boost<1&&g.boost==1&&g.cpu_jump_held&&!g.jump_held);
    game_init(&g);g.cpu=(Vec3){0,0,3};g.ball=(Vec3){0,BALL_R,0};g.car.x=50;g.cpu_velocity.z=-20;
    game_step_two(&g,coast,coast,STEP);assert(g.feedback.car_hit>10&&g.ball_velocity.z<0);
    game_init(&g);g.car=(Vec3){-1,0,0};g.cpu=(Vec3){1,0,0};game_step_two(&g,coast,coast,STEP);assert(g.car.x< -1&&g.cpu.x>1);
    game_init(&g);match_init(&m);m.ticks_left=1;match_step_two(&m,&g,coast,coast);assert(m.phase==MATCH_OVERTIME);
    g.ball=(Vec3){0,BALL_R,-ARENA_Z-BALL_R-.1f};match_step_two(&m,&g,coast,coast);assert(m.phase==MATCH_FINISHING&&g.orange_score==1);
    for(i=0;i<242&&m.phase!=MATCH_OVER;i++)match_step_two(&m,&g,coast,coast);
    assert(m.phase==MATCH_OVER);
    game_init(&g);camera_update_player(&cameras[0],&g,0,0,0,1);camera_update_player(&cameras[1],&g,1,0,0,1);
    assert(cameras[0].eye.z<g.car.z&&cameras[1].eye.z>g.cpu.z&&cameras[0].forward.z>0&&cameras[1].forward.z<0);
    before=g;triangles=0;render_frame(&g,&cameras[0],0,0);single=triangles;
    split_check=1;triangles=top=bottom=0;render_split_frame(&g,cameras,&m,modes,0,0);split=triangles;assert(split*3*32+1024<2*1024*1024);
    assert(top>0&&bottom>0&&memcmp(&g,&before,sizeof(g))==0);
    split_check=0;triangles=0;render_frame(&g,&cameras[0],0,0);assert(triangles==single);
    printf("RENDER COUNTS kickoff: accepted-style single %d, two views total %d (top %d bottom %d); not FPS\n",single,split,top,bottom);
    /* Bank/near-plane/high-ball stress in both camera modes and missing-pad UI. */
    for(i=0;i<24;i++){
        g.car=(Vec3){ARENA_X-2,i%3?5:30,-25};g.cpu=(Vec3){-ARENA_X+2,8,25};g.ball=(Vec3){0,12+(i%4)*8,0};g.heading=i*.7f;g.cpu_heading=-i*.5f;
        modes[0]=i%2;modes[1]=(i/2)%2;camera_update_player(&cameras[0],&g,0,modes[0],STEP,1);camera_update_player(&cameras[1],&g,1,modes[1],STEP,1);
        before=g;split_check=1;triangles=0;render_split_frame(&g,cameras,&m,modes,i%2,i%4);assert(triangles*3*32+1024<2*1024*1024);assert(memcmp(&g,&before,sizeof(g))==0);
    }
    game_init(&g);
    for(i=0;i<72000;i++){
        Input blue={sinf(i*.01f),i%900<80?-1:1,i%700<30?.5f:0,i%400<160,i%300<5,0};
        Input orange={cosf(i*.007f),i%1000<60?-1:1,i%600<20?.5f:0,i%500<180,i%280<4,0};
        if(i%2==0)game_clear_events(&g);
        game_step_two(&g,blue,orange,STEP);finite_game(&g);
        assert(g.cpu_attack==0&&g.cpu_stuck==0&&g.cpu_reverse==0);
    }
    puts("PASS: two neutral/human motors, mirrored acceleration, independent boost/jump, orange ball contact, shared car collisions, versus overtime/results, independent camera frames, clipped read-only viewports/no state leakage and 10-minute two-input stress without AI");return 0;
}
