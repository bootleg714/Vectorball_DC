/* Exercise the production contact response without simulation drift between shots. */
#include "../src/game.c"
#include <assert.h>
#include <stdio.h>
static Vec3 shot(float yaw,float angle,int player){
    Game g;Vec3 car={player?20:-20,0,0},velocity;CarSurface *surface;float a=yaw+angle;
    game_init(&g);surface=player?&g.cpu_surface:&g.car_surface;
    surface->forward=(Vec3){sinf(yaw),0,cosf(yaw)};
    g.ball=(Vec3){car.x+sinf(a)*(BALL_R+1.65f-.01f),BALL_R,cosf(a)*(BALL_R+1.65f-.01f)};
    velocity=(Vec3){20*sinf(yaw),0,20*cosf(yaw)};
    car_ball(&g,car,&velocity,yaw,surface);
    return g.ball_velocity;
}
int main(void){
    int player,yaw,i;float pi=3.1415926536f,small_angle=0;int samples=0;
    for(player=0;player<2;player++)for(yaw=0;yaw<4;yaw++){
        float h=yaw*pi*.5f,last=-10;
        for(i=-560;i<=560;i++){
            float angle=i*.125f*pi/180,expected=20*cosf(angle)*BALL_HIT_GAIN;
            Vec3 v=shot(h,angle,player);
            float forward=v.x*sinf(h)+v.z*cosf(h),side=v.x*cosf(h)-v.z*sinf(h),out=atan2f(side,forward);
            assert(isfinite(out)&&out>last+.0001f); /* No snapping or dead zones. */
            assert(fabsf(hypotf(v.x,v.z)-expected)<.002f); /* Direction-only tuning. */
            if(i==0)assert(fabsf(out)<.00001f);
            if(i==40){small_angle=out*180/pi;assert(small_angle>3.2f&&small_angle<3.4f);}
            if(i>0)assert(out<=angle+.00001f&&out>=angle*.64f);
            last=out;samples++;
        }
    }
    /* Left/right mirror and continuity at frontal fade boundary. */
    for(i=1;i<=70;i++){
        Vec3 a=shot(0,i*pi/180,0),b=shot(0,-i*pi/180,0);
        assert(fabsf(a.x+b.x)<.001f&&fabsf(a.z-b.z)<.001f);
    }
    /* Side/rear responses remain along the original physical contact normal. */
    for(i=0;i<3;i++){
        Game g;Vec3 velocity;float a=(90+i*45)*pi/180,dist=BALL_R+1.65f-.01f;
        game_init(&g);g.car=(Vec3){0};g.ball=(Vec3){sinf(a)*dist,BALL_R,cosf(a)*dist};velocity=(Vec3){sinf(a)*20,0,cosf(a)*20};
        car_ball(&g,g.car,&velocity,0,&g.car_surface);
        assert(fabsf(g.ball_velocity.x-sinf(a)*20*BALL_HIT_GAIN)<.001f&&fabsf(g.ball_velocity.z-cosf(a)*20*BALL_HIT_GAIN)<.001f);
    }
    printf("PASS: %d distinct monotonic frontal shot angles across both cars/four headings; 5-degree contact -> %.2f-degree shot, mirrored aiming, unchanged impulse strength, center/side/rear response\n",samples,small_angle);
    return 0;
}
