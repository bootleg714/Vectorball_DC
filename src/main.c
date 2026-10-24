#include <kos.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>
#include <math.h>
#include "render.h"
#include "audio.h"
#include "title_art.h"
KOS_INIT_FLAGS(INIT_DEFAULT);
static pvr_poly_hdr_t header;
void backend_triangle(Screen a,Screen b,Screen c,uint32_t col){
    pvr_vertex_t vert;
    Screen p[3]={a,b,c};int i;
    for(i=0;i<3;i++){
        vert.flags=i==2?PVR_CMD_VERTEX_EOL:PVR_CMD_VERTEX;
        vert.x=p[i].x;vert.y=p[i].y;vert.z=p[i].z;
        vert.u=vert.v=0;vert.argb=col;vert.oargb=0;
        pvr_prim(&vert,sizeof(vert));
    }
}
static pvr_ptr_t title_texture;
static pvr_poly_hdr_t title_header;
static int title_prepare(void){
    pvr_poly_cxt_t context;
    if(title_texture)return 1;
    title_texture=pvr_mem_malloc(sizeof(title_pixels));
    if(!title_texture)return 0;
    pvr_txr_load((void *)title_pixels,title_texture,sizeof(title_pixels));
    pvr_poly_cxt_txr(&context,PVR_LIST_OP_POLY,PVR_TXRFMT_RGB565|PVR_TXRFMT_NONTWIDDLED,512,512,title_texture,PVR_FILTER_BILINEAR);
    context.gen.culling=PVR_CULLING_NONE;pvr_poly_compile(&title_header,&context);
    return 1;
}
static void title_draw(void){
    pvr_vertex_t vert;int i;
    static const float x[4]={0,640,0,640},y[4]={0,0,480,480};
    pvr_prim(&title_header,sizeof(title_header));
    for(i=0;i<4;i++){
        vert.flags=i==3?PVR_CMD_VERTEX_EOL:PVR_CMD_VERTEX;vert.x=x[i];vert.y=y[i];vert.z=1;
        vert.u=i&1?1:0;vert.v=i&2?1:0;vert.argb=0xffffffff;vert.oargb=0;
        pvr_prim(&vert,sizeof(vert));
    }
}
static cont_state_t *port_pad(int port){
    maple_device_t *dev=maple_enum_dev(port,0);
    return dev&&(dev->info.functions&MAPLE_FUNC_CONTROLLER)?(cont_state_t *)maple_dev_status(dev):NULL;
}
static int released(const cont_state_t *pad){
    return pad&&!(pad->buttons&(CONT_A|CONT_B|CONT_START|CONT_X|CONT_Y))&&pad->rtrig<25&&pad->ltrig<25;
}
static Input controller_input(const Game *g,int player,const cont_state_t *pad){
    Input in={0};float heading=player?g->cpu_heading:g->heading;Vec3 velocity=player?g->cpu_velocity:g->velocity;
    if(!pad)return in;
    in.steer=pad->joyx/128.0f;if(fabsf(in.steer)<.15f)in.steer=0;
    if(pad->buttons&CONT_DPAD_LEFT)in.steer=-1;
    if(pad->buttons&CONT_DPAD_RIGHT)in.steer=1;
    in.pitch=-pad->joyy/128.0f;if(fabsf(in.pitch)<.25f)in.pitch=0;
    if(pad->buttons&CONT_DPAD_UP)in.pitch=1;
    if(pad->buttons&CONT_DPAD_DOWN)in.pitch=-1;
    in.throttle=pad->rtrig/255.0f;
    if(pad->ltrig>25){float forward=velocity.x*sinf(heading)+velocity.z*cosf(heading);if(forward>1)in.brake=pad->ltrig/255.0f;else in.throttle=-pad->ltrig/255.0f;}
    if(pad->buttons&CONT_DPAD_UP)in.throttle=1;
    if(pad->buttons&CONT_DPAD_DOWN)in.throttle=-1;
    in.boost=(pad->buttons&CONT_A)!=0;in.jump=(pad->buttons&CONT_B)!=0;return in;
}
int main(int argc,char **argv){
    Game g;Match match;Frontend front;int wait_release=0;Camera camera={0},camera2={0};uint32_t previous=0,previous2=0;int paused=0,ball_camera=0,ball_camera2=0,wait_release2=0;
    uint64_t last;float fps=0,fps_time=0;int fps_frames=0;float accumulator=0;pvr_poly_cxt_t context;
    (void)argc;(void)argv;
    vid_set_mode(DM_640x480,PM_RGB565);
    {pvr_init_params_t params={.opb_sizes={PVR_BINSIZE_16,PVR_BINSIZE_0,PVR_BINSIZE_0,PVR_BINSIZE_0,PVR_BINSIZE_0},.vertex_buf_size=2*1024*1024,.opb_overflow_count=3};
        if(pvr_init(&params)<0)return 1;
    }
    pvr_set_bg_color(.025f,.035f,.075f);
    pvr_poly_cxt_col(&context,PVR_LIST_OP_POLY);
    context.gen.culling=PVR_CULLING_NONE;
    pvr_poly_compile(&header,&context);
    audio_init();
    frontend_init(&front);title_prepare();game_init(&g);match_init(&match);camera_update(&camera,&g,0,0,1);last=timer_ms_gettime64();
    for(;;){
        maple_device_t *dev=maple_enum_type(0,MAPLE_FUNC_CONTROLLER);
        cont_state_t *pad=front.two_player?port_pad(0):(dev?(cont_state_t *)maple_dev_status(dev):NULL);
        cont_state_t *pad2=port_pad(1);
        uint32_t buttons=pad?pad->buttons:0,pressed=buttons&~previous;
        uint32_t buttons2=pad2?pad2->buttons:0,pressed2=buttons2&~previous2;int missing;
        uint64_t now=timer_ms_gettime64();float dt=(now-last)*.001f;Input in={0},in2={0};float frame_time=dt;last=now;
        if(dt>.1f)dt=.1f;
        game_clear_events(&g);
        if(front.page==FRONT_MATCH&&paused&&(pressed&CONT_B)){
            audio_reset();frontend_return(&front);paused=0;accumulator=0;
            /* The GPU may still be reading last frame's primitives. */
            pvr_wait_ready();title_prepare();pressed=0;
        }
        if(front.page!=FRONT_MATCH){
            unsigned menu=0;float axis=pad?pad->joyx/128.0f:0;
            if(pad&&fabsf(pad->joyy/128.0f)>fabsf(axis))axis=pad->joyy/128.0f;
            if(pressed&(CONT_DPAD_LEFT|CONT_DPAD_UP))menu|=FRONT_PREV;
            if(pressed&(CONT_DPAD_RIGHT|CONT_DPAD_DOWN))menu|=FRONT_NEXT;
            if(pressed&(CONT_A|CONT_START))menu|=FRONT_ACCEPT;
            if(pressed&CONT_B)menu|=FRONT_BACK;
            if(frontend_step(&front,menu,axis,pad!=NULL)){
                audio_reset();game_init(&g);match_init(&match);paused=ball_camera=ball_camera2=0;accumulator=0;fps=fps_time=0;fps_frames=0;wait_release=wait_release2=1;
                camera_update(&camera,&g,0,0,1);if(front.two_player)camera_update_player(&camera2,&g,1,0,0,1);pressed=pressed2=0;dt=0;
                pvr_wait_ready();if(title_texture){pvr_mem_free(title_texture);title_texture=NULL;}
            }
        }
        missing=(!pad?1:0)|((front.two_player&&!pad2)?2:0);
        if(front.page==FRONT_MATCH){
        if(wait_release){
            if(released(pad))wait_release=0;
            pressed=0;
        }
        if(wait_release2){if(released(pad2))wait_release2=0;pressed2=0;}
        if((pressed&CONT_START)||(front.two_player&&(pressed2&CONT_START)))paused=!paused;
        if(pressed&CONT_X)ball_camera=!ball_camera;
        if(front.two_player&&(pressed2&CONT_X))ball_camera2=!ball_camera2;
        if(pressed&CONT_Y){audio_reset();if(paused){game_init(&g);match_init(&match);}else if(match.phase!=MATCH_FINISHING)game_reset(&g);camera_update(&camera,&g,ball_camera,0,1);if(front.two_player)camera_update_player(&camera2,&g,1,ball_camera2,0,1);}
        if(pad&&!wait_release)in=controller_input(&g,0,pad);
        if(front.two_player&&pad2&&!wait_release2)in2=controller_input(&g,1,pad2);
        /* Pause when controller is absent; do not leave a stuck throttle active. */
        if(!paused&&!missing){accumulator+=dt;while(accumulator>=STEP&&match.phase!=MATCH_OVER){if(front.two_player)match_step_two(&match,&g,in,in2);else match_step(&match,&g,in);accumulator-=STEP;}}
        else accumulator=0;
        audio_update(&g,in,paused||missing,dt);
        camera_update(&camera,&g,ball_camera,dt,0);
        if(front.two_player)camera_update_player(&camera2,&g,1,ball_camera2,dt,0);
        if(match.phase==MATCH_OVER){
            audio_reset();front.page=FRONT_RESULTS;front.selection=0;front.stick_latched=1;
            paused=0;accumulator=0;pvr_wait_ready();title_prepare();
        }
        }else{accumulator=0;audio_update(&g,in,1,dt);}
        pvr_wait_ready();pvr_scene_begin();pvr_list_begin(PVR_LIST_OP_POLY);
        if(front.page!=FRONT_MATCH&&title_texture)title_draw();
        pvr_prim(&header,sizeof(header));
        if(front.page==FRONT_MATCH){
            if(front.two_player){Camera cameras[2]={camera,camera2};int modes[2]={ball_camera,ball_camera2};render_split_frame(&g,cameras,&match,modes,paused,missing);render_split_fps(paused||missing?0:fps);}
            else {render_frame(&g,&camera,paused||missing,ball_camera);render_match_clock(&match);}
        }
        else if(front.page==FRONT_RESULTS)render_results(&g,front.selection,pad!=NULL,title_texture!=NULL);
        else render_frontend(front.page,front.selection,pad!=NULL,title_texture!=NULL);
        pvr_list_finish();pvr_scene_finish();
        if(front.page==FRONT_MATCH&&front.two_player&&!paused&&!missing){
            fps_time+=frame_time;fps_frames++;
            if(fps_time>=1){fps=fps_frames/fps_time;fps_time=0;fps_frames=0;}
        }else {fps_time=0;fps_frames=0;}
        previous=buttons;previous2=buttons2;
    }
    return 0;
}
