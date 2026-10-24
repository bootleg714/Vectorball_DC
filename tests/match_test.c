#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int triangles;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    (void)color;assert(isfinite(a.x)&&isfinite(b.x)&&isfinite(c.x));assert(a.x>=0&&a.x<=640&&b.x>=0&&b.x<=640&&c.x>=0&&c.x<=640);assert(a.y>=0&&a.y<=480&&b.y>=0&&b.y<=480&&c.y>=0&&c.y<=480);triangles++;
}
static void setup(Match *m,Game *g){match_init(m);game_init(g);g->cpu_enabled=0;}
static void goal(Game *g,int sign){g->ball=(Vec3){0,BALL_R,sign*(ARENA_Z+BALL_R+.1f)};g->ball_velocity=(Vec3){0};}
static void finish(Match *m,Game *g){int ticks=0;while(m->phase!=MATCH_OVER){match_step(m,g,(Input){0});assert(++ticks<=242);}assert(g->reset_timer==0);}
int main(void){
    Match m;Game g,reference;int i;Frontend f;
    setup(&m,&g);assert(m.ticks_left==36000&&m.phase==MATCH_REGULATION);
    reference=g;
    for(i=0;i<1200;i++){
        Input in={sinf(i*.015f),1,0,i%120<45,i%240<5,0};
        game_clear_events(&g);game_clear_events(&reference);match_step(&m,&g,in);game_step(&reference,in,STEP);assert(memcmp(&g,&reference,sizeof(g))==0);
    }
    assert(m.ticks_left==34800);
    setup(&m,&g);goal(&g,1);match_step(&m,&g,(Input){0});assert(g.sounds.goal&&m.ticks_left==35999);
    i=m.ticks_left;while(g.reset_timer>0)match_step(&m,&g,(Input){0});assert(m.ticks_left==i&&g.blue_score==1);
    game_reset(&g);assert(m.ticks_left==i); /* Live position reset preserves clock. */
    setup(&m,&g);for(i=0;i<35999;i++)match_step(&m,&g,(Input){0});assert(m.phase==MATCH_REGULATION&&m.ticks_left==1);
    match_step(&m,&g,(Input){0});assert(m.phase==MATCH_OVERTIME&&m.ticks_left==0&&g.blue_score==g.orange_score&&g.car.z==-SPAWN_Z);
    match_step(&m,&g,(Input){0});assert(m.overtime_ticks==1);
    goal(&g,-1);match_step(&m,&g,(Input){0});assert(m.phase==MATCH_FINISHING&&g.orange_score==1&&g.reset_timer==2);
    i=m.overtime_ticks;finish(&m,&g);assert(m.overtime_ticks==i&&g.orange_score==1);
    reference=g;match_step(&m,&g,(Input){0,1,0,1,1,0});assert(memcmp(&g,&reference,sizeof(g))==0);
    setup(&m,&g);g.blue_score=2;g.orange_score=1;m.ticks_left=1;match_step(&m,&g,(Input){0});assert(m.phase==MATCH_OVER&&m.ticks_left==0);
    setup(&m,&g);m.ticks_left=1;goal(&g,1);match_step(&m,&g,(Input){0});assert(m.phase==MATCH_FINISHING&&g.blue_score==1);finish(&m,&g);
    setup(&m,&g);g.orange_score=1;m.ticks_left=1;goal(&g,1);match_step(&m,&g,(Input){0});assert(m.phase==MATCH_OVERTIME&&g.reset_timer==2);
    i=m.overtime_ticks;while(g.reset_timer>0)match_step(&m,&g,(Input){0});assert(m.overtime_ticks==i&&g.blue_score==g.orange_score);
    /* Results navigation, fresh rematch selection and title escape. */
    frontend_init(&f);f.page=FRONT_RESULTS;assert(frontend_step(&f,FRONT_ACCEPT,0,1)&&f.page==FRONT_MATCH);
    f.page=FRONT_RESULTS;f.selection=0;frontend_step(&f,FRONT_NEXT,0,1);frontend_step(&f,FRONT_ACCEPT,0,1);assert(f.page==FRONT_TITLE&&f.selection==0);
    f.page=FRONT_RESULTS;frontend_step(&f,FRONT_BACK,0,1);assert(f.page==FRONT_TITLE);
    setup(&m,&g);render_match_clock(&m);m.ticks_left=1;render_match_clock(&m);m.phase=MATCH_OVERTIME;render_match_clock(&m);m.overtime_ticks=7320;render_match_clock(&m);
    g.blue_score=3;g.orange_score=2;render_results(&g,0,1,1);g.orange_score=4;render_results(&g,1,0,0);assert(triangles>0);
    puts("PASS: exact 36,000-tick regulation, identical active physics, goal-clock freeze, tied overtime kickoff, elapsed OT, sudden-death celebration/results, regulation/final-tick winner/equalizer, reset behavior, frozen finish, result actions and finite clock/result UI");return 0;
}
