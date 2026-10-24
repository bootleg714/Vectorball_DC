/* Host close-ups: roof and side recovery, left/right air roll then recovery. */
#define main reference_snapshot_main
#include "snapshot.c"
#undef main
int main(void){
    Game g;Camera c={0};int scene,frame,tick;
    for(scene=0;scene<4;scene++){
        game_init(&g);g.cpu_enabled=0;g.car=(Vec3){0,0,0};g.ball=(Vec3){30,BALL_R,30};
        if(scene<2){
            g.car.y=1.5f;g.car_surface.dynamic=1;g.car_surface.forward=(Vec3){0,0,1};
            g.car_surface.up=scene==0?(Vec3){0,-1,0}:(Vec3){1,0,0};
            for(tick=0;tick<120;tick++)game_step(&g,(Input){0},STEP);
        }
        c.eye=(Vec3){5,3.8f,-7};c.forward=unit((Vec3){-5,-2.8f,7});
        c.right=unit((Vec3){c.forward.z,0,-c.forward.x});c.up=(Vec3){c.forward.y*c.right.z,-c.forward.x*c.right.z+c.forward.z*c.right.x,-c.forward.y*c.right.x};c.focal=350;
        for(frame=0;frame<80;frame++){
            Input in={0};
            if(scene<2)in.jump=frame==8;
            else {in.jump=(frame>=8&&frame<36)||frame==46;if(frame>=9&&frame<28)in.steer=scene==2?-1:1;}
            for(tick=0;tick<6;tick++)game_step(&g,in,STEP);
            memset(rgb,0,sizeof(rgb));memset(depth,0,sizeof(depth));count=0;render_frame(&g,&c,0,0);
            printf("P6\n640 480\n255\n");fwrite(rgb,1,sizeof(rgb),stdout);
        }
    }
    return 0;
}
