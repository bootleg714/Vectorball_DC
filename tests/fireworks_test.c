#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int effect,blue,orange;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    assert(isfinite(a.x)&&isfinite(a.y)&&isfinite(a.z)&&a.z>0);assert(isfinite(b.x)&&isfinite(b.y)&&isfinite(b.z)&&b.z>0);assert(isfinite(c.x)&&isfinite(c.y)&&isfinite(c.z)&&c.z>0);
    if(color==0xff61cfff||color==0xffff944d||color==0xffffd166||color==0xfffff3cf)effect++;
    if(color==0xff61cfff)blue++;
    if(color==0xffff944d)orange++;
}
static void draw(const Game *g,const Camera *c,int paused){
    Game before=*g;effect=blue=orange=0;render_frame(g,c,paused,0);assert(memcmp(&before,g,sizeof(*g))==0);
}
int main(void){
    Game g;Camera c={0};int sign,i,peak;
    for(sign=-1;sign<=1;sign+=2){
        game_init(&g);g.cpu_enabled=0;g.car=(Vec3){0,0,sign*(ARENA_Z-32)};g.heading=sign>0?0:3.14159265f;
        camera_update(&c,&g,0,0,1);draw(&g,&c,0);assert(effect==0);
        g.ball=(Vec3){0,BALL_R,sign*(ARENA_Z+BALL_R+.1f)};
        game_step(&g,(Input){0},STEP);assert(g.sounds.goal&&g.reset_timer==2&&g.last_goal==sign);
        assert((sign>0?g.blue_score:g.orange_score)==1);
        draw(&g,&c,0);assert(effect==36);assert(sign>0?blue>0&&orange==0:orange>0&&blue==0);
        game_clear_events(&g);assert(!g.sounds.goal);draw(&g,&c,0);assert(effect==36);
        g.reset_timer=1.3f;draw(&g,&c,0);assert(effect==108);peak=effect;
        draw(&g,&c,1);assert(effect==peak);draw(&g,&c,1);assert(effect==peak);
        /* Sweep entire timer; all geometry finite, fixed bound, no re-trigger. */
        for(i=0;i<=200;i++){g.reset_timer=2-i*.01f;draw(&g,&c,0);assert(effect<=108);if(i>=173)assert(effect==0);}
        g.reset_timer=.001f;game_step(&g,(Input){0},STEP);assert(g.reset_timer==0);draw(&g,&c,0);assert(effect==0);
        assert((sign>0?g.blue_score:g.orange_score)==1);
        game_reset(&g);draw(&g,&c,0);assert(effect==0);
        /* Cross-near-plane/overhead views remain finite and read-only. */
        g.reset_timer=1.3f;g.last_goal=sign;c.eye=(Vec3){0,GOAL_H+6,sign*(ARENA_Z-3)};
        draw(&g,&c,0);
    }
    puts("PASS: both real scoring paths, scorer-colored 3-burst fireworks, 108-triangle bound in unclipped view, full lifetime, frame-clear/pause/read-only rendering, near-plane clipping and unchanged kickoff/reset score");return 0;
}
