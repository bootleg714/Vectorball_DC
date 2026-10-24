#ifndef VECTORBALL_MATCH_H
#define VECTORBALL_MATCH_H
#include "game.h"
#define MATCH_TICKS (5*60*120)
typedef enum {MATCH_REGULATION,MATCH_OVERTIME,MATCH_FINISHING,MATCH_OVER} MatchPhase;
typedef struct {MatchPhase phase;int ticks_left,overtime_ticks;} Match;
void match_init(Match *m);
/* Called only for a running, connected, unpaused fixed STEP tick. */
void match_step(Match *m,Game *g,Input in);
void match_step_two(Match *m,Game *g,Input blue,Input orange);
#endif
