#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static int triangles;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t color){
    (void)color;assert(isfinite(a.x)&&isfinite(b.x)&&isfinite(c.x));
    assert(a.x>=0&&a.x<=640&&b.x>=0&&b.x<=640&&c.x>=0&&c.x<=640);
    assert(a.y>=0&&a.y<=480&&b.y>=0&&b.y<=480&&c.y>=0&&c.y<=480);triangles++;
}
int main(void){
    Frontend f;int i;frontend_init(&f);assert(f.page==FRONT_TITLE&&f.selection==0);
    assert(!frontend_step(&f,FRONT_ACCEPT,0,0)&&f.page==FRONT_TITLE);
    frontend_step(&f,0,0,1);frontend_step(&f,0,1,1);assert(f.selection==1);
    for(i=0;i<120;i++)frontend_step(&f,0,1,1);
    assert(f.selection==1);frontend_step(&f,0,0,1);frontend_step(&f,0,-1,1);assert(f.selection==0);
    frontend_step(&f,FRONT_NEXT,0,1);assert(f.selection==1);
    assert(!frontend_step(&f,FRONT_ACCEPT,0,1)&&f.page==FRONT_CONTROLS);
    frontend_step(&f,FRONT_BACK,0,1);assert(f.page==FRONT_TITLE&&f.selection==1);
    frontend_step(&f,FRONT_ACCEPT,0,1);frontend_step(&f,FRONT_ACCEPT,0,1);assert(f.page==FRONT_TITLE);
    frontend_step(&f,FRONT_PREV,0,1);assert(frontend_step(&f,FRONT_ACCEPT,0,1)&&f.page==FRONT_MATCH);
    assert(!frontend_step(&f,FRONT_ACCEPT,0,1));frontend_return(&f);assert(f.page==FRONT_TITLE&&f.selection==0);
    frontend_step(&f,0,1,1);assert(f.selection==0);frontend_step(&f,0,0,1);frontend_step(&f,0,1,1);assert(f.selection==1);
    for(i=0;i<2;i++){render_frontend(FRONT_TITLE,i,1,1);render_frontend(FRONT_CONTROLS,i,0,1);}
    render_frontend(FRONT_TITLE,0,1,0);assert(triangles>0);
    frontend_init(&f);frontend_step(&f,FRONT_PREV,0,1);assert(f.selection==2);
    assert(frontend_step(&f,FRONT_ACCEPT,0,1)&&f.two_player);
    f.page=FRONT_RESULTS;f.selection=0;assert(frontend_step(&f,FRONT_ACCEPT,0,1)&&f.two_player);
    f.page=FRONT_RESULTS;frontend_step(&f,FRONT_BACK,0,1);assert(!f.two_player);
    puts("PASS: menu navigation, one step per stick deflection, disconnected controller, Start/Controls/back/return flow and finite on-screen UI geometry");return 0;
}
