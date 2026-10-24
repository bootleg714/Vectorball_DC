/* Runs the actual target main loop against host API stand-ins, not an emulator. */
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#define main target_main
#include "../src/main.c"
#undef main
static jmp_buf finished;
static int frame,match_frames,menu_frames,allocs,frees,fail_alloc,clock_step;
static unsigned char texture[512*512*2];
const uint16_t title_pixels[512*512]={0};
static maple_device_t devices[2]={{{MAPLE_FUNC_CONTROLLER}},{{MAPLE_FUNC_CONTROLLER}}};
static cont_state_t states[2];static int last_ticks;
static const uint32_t blue_buttons[]={0,CONT_DPAD_LEFT,0,CONT_A,CONT_A,0,0,CONT_X,0,0,0,CONT_START,0,0,0,0,CONT_START,0,CONT_Y,0,CONT_B,0,CONT_DPAD_LEFT,CONT_A,CONT_A,0};
static const uint32_t orange_buttons[]={0,0,0,CONT_START,CONT_START,0,CONT_A,0,CONT_X,CONT_START,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
void vid_set_mode(int a,int b){(void)a;(void)b;}
uint64_t timer_ms_gettime64(void){return (uint64_t)frame*(unsigned)clock_step;}
maple_device_t *maple_enum_dev(int port,int unit){assert(unit==0&&port>=0&&port<2);if((port==1&&frame==12)||(port==0&&frame==14))return NULL;return &devices[port];}
maple_device_t *maple_enum_type(int a,int b){(void)a;(void)b;return frame==14?&devices[1]:&devices[0];}
void *maple_dev_status(maple_device_t *d){
    int player=d==&devices[1];cont_state_t *state=&states[player];memset(state,0,sizeof(*state));
    state->buttons=player?orange_buttons[frame]:blue_buttons[frame];
    if(frame==6)state->rtrig=player?128:255;
    return state;
}
int pvr_init_defaults(void){return 0;}
int pvr_init(const pvr_init_params_t *p){assert(p->vertex_buf_size==2*1024*1024&&p->opb_sizes[0]==16&&p->opb_sizes[2]==0&&p->opb_overflow_count==3);return 0;}
void pvr_set_bg_color(float a,float b,float c){(void)a;(void)b;(void)c;}
void pvr_poly_cxt_col(pvr_poly_cxt_t *c,int list){(void)c;(void)list;}
void pvr_poly_compile(pvr_poly_hdr_t *h,const pvr_poly_cxt_t *c){(void)h;(void)c;}
void pvr_poly_cxt_txr(pvr_poly_cxt_t *c,int l,int f,int w,int h,pvr_ptr_t m,int filter){(void)c;(void)l;(void)f;(void)filter;assert(w==512&&h==512&&m==texture);}
pvr_ptr_t pvr_mem_malloc(size_t n){assert(n==sizeof(texture));allocs++;return fail_alloc?NULL:texture;}
void pvr_mem_free(pvr_ptr_t m){assert(m==texture);frees++;}
void pvr_txr_load(const void *s,pvr_ptr_t d,size_t n){assert(s==title_pixels&&d==texture&&n==sizeof(texture));}
void pvr_prim(const void *p,size_t n){(void)p;(void)n;}
void pvr_wait_ready(void){}
void pvr_scene_begin(void){}
void pvr_list_begin(int list){(void)list;}
void pvr_list_finish(void){}
void pvr_scene_finish(void){if(++frame==(int)(sizeof(blue_buttons)/sizeof(blue_buttons[0])))longjmp(finished,1);}
void camera_update(Camera *c,const Game *g,int b,float dt,int snap){(void)c;(void)g;(void)b;(void)dt;(void)snap;}
void camera_update_player(Camera *c,const Game *g,int player,int b,float dt,int snap){assert(player==1);camera_update(c,g,b,dt,snap);}
void render_split_fps(float fps){
    if(clock_step==250&&frame==7)assert(fps>3.9f&&fps<4.1f);
    if(frame==9||frame==12||frame==14||frame==16)assert(fps==0);
}
void render_split_frame(const Game *g,const Camera cameras[2],const Match *m,const int modes[2],int paused,int missing){
    (void)cameras;match_frames++;
    assert(missing==(frame==12?2:frame==14?1:0));
    assert(paused==((frame>=9&&frame<=10)||(frame>=16&&frame<=19)));
    if(frame==9||frame==10||frame==12||frame==14||frame==16||frame==17||frame==19)assert(m->ticks_left==last_ticks);
    if(frame==18)assert(m->ticks_left==MATCH_TICKS&&g->blue_score==0&&g->orange_score==0&&g->boost==1&&g->cpu_boost==1);
    if(frame==3||frame==4)assert(g->boost==1&&g->cpu_boost==1);
    if(frame==6)assert(g->velocity.z>0&&g->cpu_velocity.z<0&&g->boost==1&&g->cpu_boost<1);
    if(frame==7)assert(modes[0]==1&&modes[1]==0);
    if(frame==8)assert(modes[0]==1&&modes[1]==1);
    if(frame==23)assert(modes[0]==0&&modes[1]==0&&m->ticks_left==MATCH_TICKS);
    last_ticks=m->ticks_left;
}
void render_frame(const Game *g,const Camera *c,int paused,int b){(void)g;(void)c;(void)paused;(void)b;assert(0);}
void render_match_clock(const Match *m){(void)m;assert(0);}
void render_results(const Game *g,int s,int c,int a){(void)g;(void)s;(void)c;(void)a;assert(0);}
void render_frontend(FrontPage page,int selected,int connected,int artwork){
    menu_frames++;assert(page==FRONT_TITLE&&connected&&artwork==!fail_alloc);
    assert(selected==(frame==1||frame==2||frame==22?2:0));
}
int audio_init(void){return 1;}
void audio_reset(void){}
void audio_update(const Game *g,Input in,int paused,float dt){(void)g;(void)in;(void)dt;if(frame==12||frame==14)assert(paused);}

int main(void){
    for(clock_step=17;clock_step<=250;clock_step+=233)for(fail_alloc=0;fail_alloc<=1;fail_alloc++){
        frame=match_frames=menu_frames=allocs=frees=last_ticks=0;title_texture=NULL;
        if(!setjmp(finished))target_main(0,NULL);
        assert(match_frames==20&&menu_frames==6&&allocs==2&&frees==(fail_alloc?0:2));
    }
    {Game g;cont_state_t pad={0};Input a,b;game_init(&g);g.velocity.z=-20;g.cpu_velocity.z=-10;pad.ltrig=255;
        a=controller_input(&g,0,&pad);b=controller_input(&g,1,&pad);assert(a.throttle<0&&a.brake==0&&b.brake==1);}
    /* Actual Dreamcast pad decoding -> fixed-step simulation, both players.
       Vertical stick chooses pitch independently of held acceleration/braking. */
    {int player,sign,mode,tick,sub,steps;
     for(player=0;player<2;player++)for(sign=-1;sign<=1;sign+=2)for(mode=0;mode<3;mode++){
        Game g;cont_state_t pad={0};CarSurface *surface;float minimum=1;
        game_init(&g);g.car=(Vec3){-20,0,0};g.cpu=(Vec3){20,0,0};g.ball=(Vec3){70,BALL_R,0};
        surface=player?&g.cpu_surface:&g.car_surface;steps=mode==0?1:mode==1?4:8;
        pad.joyy=sign>0?-128:127;pad.buttons=CONT_B;
        for(tick=0;tick<32/steps;tick++){
            Input in=controller_input(&g,player,&pad);
            for(sub=0;sub<steps;sub++)game_step_two(&g,player?(Input){0}:in,player?in:(Input){0},STEP);
            pad.buttons=0; /* First B press, then a visible release. */
        }
        assert(surface->air_jump_available&&!surface->flip_time);
        pad.buttons=CONT_B; /* Second edge with stick held forward/back. */
        if(sign>0)pad.ltrig=255;else pad.rtrig=255; /* Opposite trigger cannot override stick. */
        for(tick=0;tick<145/steps;tick++){
            Input in=controller_input(&g,player,&pad);
            assert(in.pitch*sign>.98f);
            for(sub=0;sub<steps;sub++){
                game_step_two(&g,player?(Input){0}:in,player?in:(Input){0},STEP);
                minimum=fminf(minimum,surface->up.y);
                if(tick==0&&sub==0)assert(surface->flip_time>0&&surface->flip_axis.x*sign*(player?-1:1)>.99f);
            }
        }
        assert(minimum<-.98f&&!surface->air_jump_available&&!surface->flip_time&&surface->up.y>.98f);
     }
     {Game g;cont_state_t pad={0};Input in;game_init(&g);pad.joyy=20;
      in=controller_input(&g,0,&pad);assert(in.pitch==0&&in.throttle==0);
      pad.buttons=CONT_DPAD_UP;in=controller_input(&g,0,&pad);assert(in.pitch==1);
      pad.buttons=CONT_DPAD_DOWN;in=controller_input(&g,1,&pad);assert(in.pitch==-1);
     }
    }
    puts("PASS: real controller vertical-stick forward/back flips for both players at 120/30/15fps input cadence, stick priority over opposite trigger, deadzone and D-pad direction");
    puts("PASS: actual two-pad loop: versus selection, independent triggers/cameras, P2 pause/P1 resume, A/B-port disconnect freezes without reassignment, shared reset/title/restart, held-button isolation, own-car reverse/braking, texture lifecycle/fallback");return 0;
}
