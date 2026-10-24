#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
int main(void){
    Game g;int wall,direction,angle_index,i,cases=0;float minimum_entry=1,minimum_down=0,maximum_peak=0;
    for(wall=-1;wall<=1;wall+=2)for(direction=-1;direction<=1;direction+=2)for(angle_index=0;angle_index<2;angle_index++){
        float angle=angle_index?.7853981634f:.5235987756f,peak=0;int release=0,saw_entry=0,saw_descent=0,saw_exit=0;
        game_init(&g);g.cpu_enabled=0;g.ball=(Vec3){-25,BALL_R,0};
        g.car=(Vec3){wall*(ARENA_X-15),0,-direction*20};
        g.heading=atan2f(wall*cosf(angle),direction*sinf(angle));
        g.velocity=(Vec3){sinf(g.heading)*20,0,cosf(g.heading)*20};
        for(i=0;i<120*10;i++){
            Input in={0};Vec3 before=g.car;float step;
            if(g.car.y>3)release=1;
            if(!release){in.throttle=1;in.boost=1;}
            game_step(&g,in,STEP);
            step=hypotf(hypotf(g.car.x-before.x,g.car.z-before.z),g.car.y-before.y);
            assert(isfinite(step)&&step<1);
            peak=fmaxf(peak,g.car.y);
            if(!saw_entry&&g.car.y>3.5f&&g.car_surface.grounded_wheels>=2){
                float sideways=direction*g.car_surface.forward.z;
                minimum_entry=fminf(minimum_entry,sideways);assert(sideways>.30f);saw_entry=1;
            }
            if(g.car.y>3&&g.velocity.y< -1.5f&&g.car_surface.grounded_wheels>=2&&g.car_surface.forward.y< -.15f){
                float speed=sqrtf(dot(g.velocity,g.velocity));
                if(dot(g.car_surface.forward,g.velocity)/speed>.6f){saw_descent=1;minimum_down=fminf(minimum_down,g.car_surface.forward.y);}
            }
            if(saw_entry&&g.car.y<.2f&&wall*g.car.x<ARENA_X-BANK_W-1){
                float speed=sqrtf(dot(g.velocity,g.velocity));
                assert(wall*g.car_surface.forward.x<-.3f);
                assert(dot(g.car_surface.forward,g.velocity)/speed>.7f);
                assert(direction*(g.car.z+direction*20)>10);saw_exit=1;break;
            }
        }
        assert(release&&saw_entry&&saw_descent&&saw_exit);maximum_peak=fmaxf(maximum_peak,peak);cases++;
    }
    /* A head-on approach is allowed to roll back; it must not invent a sideways turn. */
    game_init(&g);g.cpu_enabled=0;g.ball.x=-25;g.car=(Vec3){ARENA_X-15,0,0};g.heading=1.5707963268f;g.velocity=(Vec3){20,0,0};
    for(i=0;i<120*4;i++){
        Input in={0,i<85?1:0,0,i<85,0,0};game_step(&g,in,STEP);
        if(g.car.y>3&&g.car_surface.grounded_wheels>=2)assert(fabsf(g.car_surface.forward.z)<.15f);
    }
    printf("PASS: %d mirrored 30/45-degree unsteered carves retain entry heading, crest, face down while descending and exit forward; minimum entry lateral %.3f, descending forward.y %.3f, peak %.2f; no forced head-on turn\n",cases,minimum_entry,minimum_down,maximum_peak);
    return 0;
}
