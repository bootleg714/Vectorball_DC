#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int triangles;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    (void)color;assert(isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z)&&a.z>0);assert(isfinite(b.x)&&isfinite(b.y)&&isfinite(b.z)&&b.z>0);assert(isfinite(c.x)&&isfinite(c.y)&&isfinite(c.z)&&c.z>0);triangles++;
}
static int active(const Game *g){int i,n=0;for(i=0;i<CONTACT_SPARKS;i++)if(g->feedback.sparks[i].life>0)n++;return n;}
static void solo(Game *g){game_init(g);g->cpu_enabled=0;g->car=(Vec3){0,0,0};g->ball=(Vec3){0,BALL_R,3};}
int main(void){
    Game g;int i,base;Camera c={0};
    solo(&g);g.velocity.z=10;game_step(&g,(Input){0},STEP);
    assert(g.feedback.car_hit>9&&active(&g)==1&&g.feedback.sparks[0].normal.z>.99f);
    assert(fabsf(g.feedback.hit_position.z-(g.ball.z-BALL_R))<.01f);
    {float weak=g.feedback.car_hit;g.car=(Vec3){0,0,0};g.ball=(Vec3){0,BALL_R,3};g.ball_velocity=(Vec3){0};g.velocity.z=30;
        game_step(&g,(Input){0},STEP);assert(g.feedback.car_hit>weak&&active(&g)==2);
    }
    game_clear_events(&g);assert(g.feedback.car_hit==0&&active(&g)==2);
    for(i=0;i<30;i++){game_step(&g,(Input){0},STEP);}
    assert(active(&g)==0);
    solo(&g);g.velocity.z=3;game_step(&g,(Input){0},STEP);assert(g.feedback.car_hit>2&&active(&g)==0);
    solo(&g);g.ball_velocity.z=30;game_step(&g,(Input){0},STEP);assert(g.feedback.car_hit==0&&active(&g)==0);
    solo(&g);g.car=(Vec3){0,2,0};g.velocity=(Vec3){0,3,16};g.ball=(Vec3){0,BALL_R+1.85f,3};game_step(&g,(Input){0},STEP);
    assert(g.feedback.car_hit>5&&active(&g)==1&&g.feedback.sparks[0].normal.y>0);
    game_init(&g);g.car.x=50;g.cpu=(Vec3){0,0,3};g.cpu_velocity.z=-20;game_step(&g,(Input){0},STEP);
    assert(g.feedback.car_hit>10&&active(&g)>0);
    solo(&g);g.ball=(Vec3){ARENA_X-BALL_R-.1f,10,0};g.ball_velocity=(Vec3){30,0,0};game_step(&g,(Input){0},STEP);
    assert(g.sounds.hit>2&&g.feedback.car_hit==0&&active(&g)==0);
    /* Fixed pool remains bounded under repeated impacts and retains strongest event. */
    solo(&g);
    for(i=0;i<20;i++){
        g.car=(Vec3){0,0,0};g.ball=(Vec3){0,BALL_R,3};g.ball_velocity=(Vec3){0};g.velocity=(Vec3){0,0,20};
        game_step(&g,(Input){0},STEP);assert(active(&g)<=CONTACT_SPARKS);
    }
    assert(active(&g)==CONTACT_SPARKS);game_reset(&g);assert(active(&g)==0&&g.feedback.car_hit==0);
    solo(&g);g.ball=(Vec3){0,BALL_R,10};camera_update(&c,&g,0,0,1);triangles=0;render_frame(&g,&c,0,0);base=triangles;
    for(i=0;i<CONTACT_SPARKS;i++)g.feedback.sparks[i]=(ContactSpark){{0,0,1},CONTACT_SPARK_TIME*.5f,1};
    {Game before=g;triangles=0;render_frame(&g,&c,0,0);assert(triangles==base+CONTACT_SPARKS*8&&memcmp(&before,&g,sizeof(g))==0);}
    g.ball=(Vec3){0,BALL_R,ARENA_Z+BALL_R+.1f};game_step(&g,(Input){0},STEP);assert(g.sounds.goal&&active(&g)==0);
    puts("PASS: real ground/aerial/CPU contact, separating/soft touches, no wall spark, strongest substep event, lifetime/frame-clear/reset/goal handling, fixed four-spark pool and <=32 finite read-only VFX triangles");return 0;
}
