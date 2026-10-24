#ifndef VECTORBALL_SOUND_BANK_H
#define VECTORBALL_SOUND_BANK_H
#include <stdint.h>
#include <stddef.h>
#define SOUND_RATE 11025
/* Original, procedurally synthesized mono PCM16; no external assets. */
enum { SOUND_ENGINE,SOUND_HIT,SOUND_JUMP,SOUND_LAND,SOUND_GOAL,SOUND_COUNT };
size_t sound_samples(int kind);
void sound_synthesize(int kind,int16_t *data,size_t count);
#endif
