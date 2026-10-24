#include "audio.h"
#include "sound_bank.h"
#include <dc/sound/sfxmgr.h>
#include <dc/sound/aica_comm.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
static int loaded,allocated,plays[SOUND_COUNT],stops,updates,last_freq,last_hit_vol,last_hit_freq;
int snd_init(void){return 0;}
sfxhnd_t snd_sfx_load_raw_buf(char *p,size_t bytes,uint32_t rate,uint16_t bits,uint16_t chans){
    size_t i;int16_t *pcm=(int16_t *)p;double sum=0;int peak=0;
    assert(bytes%32==0&&bytes/2<=65534&&rate==SOUND_RATE&&bits==16&&chans==1);
    for(i=0;i<bytes/2;i++){int a=abs(pcm[i]);if(a>peak)peak=a;sum+=(double)pcm[i]*pcm[i];}
    assert(peak>1000&&peak<30000&&sum>0);
    printf("PCM bank %d: %zu bytes, peak %d, RMS %.0f\n",loaded,bytes,peak,sqrt(sum/(bytes/2)));
    return ++loaded;
}
int snd_sfx_chn_alloc(void){return allocated++;}
void snd_sfx_chn_free(int c){assert(c>=0&&c<SOUND_COUNT);}
void snd_sfx_unload(sfxhnd_t h){assert(h>0&&h<=SOUND_COUNT);}
int snd_sfx_play_ex(sfx_play_data_t *p){
    assert(p->idx>0&&p->idx<=SOUND_COUNT&&p->chn==(int)p->idx-1);
    assert(p->vol>=0&&p->vol<=255&&p->freq>=7000&&p->freq<=32000);
    assert(p->pan==128);assert(p->loop==(p->idx==SOUND_ENGINE+1));
    plays[p->idx-1]++;if(p->idx==SOUND_HIT+1){last_hit_vol=p->vol;last_hit_freq=p->freq;}return p->chn;
}
void snd_sfx_stop(int ch){assert(ch>=0&&ch<SOUND_COUNT);stops++;}
int snd_sh4_to_aica(void *packet,uint32_t size){
    aica_cmd_t *cmd=packet;aica_channel_t *ch=(aica_channel_t *)cmd->cmd_data;
    assert(size==24&&cmd->size==24&&cmd->cmd==AICA_CMD_CHAN&&cmd->cmd_id==0);
    assert(ch->cmd==(AICA_CH_CMD_UPDATE|AICA_CH_UPDATE_SET_FREQ|AICA_CH_UPDATE_SET_VOL));
    assert(ch->freq>=7000&&ch->freq<=32000&&ch->vol<=255);last_freq=ch->freq;updates++;return 0;
}
int main(void){
    Game g;Input idle={0};int i,low,quiet;game_init(&g);
    assert(audio_init()==0&&loaded==SOUND_COUNT&&allocated==SOUND_COUNT);
    for(i=0;i<10;i++)audio_update(&g,idle,0,1.0f/60);
    assert(plays[SOUND_ENGINE]==1&&updates>0);low=last_freq;
    g.velocity.z=25;for(i=0;i<60;i++)audio_update(&g,idle,0,1.0f/60);
    assert(last_freq>low+8000&&plays[SOUND_ENGINE]==1);
    g.sounds.hit=3;audio_update(&g,idle,0,.1f);quiet=last_hit_vol;
    g.sounds.hit=30;audio_update(&g,idle,0,.1f);assert(last_hit_vol>quiet);
    audio_update(&g,idle,0,.001f);assert(plays[SOUND_HIT]==2);
    g.sounds.jump=1;g.sounds.land=9;audio_update(&g,idle,0,.01f);
    assert(plays[SOUND_JUMP]==1&&plays[SOUND_LAND]==1);game_clear_events(&g);
    audio_update(&g,idle,1,.01f);assert(stops==SOUND_COUNT);
    audio_update(&g,idle,1,.01f);assert(stops==SOUND_COUNT);
    audio_update(&g,idle,0,.01f);assert(plays[SOUND_ENGINE]==2);
    g.sounds.goal=1;g.reset_timer=2;audio_update(&g,idle,0,.01f);game_clear_events(&g);
    for(i=0;i<10;i++)audio_update(&g,idle,0,.01f);
    assert(plays[SOUND_GOAL]==1&&plays[SOUND_ENGINE]==2);
    game_reset(&g);audio_update(&g,idle,0,.01f);assert(plays[SOUND_ENGINE]==3);
    game_clear_events(&g);g.feedback.car_hit=5;g.feedback.hit_position=g.car;
    audio_update(&g,idle,0,.1f);
    {int soft_volume=last_hit_vol,soft_pitch=last_hit_freq,hard_volume,count;
        g.feedback.car_hit=BOOST_SPEED;audio_update(&g,idle,0,.1f);hard_volume=last_hit_vol;
        assert(hard_volume>soft_volume+100&&last_hit_freq<soft_pitch);
        count=plays[SOUND_HIT];audio_update(&g,idle,0,.001f);assert(plays[SOUND_HIT]==count);
        g.feedback.hit_position.y+=50;audio_update(&g,idle,0,.1f);assert(last_hit_vol<hard_volume/2);
        game_clear_events(&g);assert(g.feedback.car_hit==0);
    }
    puts("PASS: bank sizes/headroom, channel ownership, continuous engine/pitch, car-hit loudness/depth/3D attenuation, hit strength/cooldown, pause/resume, single goal/reset");return 0;
}
