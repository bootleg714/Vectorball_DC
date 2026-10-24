/* Runs the actual target main loop against host API stand-ins, not an emulator. */
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#define match_init test_match_init
#define match_step test_match_step
#define main target_main
#include "../src/main.c"
#undef main
#undef match_init
#undef match_step
void match_init(Match *m);
void match_step(Match *m,Game *g,Input in);
void test_match_init(Match *m){match_init(m);m->ticks_left=4;}
void test_match_step(Match *m,Game *g,Input in){if(m->phase==MATCH_REGULATION&&m->ticks_left==1)g->blue_score=1;match_step(m,g,in);}
static jmp_buf finished;
static int frame,match_frames,menu_frames,allocs,frees,fail_alloc;
static unsigned char texture[512*512*2];
const uint16_t title_pixels[512*512]={0};
static maple_device_t device;
maple_device_t *maple_enum_dev(int port,int unit){(void)port;(void)unit;return NULL;}
static cont_state_t state;
static const uint32_t sequence[]={0,CONT_A,CONT_A,0,CONT_A,CONT_A,0,CONT_DPAD_RIGHT,CONT_A,0,CONT_START,CONT_START,0,CONT_B,0,CONT_A,0,0,CONT_DPAD_LEFT,CONT_A};
void vid_set_mode(int a,int b){(void)a;(void)b;}
uint64_t timer_ms_gettime64(void){return (uint64_t)frame*17;}
maple_device_t *maple_enum_type(int a,int b){(void)a;(void)b;return frame==0?NULL:&device;}
void *maple_dev_status(maple_device_t *d){(void)d;memset(&state,0,sizeof(state));state.buttons=sequence[frame];return &state;}
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
void pvr_scene_finish(void){if(++frame==(int)(sizeof(sequence)/sizeof(sequence[0])))longjmp(finished,1);}
void camera_update(Camera *c,const Game *g,int b,float dt,int snap){(void)c;(void)g;(void)b;(void)dt;(void)snap;}
void camera_update_player(Camera *c,const Game *g,int player,int b,float dt,int snap){(void)player;camera_update(c,g,b,dt,snap);}
void render_split_fps(float fps){(void)fps;}
void render_split_frame(const Game *g,const Camera cameras[2],const Match *m,const int modes[2],int paused,int missing){(void)g;(void)cameras;(void)m;(void)modes;(void)paused;(void)missing;assert(0);}
void render_frame(const Game *g,const Camera *c,int paused,int b){
    (void)c;(void)b;match_frames++;assert(!paused&&g->blue_score==0&&g->orange_score==0&&g->boost==1);
}
void render_match_clock(const Match *m){assert(m->phase==MATCH_REGULATION&&m->ticks_left>0);}
void render_results(const Game *g,int selection,int connected,int artwork){
    menu_frames++;assert(g->blue_score==1&&g->orange_score==0&&connected&&artwork==!fail_alloc);
    assert(selection==(frame==7||frame==18));
}
void render_frontend(FrontPage page,int selected,int connected,int artwork){
    menu_frames++;assert(page==FRONT_TITLE&&selected==0&&connected==(frame!=0)&&artwork==!fail_alloc);
}
int audio_init(void){return 1;}
void audio_reset(void){}
void audio_update(const Game *g,Input in,int paused,float dt){(void)g;(void)paused;(void)dt;assert(in.boost==0&&in.jump==0);}

int main(void){
    for(fail_alloc=0;fail_alloc<=1;fail_alloc++){
        frame=match_frames=menu_frames=allocs=frees=0;title_texture=NULL;
        if(!setjmp(finished))target_main(0,NULL);
        assert(match_frames==8&&menu_frames==12&&allocs==5&&frees==(fail_alloc?0:4));
    }
    puts("PASS: actual main-loop accelerated match expiry, results score, A/Start rematches, fresh clock/scores, Title/B exits, held-button isolation and results texture lifecycle/fallback");return 0;
}
