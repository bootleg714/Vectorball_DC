#include "game.h"
#include "vehicle.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const Input idle={0},gas={0,1,0,1,0,0},jump={0,0,0,0,1,0};
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static void start(Game *g,int axis,int side,float speed){
    game_init(g);g->cpu_enabled=0;g->ball=(Vec3){-25,BALL_R,-25};
    g->car=axis?(Vec3){30,0,side*(ARENA_Z-15)}:(Vec3){side*(ARENA_X-15),0,0};
    g->heading=axis?(side>0?0:3.14159265f):(side*1.57079633f);
    g->velocity=(Vec3){sinf(g->heading)*speed,0,cosf(g->heading)*speed};
}
static void check(const Game *g){
    Vec3 r,u,f;game_car_basis(g,0,&r,&u,&f);
    assert(isfinite(g->car.x)&&isfinite(g->car.y)&&isfinite(g->car.z));
    assert(fabsf(dot(r,u))<.002f&&fabsf(dot(u,f))<.002f&&fabsf(dot(r,f))<.002f);
    assert(fabsf(dot(u,u)-1)<.002f&&fabsf(dot(f,f)-1)<.002f);
    assert(fabsf(g->car.x)<ARENA_X+.1f&&fabsf(g->car.z)<ARENA_Z+.1f);
    assert(g->car.y>-.1f&&g->car.y<60);
}
static void climb(Game *g,Vec3 away,float *maximum_turn){
    int i,j,k,split_contact=0,saw_bank=0,saw_wall=0;
    for(i=0;i<480;i++){
        Vec3 previous=g->car_surface.up,position=g->car;float turn,step;
        game_step(g,gas,STEP);check(g);
        turn=acosf(fmaxf(-1,fminf(1,dot(previous,g->car_surface.up))));
        *maximum_turn=fmaxf(*maximum_turn,turn);
        step=hypotf(hypotf(g->car.x-position.x,g->car.z-position.z),g->car.y-position.y);
        assert(turn<.12f&&step<.8f);
        if(g->car_surface.up.y>.2f&&g->car_surface.up.y<.95f&&g->car_surface.grounded_wheels>=2)saw_bank=1;
        for(j=0;j<4;j++)for(k=j+1;k<4;k++)if(g->car_surface.wheels[j].hit&&g->car_surface.wheels[k].hit&&dot(g->car_surface.wheels[j].normal,g->car_surface.wheels[k].normal)<.98f)split_contact=1;
        if(g->car.y>4&&g->car_surface.grounded_wheels>=3&&dot(g->car_surface.up,away)>.97f){saw_wall=1;break;}
    }
    assert(saw_bank&&saw_wall&&split_contact&&g->velocity.y>1);
}
int main(void){
    Game g;WheelContact hit;int axis,side,i;float max_turn=0;
    /* Probe distance and normals are independently checked against known surfaces. */
    assert(vehicle_probe((Vec3){0,1,0},(Vec3){0,-1,0},2,&hit));
    assert(fabsf(hit.distance-1)<.001f&&hit.normal.y>.999f);
    assert(vehicle_probe((Vec3){ARENA_X-1,5,0},(Vec3){1,0,0},2,&hit));
    assert(fabsf(hit.distance-1)<.001f&&hit.normal.x<-.999f);
    assert(!vehicle_probe((Vec3){0,5,0},(Vec3){0,1,0},2,&hit));
    for(axis=0;axis<2;axis++)for(side=-1;side<=1;side+=2){
        Vec3 away=axis?(Vec3){0,0,-side}:(Vec3){-side,0,0};
        start(&g,axis,side,20);climb(&g,away,&max_turn);
        {float before=dot(g.velocity,away);
            game_clear_events(&g);game_step(&g,jump,STEP);check(&g);
            assert(!g.car_surface.attached&&dot(g.velocity,away)>2&&dot(g.velocity,away)-before>8.5f&&g.sounds.jump>0);
        }
        for(i=0;i<120*8;i++){game_step(&g,idle,STEP);check(&g);}
        assert(g.car.y<2.5f);
    }
    /* Without wheel contact there is no surface attachment or adhesion. */
    start(&g,0,1,0);g.car=(Vec3){0,12,0};g.car_surface.dynamic=1;
    g.car_surface.angular_velocity=(Vec3){0,0,1};
    game_step(&g,idle,STEP);assert(!g.car_surface.attached&&g.velocity.y<0&&g.car_surface.up.x<0);
    /* Release on the wall: gravity must return the car to the bank/floor. */
    start(&g,0,1,20);climb(&g,(Vec3){-1,0,0},&max_turn);
    for(i=0;i<120*10;i++){game_step(&g,idle,STEP);check(&g);}
    assert(g.car.y<2.5f&&g.car.x<ARENA_X-5);
    game_reset(&g);assert(!g.car_surface.dynamic&&!g.cpu_surface.dynamic&&g.car_surface.up.y==1);
    printf("PASS: analytic wheel probes, four force-driven bank/wall climbs, mixed wheel normals, jumps/falls, contact loss, gravity descent/reset; max step rotation %.2f degrees\n",max_turn*57.2957795f);
    return 0;
}
