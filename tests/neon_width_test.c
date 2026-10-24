/* Neon arena: thin lines must stay visible at any distance. The PVR has no
   anti-aliasing, so a strip thinner than a pixel blinks in and out. Over many
   cameras, every distance-widened strip piece must be >=1.2 px thick at its
   middle (target 1.6 px at its ends). Build with -DNEON_WIDTH_CHECK. */
#include "render.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
static int checked,thin;static float thinnest=1e9f;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t col){(void)a;(void)b;(void)c;(void)col;}
void neon_width_check(float depth,float needed,float drawn,float base){
    float px;
    if(depth<=1||needed<=base*1.01f)return;   /* near plane, or design width already enough */
    px=1.6f*drawn/needed;checked++;if(px<thinnest)thinnest=px;if(px<1.2f)thin++;
}
static Vec3 unit(Vec3 a){float l=sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);return (Vec3){a.x/l,a.y/l,a.z/l};}
int main(void){
    Game g;Camera c={0};int i;game_init(&g);
    for(i=0;i<2000;i++){
        float ang=i*.37f,rad=5+(i%11)*9.0f;
        c.eye=(Vec3){sinf(ang)*rad*.85f,1.2f+(i%7)*3.5f,cosf(ang)*rad*1.1f};
        c.forward=unit((Vec3){-sinf(ang*1.7f)*40-c.eye.x*.2f,-c.eye.y*(.2f+(i%3)*.3f),-cosf(ang*1.7f)*40-c.eye.z*.2f});
        c.right=unit((Vec3){c.forward.z,0,-c.forward.x});
        c.up=(Vec3){c.forward.y*c.right.z,-c.forward.x*c.right.z+c.forward.z*c.right.x,-c.forward.y*c.right.x};
        c.initialized=1;c.focal=390;
        render_frame(&g,&c,0,0);
    }
    assert(checked>100000&&thin==0&&thinnest>=1.2f);
    printf("PASS: neon line width, %d widened pieces over 2000 cameras, thinnest %.2f px at mid-piece\n",checked,thinnest);
    return 0;
}
