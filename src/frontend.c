#include "frontend.h"
#include <math.h>
void frontend_init(Frontend *f){f->page=FRONT_TITLE;f->selection=0;f->stick_latched=1;f->two_player=0;}
void frontend_return(Frontend *f){frontend_init(f);}
int frontend_step(Frontend *f,unsigned pressed,float axis,int connected){
    int move=0;
    if(!connected){f->stick_latched=1;return 0;}
    if(fabsf(axis)<.25f)f->stick_latched=0;
    if(f->page==FRONT_MATCH)return 0;
    if(f->page==FRONT_CONTROLS){
        if(pressed&(FRONT_BACK|FRONT_ACCEPT))f->page=FRONT_TITLE;
        return 0;
    }
    if(f->page==FRONT_RESULTS&&(pressed&FRONT_BACK)){frontend_return(f);return 0;}
    if((pressed&FRONT_PREV)&&!(pressed&FRONT_NEXT))move=-1;
    if((pressed&FRONT_NEXT)&&!(pressed&FRONT_PREV))move=1;
    if(!f->stick_latched&&fabsf(axis)>.55f){move=axis>0?1:-1;f->stick_latched=1;}
    if(move){int count=f->page==FRONT_TITLE?3:2;f->selection=(f->selection+move+count)%count;}
    /* Navigation and confirmation in one frame selects the newly highlighted item. */
    if(pressed&FRONT_ACCEPT){
        if(f->page==FRONT_RESULTS){
            if(f->selection)frontend_return(f);
            else {f->page=FRONT_MATCH;return 1;}
        }else if(f->selection==1)f->page=FRONT_CONTROLS;
        else {f->two_player=f->selection==2;f->page=FRONT_MATCH;return 1;}
    }
    return 0;
}
