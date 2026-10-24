#include <kos.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>
#include <dc/sound/aica_comm.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "audio.h"
#include "sound_bank.h"
static sfxhnd_t samples[SOUND_COUNT];
static int channels[SOUND_COUNT];
static int ready,engine_on,was_muted=1;
static float update_timer,hit_cooldown,rpm;
static float clamp(float v,float a,float b){return v<a?a:(v>b?b:v);}
static void play(int kind,int volume,int frequency,int loop){
    sfx_play_data_t p;
    memset(&p,0,sizeof(p));p.chn=channels[kind];p.idx=samples[kind];
    p.vol=volume;p.pan=128;p.freq=frequency;p.loop=loop;
    snd_sfx_play_ex(&p);
}
static void engine_update(int frequency,int volume){
    AICA_CMDSTR_CHANNEL(packet,cmd,chn);
    memset(packet,0,sizeof(packet));
    cmd->size=AICA_CMDSTR_CHANNEL_SIZE;cmd->cmd=AICA_CMD_CHAN;cmd->cmd_id=channels[SOUND_ENGINE];
    chn->cmd=AICA_CH_CMD_UPDATE|AICA_CH_UPDATE_SET_FREQ|AICA_CH_UPDATE_SET_VOL;
    chn->freq=frequency;chn->vol=volume;
    snd_sh4_to_aica(packet,AICA_CMDSTR_CHANNEL_SIZE);
}
int audio_init(void){
    int k;
    for(k=0;k<SOUND_COUNT;k++)channels[k]=-1;
    if(snd_init()<0){printf("Vectorball: sound init failed; continuing silently\n");return -1;}
    for(k=0;k<SOUND_COUNT;k++){
        size_t n=sound_samples(k);int16_t *pcm=malloc(n*sizeof(*pcm));
        if(!pcm)break;
        sound_synthesize(k,pcm,n);
        samples[k]=snd_sfx_load_raw_buf((char *)pcm,n*sizeof(*pcm),SOUND_RATE,16,1);
        free(pcm);
        if(samples[k]==SFXHND_INVALID)break;
        channels[k]=snd_sfx_chn_alloc();if(channels[k]<0)break;
    }
    if(k!=SOUND_COUNT){
        for(k=0;k<SOUND_COUNT;k++){
            if(channels[k]>=0)snd_sfx_chn_free(channels[k]);
            if(samples[k]!=SFXHND_INVALID){snd_sfx_unload(samples[k]);samples[k]=SFXHND_INVALID;}
        }
        printf("Vectorball: sound bank unavailable; continuing silently\n");return -1;
    }
    ready=1;rpm=8000;return 0;
}
void audio_reset(void){
    int k;if(!ready)return;
    for(k=0;k<SOUND_COUNT;k++)snd_sfx_stop(channels[k]);
    engine_on=0;hit_cooldown=update_timer=0;rpm=8000;
}
void audio_update(const Game *g,Input in,int muted,float dt){
    float speed,target;int frequency,volume;
    if(!ready)return;
    if(muted){if(!was_muted)audio_reset();was_muted=1;return;}
    was_muted=0;hit_cooldown=fmaxf(0,hit_cooldown-dt);
    if(g->sounds.goal){
        audio_reset();play(SOUND_GOAL,220,SOUND_RATE,0);
    }
    if(g->reset_timer>0){if(engine_on){snd_sfx_stop(channels[SOUND_ENGINE]);engine_on=0;}return;}
    speed=sqrtf(g->velocity.x*g->velocity.x+g->velocity.z*g->velocity.z+(g->car_surface.attached?g->velocity.y*g->velocity.y:0));
    target=8000+speed*(530*37/BOOST_SPEED)+fabsf(in.throttle)*1500+(in.boost&&g->boost>0?1800:0);
    rpm+=(target-rpm)*fminf(1,dt*7);frequency=(int)clamp(rpm,7000,32000);
    volume=(int)(36+clamp(speed/BOOST_SPEED,0,1)*42+fabsf(in.throttle)*10);
    if(!engine_on){play(SOUND_ENGINE,volume,frequency,1);engine_on=1;update_timer=0;}
    update_timer+=dt;
    if(update_timer>=1.0f/30){engine_update(frequency,volume);update_timer=0;}
    if((g->sounds.hit>2||g->feedback.car_hit>2)&&hit_cooldown<=0){
        if(g->feedback.car_hit>2){
            Vec3 point=g->feedback.hit_position;
            float distance=sqrtf((point.x-g->car.x)*(point.x-g->car.x)+(point.y-g->car.y)*(point.y-g->car.y)+(point.z-g->car.z)*(point.z-g->car.z));
            float power=clamp(g->feedback.car_hit/BOOST_SPEED,0,1);
            volume=(int)((45+195*power)*clamp(22/(distance+12),.20f,1));
            frequency=(int)(SOUND_RATE*(1.12f-.22f*power));
        }else{
            float distance=hypotf(g->ball.x-g->car.x,g->ball.z-g->car.z);
            volume=(int)((60+clamp(g->sounds.hit/35,0,1)*170)*clamp(22/(distance+12),.35f,1));
            frequency=(int)(SOUND_RATE*(.85f+clamp(g->sounds.hit/50,0,.35f)));
        }
        play(SOUND_HIT,volume,frequency,0);hit_cooldown=.07f;
    }
    if(g->sounds.jump>0)play(SOUND_JUMP,(int)(155*g->sounds.jump),SOUND_RATE,0);
    if(g->sounds.land>0)play(SOUND_LAND,(int)clamp(g->sounds.land*16,35,180),SOUND_RATE,0);
}
