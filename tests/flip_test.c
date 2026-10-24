/* Include the simulation to isolate real contact response as well as public inputs. */
#include "../src/game.c"
#include <assert.h>
#include <stdio.h>
static const Input idle={0},jump={0,0,0,0,1,0};
static void setup(Game *g){game_init(g);g->car=(Vec3){-20,0,0};g->cpu=(Vec3){20,0,0};g->ball=(Vec3){70,BALL_R,0};g->cpu_heading=0;g->cpu_surface.forward=(Vec3){0,0,1};}
static void step(Game *g,int player,Input in){game_step_two(g,player?idle:in,player?in:idle,STEP);}
static Game contact(int flip){Game g;setup(&g);g.cpu_enabled=0;g.car=(Vec3){0,10,0};g.ball=(Vec3){0,10.85f,3};g.velocity.z=20;g.car_surface.flip_time=flip?.5f:0;g.car_surface.flip_hit_available=flip;return g;}
int main(void){
    int player,dir,i,yaw;float longest=0;
    for(player=0;player<2;player++)for(dir=-1;dir<=1;dir+=2)for(yaw=0;yaw<4;yaw++){
        Game g;CarSurface *s;Vec3 *pos,*v;float heading=yaw*1.5707963268f,minimum=1;setup(&g);
        s=player?&g.cpu_surface:&g.car_surface;pos=player?&g.cpu:&g.car;v=player?&g.cpu_velocity:&g.velocity;
        if(player)g.cpu_heading=heading;else g.heading=heading;
        s->forward=(Vec3){sinf(heading),0,cosf(heading)};
        step(&g,player,jump);for(i=0;i<30;i++)step(&g,player,idle);
        /* Steering held on the second press must still produce pitch, not roll. */
        step(&g,player,(Input){1,0,dir<0?1:0,0,1,0});
        assert(s->flip_time>0&&!s->air_jump_available&&s->flip_axis.x*cosf(heading)*dir+s->flip_axis.z*-sinf(heading)*dir>.99f);
        for(i=0;i<150&&s->flip_time>0;i++){
            step(&g,player,(Input){1,0,0,0,1,0});minimum=fminf(minimum,s->up.y);
            assert(isfinite(pos->y)&&pos->y<15&&s->recovery_time==0&&!s->air_jump_available);
            assert(fabsf(angle_wrap((player?g.cpu_heading:g.heading)-heading))<.01f);
        }
        assert(i<125&&minimum<-.98f&&s->up.y>.995f&&!s->flip_hit_available);
        longest=fmaxf(longest,i*STEP);
        for(i=0;i<360;i++)step(&g,player,idle);
        assert(pos->y<.1f&&s->up.y>.98f&&fabsf(v->y)<.2f&&!s->flip_time);
        step(&g,player,jump);assert(s->air_jump_available); /* Recharged by a real landing. */
        game_reset(&g);assert(!g.car_surface.flip_time&&!g.cpu_surface.flip_time&&!g.car_surface.flip_hit_available&&!g.cpu_surface.flip_hit_available);
    }
    /* Flip out of a deliberate first-jump roll; local pitch returns to that pose. */
    for(dir=-1;dir<=1;dir+=2){
        Game g;Vec3 start;setup(&g);step(&g,0,jump);
        for(i=0;i<20;i++)step(&g,0,(Input){(float)dir,0,0,0,1,0});
        step(&g,0,idle);start=g.car_surface.up;g.car.y=20;
        step(&g,0,jump);
        for(i=0;i<160&&g.car_surface.flip_time>0;i++){step(&g,0,idle);g.car.y=20;}
        assert(i<150&&vec_dot(start,g.car_surface.up)>.995f);
    }
    {Game plain=contact(0),flip=contact(1),again;float ordinary;
        car_ball(&plain,plain.car,&plain.velocity,0,&plain.car_surface);ordinary=plain.ball_velocity.z;
        car_ball(&flip,flip.car,&flip.velocity,0,&flip.car_surface);
        assert(fabsf(ordinary-20*1.45f*1.1f)<.001f);
        assert(fabsf(flip.ball_velocity.z/ordinary-1.15f)<.001f&&!flip.car_surface.flip_hit_available);
        again=contact(1);again.car_surface.flip_hit_available=0;car_ball(&again,again.car,&again.velocity,0,&again.car_surface);
        assert(fabsf(again.ball_velocity.z-ordinary)<.001f);
        flip=contact(1);flip.ball_velocity.z=30;car_ball(&flip,flip.car,&flip.velocity,0,&flip.car_surface);
        assert(flip.ball_velocity.z==30&&flip.car_surface.flip_hit_available); /* Separating contact never spends it. */
        flip=contact(1);flip.velocity=(Vec3){0};flip.ball.y=10;flip.car_surface.angular_velocity=(Vec3){10,0,0};
        car_ball(&flip,flip.car,&flip.velocity,0,&flip.car_surface);
        assert(flip.ball_velocity.z>0&&flip.ball_velocity.y<0&&!flip.car_surface.flip_hit_available); /* Stationary rotating swat. */
        flip.ball_velocity=(Vec3){1000,1000,1000};limit_ball_speed(&flip);
        assert(sqrtf(vec_dot(flip.ball_velocity,flip.ball_velocity))<BALL_SPEED+.001f);
    }
    {Game g;setup(&g);g.car.y=12;step(&g,0,jump);assert(!g.car_surface.flip_time&&!g.car_surface.air_jump_available);
        /* Reverse throttle also requests a backflip (left trigger at low speed). */
        setup(&g);step(&g,0,jump);step(&g,0,idle);step(&g,0,(Input){0,-1,0,0,1,0});assert(g.car_surface.flip_axis.x<-.99f);
    }
    printf("PASS: 16 player/direction/heading flips, inverted midpoint/upright completion <=%.2fs, held steering priority, landing/recharge/reset; +10%% ordinary hits, single +15%% flip hit, separating/rotating swat and ball cap\n",longest);
    return 0;
}
