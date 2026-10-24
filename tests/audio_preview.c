/* Optional host PCM preview of the new close-range contact profile. */
#include "game.h"
#include "sound_bank.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static void sample(int16_t v){unsigned char b[2]={(unsigned char)(v&255),(unsigned char)((uint16_t)v>>8)};fwrite(b,1,2,stdout);}
int main(void){
    size_t n=sound_samples(SOUND_HIT),i;int16_t *pcm=malloc(n*sizeof(*pcm));int hit;
    float strengths[3]={5,25,BOOST_SPEED};if(!pcm)return 1;sound_synthesize(SOUND_HIT,pcm,n);
    for(i=0;i<SOUND_RATE/2;i++)sample(0);
    for(hit=0;hit<3;hit++){
        float power=strengths[hit]/BOOST_SPEED;
        int volume=(int)(45+195*power),frequency=(int)(SOUND_RATE*(1.12f-.22f*power));
        float rate=(float)frequency/SOUND_RATE,gain=volume/255.0f;
        for(i=0;i*rate<n-1;i++){
            float index=i*rate;size_t a=(size_t)index;float t=index-a;
            sample((int16_t)((pcm[a]*(1-t)+pcm[a+1]*t)*gain));
        }
        for(i=0;i<SOUND_RATE*.65f;i++)sample(0);
    }
    free(pcm);return ferror(stdout)?1:0;
}
