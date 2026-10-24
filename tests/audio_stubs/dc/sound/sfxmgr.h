#include <stddef.h>
#include <stdint.h>
typedef uint32_t sfxhnd_t;
#define SFXHND_INVALID 0
typedef struct {int chn;sfxhnd_t idx;int vol,pan,loop,freq;unsigned int loopstart,loopend;} sfx_play_data_t;
sfxhnd_t snd_sfx_load_raw_buf(char *,size_t,uint32_t,uint16_t,uint16_t);
int snd_sfx_play_ex(sfx_play_data_t *);
int snd_sfx_chn_alloc(void);
void snd_sfx_chn_free(int);
void snd_sfx_unload(sfxhnd_t);
void snd_sfx_stop(int);
