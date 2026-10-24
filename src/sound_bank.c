#include "sound_bank.h"
#include <math.h>
static const size_t lengths[SOUND_COUNT]={2048,2208,1760,1984,21168};
size_t sound_samples(int kind){return kind>=0&&kind<SOUND_COUNT?lengths[kind]:0;}
void sound_synthesize(int kind,int16_t *data,size_t count){
    size_t i;uint32_t seed=0x62949a71u;float filtered=0;
    const float tau=6.283185307f;
    for(i=0;i<count;i++){
        float t=(float)i/SOUND_RATE,u=(float)i/count,s=0,env;
        seed=seed*1664525u+1013904223u;
        filtered=.7f*filtered+.3f*((float)(seed>>8)/8388607.5f-1);
        if(kind==SOUND_ENGINE){
            float p=tau*8*i/count;
            s=.42f*sinf(p)+.22f*sinf(p*2)+.10f*sinf(p*3)+.05f*sinf(p*7);
        }else if(kind==SOUND_HIT){
            env=fminf(1,t/.002f)*expf(-t*25)*(1-u);
            s=(.75f*sinf(tau*(105*t+50*(1-expf(-t*25))/25))+.45f*filtered)*env;
        }else if(kind==SOUND_JUMP){
            env=sinf(3.14159265f*u)*expf(-u*1.2f);
            s=(.56f*sinf(tau*(170*t+950*t*t))+.25f*filtered)*env;
        }else if(kind==SOUND_LAND){
            env=fminf(1,t/.003f)*expf(-t*24)*(1-u);
            s=(.65f*sinf(tau*67*t)+.50f*filtered)*env;
        }else if(kind==SOUND_GOAL){
            /* Filtered noise with many wavering voices: a brief synthetic cheer. */
            float voices=.11f*sinf(tau*(271*t+.7f*sinf(9*t)))+.10f*sinf(tau*(329*t+.9f*sinf(7*t)))+.10f*sinf(tau*(393*t+.5f*sinf(11*t)));
            float swell=(1-expf(-t*20))*powf(1-u,.8f);
            float chord=(sinf(tau*523.25f*t)+sinf(tau*659.25f*t)+sinf(tau*783.99f*t))*.08f;
            s=(filtered*(.70f+.25f*sinf(tau*3*t))+voices+chord*expf(-t*2))*swell;
        }
        /* Soft headroom; the endpoint fade removes one-shot start/end clicks. */
        if(kind!=SOUND_ENGINE)s*=fminf(1,(float)(count-1-i)/64);
        s=fmaxf(-.92f,fminf(.92f,s));data[i]=(int16_t)(s*26000);
    }
}
