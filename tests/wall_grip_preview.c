/* Actual split simulation with host inspection cameras: blue climbs from pitch, orange rolls back. */
#define main reference_snapshot_main
#include "snapshot.c"
#undef main
int main(void){
    Game g;Match m;Camera cameras[2]={0};int modes[2]={0,0},frame,tick;
    game_init(&g);match_init(&m);
    g.car=(Vec3){ARENA_X-15,0,0};g.cpu=(Vec3){-ARENA_X+.06f,ARENA_HEIGHT*.8f,0};
    g.heading=1.5707963268f;g.cpu_heading=-1.5707963268f;
    g.car_surface.dynamic=0;g.cpu_surface.dynamic=1;
    g.car_surface.up=(Vec3){0,1,0};g.cpu_surface.up=(Vec3){1,0,0};
    g.car_surface.forward=(Vec3){1,0,0};g.cpu_surface.forward=(Vec3){0,1,0};g.velocity.x=20;
    for(frame=0;frame<160;frame++){
        for(tick=0;tick<6;tick++)match_step_two(&m,&g,(Input){0,1,0,1,0,0},(Input){0,1,0,1,0,0});
        {int player;for(player=0;player<2;player++){
            Camera *c=&cameras[player];Vec3 p=player?g.cpu:g.car,up=player?g.cpu_surface.up:g.car_surface.up;
            c->eye=(Vec3){p.x+(player?12:-12),p.y+5,p.z-9};
            c->forward=unit((Vec3){p.x+up.x*.85f-c->eye.x,p.y+up.y*.85f-c->eye.y,p.z+up.z*.85f-c->eye.z});
            c->right=unit((Vec3){c->forward.z,0,-c->forward.x});
            c->up=(Vec3){c->forward.y*c->right.z,-c->forward.x*c->right.z+c->forward.z*c->right.x,-c->forward.y*c->right.x};c->focal=390;
        }}
        memset(rgb,0,sizeof(rgb));memset(depth,0,sizeof(depth));count=0;
        render_split_frame(&g,cameras,&m,modes,0,0);render_split_fps(0);
        printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);
    }
    return 0;
}
