#include "match.h"
void match_init(Match *m){m->phase=MATCH_REGULATION;m->ticks_left=MATCH_TICKS;m->overtime_ticks=0;}
static void advance_game(Game *g,Input in,const Input *second){
    if(second)game_step_two(g,in,*second,STEP);else game_step(g,in,STEP);
}
static void match_step_internal(Match *m,Game *g,Input in,const Input *second){
    int blue=g->blue_score,orange=g->orange_score;
    if(m->phase==MATCH_OVER)return;
    if(m->phase==MATCH_FINISHING){
        if(g->reset_timer>0)advance_game(g,in,second);
        if(g->reset_timer<=0)m->phase=MATCH_OVER;
        return;
    }
    if(g->reset_timer>0){advance_game(g,in,second);return;}
    advance_game(g,in,second);
    if(m->phase==MATCH_OVERTIME){
        m->overtime_ticks++;
        if(g->blue_score!=blue||g->orange_score!=orange)m->phase=MATCH_FINISHING;
        return;
    }
    if(m->ticks_left>0)m->ticks_left--;
    if(m->ticks_left==0){
        if(g->blue_score==g->orange_score){
            m->phase=MATCH_OVERTIME;
            /* Final-tick equalizer keeps its celebration; otherwise kick off now. */
            if(g->reset_timer<=0)game_reset(g);
        }else m->phase=g->reset_timer>0?MATCH_FINISHING:MATCH_OVER;
    }
}

void match_step(Match *m,Game *g,Input in){match_step_internal(m,g,in,0);}
void match_step_two(Match *m,Game *g,Input blue,Input orange){match_step_internal(m,g,blue,&orange);}
