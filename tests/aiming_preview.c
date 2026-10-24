/* Three actual ground shots: left offset, center, right offset. Offline20fps. */
#define main reference_snapshot_main
#include "snapshot.c"
#undef main
int main(void){
    int scene,frame,tick;float offsets[3]={-1.35f,0,1.35f};
    for(scene=0;scene<3;scene++){
        Game g;Match m;Camera camera={0};game_init(&g);match_init(&m);g.cpu_enabled=0;
        g.car=(Vec3){0,0,-14};g.ball=(Vec3){offsets[scene],BALL_R,0};
        for(frame=0;frame<50;frame++){
            for(tick=0;tick<6;tick++)match_step(&m,&g,(Input){0,frame<24,0,frame<14,0,0});
            camera_update(&camera,&g,1,.05f,frame==0);
            memset(rgb,0,sizeof(rgb));memset(depth,0,sizeof(depth));count=0;
            render_frame(&g,&camera,0,1);render_match_clock(&m);
            printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);
        }
    }
    return 0;
}
