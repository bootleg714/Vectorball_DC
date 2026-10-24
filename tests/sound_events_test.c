#include "game.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    Game g;Input jump={0,0,0,0,1,0},idle={0};int i;
    game_init(&g);g.cpu_enabled=0;g.car.x=15;game_step(&g,jump,STEP);assert(g.sounds.jump==1);
    game_clear_events(&g);game_step(&g,jump,STEP);assert(g.sounds.jump==0);
    /* Allow the complete longer flight under the converted gravity. */
    for(i=0;i<240;i++){game_step(&g,idle,STEP);}
    assert(g.sounds.land>8);
    game_clear_events(&g);assert(g.sounds.land==0&&g.sounds.hit==0);
    game_reset(&g);g.ball.z=g.car.z+3;g.velocity.z=20;game_step(&g,idle,STEP);assert(g.sounds.hit>15);
    game_reset(&g);g.ball.z=ARENA_Z+BALL_R+.1f;game_step(&g,idle,STEP);assert(g.sounds.goal==1);
    game_clear_events(&g);game_step(&g,idle,STEP);assert(g.sounds.goal==0&&g.orange_score==0&&g.blue_score==1);
    puts("PASS: actual jump/landing/hit/goal events and frame clearing");return 0;
}
