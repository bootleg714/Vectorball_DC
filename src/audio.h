#ifndef VECTORBALL_AUDIO_H
#define VECTORBALL_AUDIO_H
#include "game.h"
int audio_init(void);
void audio_reset(void);
void audio_update(const Game *g,Input in,int muted,float dt);
#endif
