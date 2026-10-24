#include "render.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint64_t hash;static int triangles;
static void number(float f){union {float f;uint32_t bits;} x;x.f=f;hash^=x.bits;hash*=UINT64_C(1099511628211);}
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    if(triangles++>=1000)return;
    number(a.x);number(a.y);number(a.z);number(b.x);number(b.y);number(b.z);number(c.x);number(c.y);number(c.z);
    hash^=color;hash*=UINT64_C(1099511628211);
}
static uint64_t frame(const Game *g,const Camera *c){hash=UINT64_C(1469598103934665603);triangles=0;render_frame(g,c,0,1);assert(triangles>1000);return hash;}
int main(void){
    Game g;Camera c={0};uint64_t baseline;
    game_init(&g);g.cpu_enabled=0;camera_update(&c,&g,1,0,1);frame(&g,&c);baseline=frame(&g,&c);
    g.car_surface.dynamic=1;g.car_surface.up=(Vec3){-1,0,0};g.car_surface.forward=(Vec3){0,1,0};frame(&g,&c);
    g.car_surface.dynamic=0;g.car_surface.up=(Vec3){0,1,0};g.car_surface.forward=(Vec3){0,0,1};
    assert(frame(&g,&c)==baseline);
    puts("PASS: fixed world geometry stays identical after rendering a tilted car");return 0;
}
