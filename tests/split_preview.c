/* v0.29: dedicated forward/back pitch input, enlarged ball, then boosted turns. */
#define main reference_snapshot_main
#include "snapshot.c"
#undef main
int main(void){
    Game g;Match m;Camera cameras[2]={0};int modes[2]={0,0},scene,frame,tick;
    for(scene=0;scene<2;scene++){
        game_init(&g);match_init(&m);memset(cameras,0,sizeof(cameras));
        g.car=(Vec3){-3,0,-28};g.cpu=(Vec3){3,0,28};
        if(scene){g.car=(Vec3){ARENA_X-12,0,-20};g.cpu=(Vec3){ARENA_X-18,0,24};g.ball=(Vec3){ARENA_X-10,12,0};g.ball_velocity.y=3;}
        modes[0]=scene;modes[1]=scene;
        for(frame=0;frame<80;frame++){
            for(tick=0;tick<6;tick++){
                Input blue={scene?.8f:0,scene?1:0,0,scene&&frame<55,frame==8||(frame>=18&&frame<42),scene?0:1};
                Input orange={scene?-.8f:0,scene?1:0,0,scene&&frame<55,frame==12||(frame>=22&&frame<46),scene?0:-1};
                match_step_two(&m,&g,blue,orange);
            }
            camera_update_player(&cameras[0],&g,0,modes[0],.05f,frame==0);
            camera_update_player(&cameras[1],&g,1,modes[1],.05f,frame==0);
            memset(rgb,0,sizeof(rgb));memset(depth,0,sizeof(depth));count=0;
            render_split_frame(&g,cameras,&m,modes,0,0);render_split_fps(0);
            printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);
        }
    }
    return 0;
}
