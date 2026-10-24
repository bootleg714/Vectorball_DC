/* Production grip helper and physical solver, driven through public game inputs. */
#include "../src/vehicle.c"
#include <assert.h>
#include <stdio.h>
int main(void){
    int player,axis,side,boosted,mode,i,cases=0;float worst_return=0,minimum_alignment=1,maximum_gap=0;int contact_ticks=0;
    {Vec3 wall={1,0,0},floor={0,1,0},roof={0,-1,0};float previous=1;
        for(i=0;i<=100;i++){
            float h=ARENA_HEIGHT*i/100,grip=surface_grip(h,wall);
            assert(grip<=previous+.00001f&&grip>=0&&grip<=1);
            if(i<=40)assert(grip==1);
            if(i>=78)assert(grip==0);
            assert(surface_grip(h,floor)==1&&surface_grip(h,roof)==0);previous=grip;
        }
    }
    for(player=0;player<2;player++)for(axis=0;axis<2;axis++)for(side=-1;side<=1;side+=2)for(boosted=0;boosted<2;boosted++)for(mode=0;mode<6;mode++){
        Game g;CarSurface *s;Vec3 *p,*v;float *fuel;int saw_slide=0,returned=0;float start;
        game_init(&g);g.car=(Vec3){-30,0,-50};g.cpu=(Vec3){30,0,50};g.ball=(Vec3){0,BALL_R,0};
        s=player?&g.cpu_surface:&g.car_surface;p=player?&g.cpu:&g.car;v=player?&g.cpu_velocity:&g.velocity;fuel=player?&g.cpu_boost:&g.boost;
        start=mode>=4?0:mode==3?12:mode==2?ARENA_HEIGHT-2.5f:ARENA_HEIGHT*.8f;
        *p=axis?(Vec3){30,start,side*(ARENA_Z-.06f)}:(Vec3){side*(ARENA_X-.06f),start,0};
        s->dynamic=1;s->up=axis?(Vec3){0,0,-side}:(Vec3){-side,0,0};
        s->forward=mode==1?(axis?(Vec3){1,0,0}:(Vec3){0,0,1}):(Vec3){0,1,0};
        if(mode==1)*v=mul(s->forward,12);
        if(mode==2)v->y=BOOST_SPEED;
        if(mode==3)v->y=10;
        if(mode>=4){
            float heading=(axis?(side>0?0:3.14159265f):side*1.5707963268f)+(mode==5?.18f:0);
            *p=axis?(Vec3){30,0,side*(ARENA_Z-15)}:(Vec3){side*(ARENA_X-15),0,0};
            s->dynamic=0;s->up=(Vec3){0,1,0};s->forward=(Vec3){sinf(heading),0,cosf(heading)};
            *v=mul(s->forward,20);
            if(player)g.cpu_heading=heading;else g.heading=heading;
        }
        for(i=0;i<120*12;i++){
            Vec3 before=*p;Input active={0,1,0,boosted,0,0},idle={0};float travel;
            *fuel=1; /* Sustained boost: falling must not depend on tank depletion. */
            game_step_two(&g,player?idle:active,player?active:idle,STEP);
            travel=sqrtf(dot(sub(*p,before),sub(*p,before)));
            assert(isfinite(p->y)&&isfinite(travel)&&p->y>-.1f&&p->y<ARENA_HEIGHT);
            assert(travel<1.0f&&sqrtf(dot(*v,*v))<=BOOST_SPEED+.02f);
            if(s->upper_wall_sliding)saw_slide=1;
            /* The goal opening has no wall below the crossbar; do not require contact over empty space. */
            if(i>5&&s->upper_wall_sliding&&p->y+s->up.y*.85f>BANK_H+3&&!(axis&&fabsf(p->x)<GOAL_W+1.5f&&p->y<GOAL_H+2)){
                Vec3 wall_normal=axis?(Vec3){0,0,-side}:(Vec3){-side,0,0};
                float alignment=dot(s->up,wall_normal),gap=axis?ARENA_Z-side*p->z:ARENA_X-side*p->x;
                minimum_alignment=fminf(minimum_alignment,alignment);maximum_gap=fmaxf(maximum_gap,gap);
                if(s->grounded_wheels<2||alignment<.95f||gap>.5f){fprintf(stderr,"DETACHED player%d axis%d side%d boost%d mode%d tick%d height%.2f wheels%d align%.3f gap%.3f\n",player,axis,side,boosted,mode,i,p->y,s->grounded_wheels,alignment,gap);fprintf(stderr,"pos %.3f %.3f %.3f forward %.3f %.3f %.3f cooldown %.3f hit0 %d dist%.3f\n",p->x,p->y,p->z,s->forward.x,s->forward.y,s->forward.z,s->cooldown,s->wheels[0].hit,s->wheels[0].distance);return 1;}
                contact_ticks++;
            }
            if(saw_slide&&p->y+s->up.y*.85f<BANK_H+1&&!s->upper_wall_sliding){returned=1;break;}
        }
        assert(saw_slide&&returned);worst_return=fmaxf(worst_return,i*STEP);cases++;
        game_reset(&g);assert(!g.car_surface.upper_wall_sliding&&!g.cpu_surface.upper_wall_sliding);
    }
    /* Upside-down ceiling wheel contacts cannot adhere even with full boost. */
    for(player=0;player<2;player++){
        Game g;CarSurface *s;Vec3 *p;int dropped=0;
        game_init(&g);g.car=(Vec3){-20,0,0};g.cpu=(Vec3){20,0,0};g.ball=(Vec3){0,BALL_R,30};
        s=player?&g.cpu_surface:&g.car_surface;p=player?&g.cpu:&g.car;
        *p=(Vec3){player?20:-20,ARENA_HEIGHT-.06f,0};s->dynamic=1;s->up=(Vec3){0,-1,0};s->forward=(Vec3){0,0,1};
        for(i=0;i<240;i++){
            Input in={0,1,0,1,0,0},idle={0};g.boost=g.cpu_boost=1;
            game_step_two(&g,player?idle:in,player?in:idle,STEP);
            if(p->y<ARENA_HEIGHT-5)dropped=1;
        }
        assert(dropped);
    }
    /* Wall hold must release immediately for an intentional jump/flip. */
    for(player=0;player<2;player++){
        Game g;CarSurface *s;Vec3 *p,*velocity;Input idle={0},jump={0,1,0,1,1,1};
        game_init(&g);g.car=(Vec3){-20,0,-30};g.cpu=(Vec3){20,0,30};g.ball=(Vec3){0,BALL_R,0};
        s=player?&g.cpu_surface:&g.car_surface;p=player?&g.cpu:&g.car;velocity=player?&g.cpu_velocity:&g.velocity;
        *p=(Vec3){ARENA_X-.06f,ARENA_HEIGHT*.8f,0};s->dynamic=1;s->up=(Vec3){-1,0,0};s->forward=(Vec3){0,1,0};
        for(i=0;i<3;i++)game_step_two(&g,idle,idle,STEP);
        assert(s->upper_wall_sliding&&s->grounded_wheels>=2);
        game_step_two(&g,player?idle:jump,player?jump:idle,STEP);
        assert(!s->attached&&velocity->x< -8&&s->air_jump_available);
        game_step_two(&g,idle,idle,STEP);game_step_two(&g,player?idle:jump,player?jump:idle,STEP);
        assert(s->flip_time>0&&!s->air_jump_available);
    }
    printf("PASS: %d upper slide ticks retain >=2 wheels, alignment>=%.3f and wall gap<=%.3f; ",contact_ticks,minimum_alignment,maximum_gap);
    printf("monotonic upper-wall grip, full lower/floor grip and zero ceiling grip; %d human/wall/direction/boost fixtures return to the lower bank (worst%.2fs), no depleted-tank dependency, bounded motion/reset both ceiling drops and intentional high-wall jump/flip releases\n",cases,worst_return);
    return 0;
}
