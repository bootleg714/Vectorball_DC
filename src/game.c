#include "game.h"
#include "vehicle.h"
#include <math.h>
#include <string.h>
static float clamp(float x,float a,float b){return x<a?a:(x>b?b:x);}
/* Shared low-poly boundary: six facets per quarter circle, open goal mouths. */
static Vec3 perimeter_point(int i){
    int q=(i/7)%4,k=i%7;
    float angle=(q*6+k)*1.5707963268f/6;
    float cx=(q==0||q==3)?ARENA_X-CORNER_R:-ARENA_X+CORNER_R;
    float cz=q<2?ARENA_Z-CORNER_R:-ARENA_Z+CORNER_R;
    return (Vec3){cx+CORNER_R*cosf(angle),0,cz+CORNER_R*sinf(angle)};
}
void arena_edge(int index,Vec3 *a,Vec3 *b){
    int i=index>=28?(index==28?6:20):index;
    *a=perimeter_point(i);*b=perimeter_point((i+1)%28);
    if(i==6||i==20){
        if(index<28)b->x=copysignf(GOAL_W,a->x);
        else a->x=copysignf(GOAL_W,b->x);
    }
}
static int edge_plane(int i,float x,float z,float *d,float *nx,float *nz){
    static Vec3 starts[ARENA_EDGES],normals[ARENA_EDGES];
    static int ready;Vec3 a;int k;
    if(!ready){
        for(k=0;k<ARENA_EDGES;k++){
            Vec3 b;float length;arena_edge(k,&starts[k],&b);
            length=hypotf(b.x-starts[k].x,b.z-starts[k].z);
            normals[k]=(Vec3){(b.z-starts[k].z)/length,0,(starts[k].x-b.x)/length};
        }
        ready=1;
    }
    a=starts[i];
    /* Corner facets apply in their corner quadrant; straight runs cover the rest. */
    if(fabsf(normals[i].x)>.001f&&fabsf(normals[i].z)>.001f&&
       (fabsf(x)<ARENA_X-CORNER_R||fabsf(z)<ARENA_Z-CORNER_R))return 0;
    /* End segments share a plane, but the central goal corridor stays flat. */
    if(fabsf(normals[i].x)<.001f&&fabsf(a.z)>ARENA_Z-.01f&&fabsf(x)<GOAL_W)return 0;
    *nx=normals[i].x;*nz=normals[i].z;
    *d=(x-a.x)*(*nx)+(z-a.z)*(*nz);return 1;
}
void arena_bank_profile(int index,float *distance,float *height){
    static float ds[BANK_SEGMENTS+1],hs[BANK_SEGMENTS+1];static int ready;int i;
    if(!ready){for(i=0;i<=BANK_SEGMENTS;i++){
        float angle=i*1.5707963268f/BANK_SEGMENTS;
        ds[i]=-BANK_W+BANK_W*sinf(angle);hs[i]=BANK_H*(1-cosf(angle));
    }ready=1;}
    *distance=ds[index];*height=hs[index];
}
Vec3 arena_bank_vertex(int edge,int end,int section){
    static Vec3 points[ARENA_EDGES][2],inset[ARENA_EDGES][2];static int ready;
    int i,j,k;float d,h;
    if(!ready){
        for(i=0;i<ARENA_EDGES;i++){
            Vec3 a,b;arena_edge(i,&a,&b);points[i][0]=a;points[i][1]=b;
            for(k=0;k<2;k++){
                Vec3 p=points[i][k],n1={0,0,0},n2={0,0,0};int found=0;
                for(j=0;j<ARENA_EDGES;j++){
                    Vec3 x,y;float length;arena_edge(j,&x,&y);
                    if(hypotf(p.x-x.x,p.z-x.z)>.001f&&hypotf(p.x-y.x,p.z-y.z)>.001f)continue;
                    length=hypotf(y.x-x.x,y.z-x.z);
                    if(!found){n1=(Vec3){(y.z-x.z)/length,0,(x.x-y.x)/length};found=1;}
                    else n2=(Vec3){(y.z-x.z)/length,0,(x.x-y.x)/length};
                }
                if(!n2.x&&!n2.z)n2=n1;
                {float denom=1+n1.x*n2.x+n1.z*n2.z;inset[i][k]=(Vec3){(n1.x+n2.x)/denom,0,(n1.z+n2.z)/denom};}
            }
        }ready=1;
    }
    arena_bank_profile(section,&d,&h);
    return (Vec3){points[edge][end].x+inset[edge][end].x*d,h+.015f,points[edge][end].z+inset[edge][end].z*d};
}
float arena_floor(float x,float z){
    int i,j;float h=0,d,nx,nz;
    for(i=0;i<ARENA_EDGES;i++)if(edge_plane(i,x,z,&d,&nx,&nz)&&d> -BANK_W){
        float value=BANK_H;
        for(j=0;j<BANK_SEGMENTS;j++){
            float a,b,ha,hb;arena_bank_profile(j,&a,&ha);arena_bank_profile(j+1,&b,&hb);
            if(d<=b){value=ha+(hb-ha)*clamp((d-a)/(b-a),0,1);break;}
        }
        h=fmaxf(h,value);
    }
    return h;
}
static void car_boundary(Vec3 *p,Vec3 *velocity){
    int i;float d,nx,nz,out;
    for(i=0;i<ARENA_EDGES;i++)if(edge_plane(i,p->x,p->z,&d,&nx,&nz)&&d> -1.8f){
        p->x-=(d+1.8f)*nx;p->z-=(d+1.8f)*nz;
        out=velocity->x*nx+velocity->z*nz;
        if(out>0){velocity->x-=1.3f*out*nx;velocity->z-=1.3f*out*nz;}
    }
    /* Cars remain inside the pitch even at the open ball goals. */
    if(fabsf(p->z)>ARENA_Z-1.8f){
        p->z=copysignf(ARENA_Z-1.8f,p->z);
        if(velocity->z*p->z>0)velocity->z*=-.3f;
    }
}
static void ball_boundary(Game *g){
    int i;float d,nx,nz,out;
    for(i=0;i<ARENA_EDGES;i++)if(edge_plane(i,g->ball.x,g->ball.z,&d,&nx,&nz)){

        int j;
        for(j=0;j<BANK_SEGMENTS;j++){
            float a,b,ha,hb,dx,dy,t,rx,ry,distance,out;
            if(d< -BANK_W-BALL_R||g->ball.y>BANK_H+BALL_R)break;
            arena_bank_profile(j,&a,&ha);arena_bank_profile(j+1,&b,&hb);dx=b-a;dy=hb-ha;
            t=clamp(((d-a)*dx+(g->ball.y-ha)*dy)/(dx*dx+dy*dy),0,1);
            rx=d-(a+t*dx);ry=g->ball.y-(ha+t*dy);distance=hypotf(rx,ry);
            if(distance<BALL_R&&distance>.0001f){
                float px=rx/distance,py=ry/distance,penetration=BALL_R-distance;
                g->ball.x+=nx*px*penetration;g->ball.z+=nz*px*penetration;g->ball.y+=py*penetration;
                out=(g->ball_velocity.x*nx+g->ball_velocity.z*nz)*px+g->ball_velocity.y*py;
                if(out<0){g->sounds.hit=fmaxf(g->sounds.hit,-out*.65f);g->ball_velocity.x-=1.64f*out*nx*px;g->ball_velocity.z-=1.64f*out*nz*px;g->ball_velocity.y-=1.64f*out*py;}
                edge_plane(i,g->ball.x,g->ball.z,&d,&nx,&nz);
            }
        }
        /* Perimeter backstop complements the bank contacts and ceiling. */
        if(edge_plane(i,g->ball.x,g->ball.z,&d,&nx,&nz)&&d> -BALL_R){
            g->ball.x-=(d+BALL_R)*nx;g->ball.z-=(d+BALL_R)*nz;
            out=g->ball_velocity.x*nx+g->ball_velocity.z*nz;
            if(out>0){g->sounds.hit=fmaxf(g->sounds.hit,out*.65f);g->ball_velocity.x-=1.8f*out*nx;g->ball_velocity.z-=1.8f*out*nz;}
        }
    }
}
static float vec_dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 vec_scale(Vec3 a,float s){return (Vec3){a.x*s,a.y*s,a.z*s};}
static Vec3 vec_add(Vec3 a,Vec3 b){return (Vec3){a.x+b.x,a.y+b.y,a.z+b.z};}
static Vec3 vec_cross(Vec3 a,Vec3 b){return (Vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static Vec3 vec_unit(Vec3 a){float l=sqrtf(vec_dot(a,a));return l>.0001f?vec_scale(a,1/l):(Vec3){0,1,0};}
void game_car_basis(const Game *g,int cpu,Vec3 *right,Vec3 *up,Vec3 *forward){
    const CarSurface *s=cpu?&g->cpu_surface:&g->car_surface;
    float heading=cpu?g->cpu_heading:g->heading;
    *up=s->up;
    if(vec_dot(*up,*up)<.5f)*up=(Vec3){0,1,0};
    *forward=!s->dynamic&&up->y>.9999f?(Vec3){sinf(heading),0,cosf(heading)}:s->forward;
    *right=vec_unit(vec_cross(*up,*forward));*forward=vec_unit(vec_cross(*right,*up));
}
static int car_edge_plane(int i,Vec3 p,float *d,float *nx,float *nz){
    float x=p.x;
    if(p.y>GOAL_H&&(i==6||i==20||i>=28)&&fabsf(x)<GOAL_W)x=GOAL_W+.01f;
    return edge_plane(i,x,p.z,d,nx,nz);
}
static void free_car_boundary(Vec3 *p,Vec3 *velocity){
    int i;float d,nx,nz;
    for(i=0;i<ARENA_EDGES;i++)if(car_edge_plane(i,*p,&d,&nx,&nz)&&d>-.06f){
        float out=velocity->x*nx+velocity->z*nz;
        p->x-=(d+.06f)*nx;p->z-=(d+.06f)*nz;
        if(out>0){velocity->x-=out*nx;velocity->z-=out*nz;}
    }
    if(fabsf(p->z)>ARENA_Z-1.8f&&fabsf(p->x)<GOAL_W&&p->y<=GOAL_H){p->z=copysignf(ARENA_Z-1.8f,p->z);if(p->z*velocity->z>0)velocity->z*=-.3f;}
}
static int nearest_edge(Vec3 p,float *distance,Vec3 *normal){
    int i,best=-1;float d,nx,nz;*distance=-10000;
    for(i=0;i<ARENA_EDGES;i++)if(car_edge_plane(i,p,&d,&nx,&nz)&&d>*distance){
        best=i;*distance=d;*normal=(Vec3){nx,0,nz};
    }
    return best;
}
/* The CPU uses exactly the same steering, grip, acceleration and collision body. */
/* Full low-speed lock; larger turn radius as speed rises. */
float game_steering_scale(float speed){
    return clamp(TURN_ACCEL_LIMIT/(2.15f*fmaxf(fabsf(speed),1)),.30f,1);
}
/* Shared by the flat driver and near-flat wheel contacts. */
float game_ground_force(float speed,Input in,int burning,float dt){
    float throttle=clamp(in.throttle,-1,1),brake=clamp(in.brake,0,1);
    float cap=burning?BOOST_SPEED:CAR_SPEED,ratio=clamp(fabsf(speed)/cap,0,1),motor;
    if(throttle*speed< -1)brake=fmaxf(brake,fabsf(throttle));
    if(brake>0)return -copysignf(fminf(BRAKE_DECEL*brake+.65f*fabsf(speed),fabsf(speed)/fmaxf(dt,.0001f)),speed);
    motor=throttle*(burning?BOOST_ACCEL:DRIVE_ACCEL)*(1-.12f*ratio*ratio);
    if((speed>cap+.01f&&motor>0)||(speed< -10&&motor<0))motor=0;
    return motor-speed*.65f;
}
static void flat_drive(Vec3 *car,Vec3 *velocity,float *heading,float *boost,int *jump_held,SoundEvents *sounds,float gain,Input in,float dt){
    float fx,fz,forward,side,drag,ground=0;int burning,airborne;
    if(in.jump&&!*jump_held&&car->y<=ground+.001f&&velocity->y<=0){velocity->y=JUMP_SPEED;sounds->jump=fmaxf(sounds->jump,gain);}
    *jump_held=in.jump!=0;
    airborne=car->y>ground+.001f||velocity->y>0;
    fx=sinf((*heading)); fz=cosf((*heading));
    forward=velocity->x*fx+velocity->z*fz;
    (*heading)+=clamp(in.steer,-1,1)*2.15f*(airborne?.30f:game_steering_scale(forward))*clamp(fabsf(forward)/7,0,1)*(forward<-.5f?-1:1)*dt;
    fx=sinf((*heading));fz=cosf((*heading));
    forward=velocity->x*fx+velocity->z*fz;
    side=velocity->x*fz-velocity->z*fx;
    burning=in.boost&&(*boost)>0&&in.throttle>0&&in.brake<=0;
    {float previous=forward;
        if(airborne){
            forward+=clamp(in.throttle,-1,1)*(burning?BOOST_ACCEL:DRIVE_ACCEL)*.25f*dt;
            forward*=fmaxf(0,1-.08f*dt);
        }else forward+=game_ground_force(forward,in,burning,dt)*dt;
        /* Releasing boost carries speed into a gradual coast, not a hard cut. */
        forward=clamp(forward,-10,burning?BOOST_SPEED:fmaxf(CAR_SPEED,previous));
    }
    drag=airborne?.65f:8;
    side*=fmaxf(0,1-drag*dt);
    velocity->x=fx*forward+fz*side;velocity->z=fz*forward-fx*side;
    (*boost)=clamp((*boost)+(burning?-.30f:.12f)*dt,0,1);
    car->x+=velocity->x*dt;car->z+=velocity->z*dt;
    if(airborne){velocity->y-=CAR_GRAVITY*dt;car->y+=velocity->y*dt;}
    if(car->y>ARENA_HEIGHT-1.8f){car->y=ARENA_HEIGHT-1.8f;if(velocity->y>0)velocity->y*=-.05f;}
    ground=0;
    if(!airborne||car->y<ground){
        if(airborne)sounds->land=fmaxf(sounds->land,-velocity->y*gain);
        car->y=ground;velocity->y=0;
    }
}

/* The accepted flat driver remains the fast path away from the perimeter. */
static void drive(Vec3 *p,Vec3 *velocity,float *heading,float *boost,int *jump_held,CarSurface *s,SoundEvents *sounds,float gain,Input in,float dt,int allow_air_roll){
    float distance;Vec3 normal;nearest_edge(*p,&distance,&normal);
    if(!in.jump&&s->air_roll_active<0)s->air_roll_active=0;
    /* Manual roll or a second jump opts into the physical body solver. */
    if(allow_air_roll&&!s->dynamic&&in.jump&&(fabsf(in.steer)>.15f||(s->air_jump_available&&!*jump_held))&&(p->y>.001f||velocity->y>0)){
        s->forward=(Vec3){sinf(*heading),0,cosf(*heading)};s->dynamic=1;
    }
    if(!s->dynamic&&distance< -BANK_W-3){
        s->contact_frame_valid=0;s->wall_arc=0;
        s->cooldown=fmaxf(0,s->cooldown-dt);
        if(allow_air_roll){
            int airborne=p->y>.001f||velocity->y>0;
            if(!airborne)s->air_jump_available=0;
            if(in.jump&&!*jump_held){
                if(!airborne){s->air_jump_available=1;s->cooldown=.22f;}
            }
        }
        flat_drive(p,velocity,heading,boost,jump_held,sounds,gain,in,dt);
        if(fabsf(p->z)>ARENA_Z-1.8f&&fabsf(p->x)<GOAL_W&&p->y<GOAL_H){
            p->z=copysignf(ARENA_Z-1.8f,p->z);if(p->z*velocity->z>0)velocity->z*=-.3f;
        }
        s->up=(Vec3){0,1,0};s->forward=(Vec3){sinf(*heading),0,cosf(*heading)};
        s->attached=p->y<=.001f;s->grounded_wheels=s->attached?4:0;
    }else{
        if(!s->dynamic){s->forward=(Vec3){sinf(*heading),0,cosf(*heading)};s->dynamic=1;}
        vehicle_step(p,velocity,heading,boost,jump_held,s,sounds,gain,in,dt,allow_air_roll);
        if(distance< -BANK_W-3&&s->grounded_wheels==4&&s->up.y>.999f&&fabsf(p->y)<.08f&&vec_dot(s->angular_velocity,s->angular_velocity)<.1f){
            s->dynamic=0;s->up=(Vec3){0,1,0};s->forward=(Vec3){sinf(*heading),0,cosf(*heading)};
            s->angular_velocity=(Vec3){0,0,0};p->y=0;velocity->y=0;
        }
    }
}
static float angle_wrap(float a){
    while(a>3.14159265f)a-=6.2831853f;
    while(a< -3.14159265f)a+=6.2831853f;
    return a;
}
static Input cpu_input(Game *g,float dt){
    Input in={0};
    float bx=g->ball.x+g->ball_velocity.x*.22f;
    float bz=g->ball.z+g->ball_velocity.z*.22f;
    float sx=-bx,sz=-ARENA_Z-bz,length=sqrtf(sx*sx+sz*sz);
    float dx,dz,dist,along,lateral,tx,tz,error,speed,forward;
    if(length<.1f){sx=0;sz=-1;}else{sx/=length;sz/=length;}
    bx=clamp(bx,-ARENA_X+BALL_R,ARENA_X-BALL_R);
    bz=clamp(bz,-ARENA_Z+BALL_R,ARENA_Z-BALL_R);
    dx=g->cpu.x-bx;dz=g->cpu.z-bz;dist=sqrtf(dx*dx+dz*dz);
    along=dx*sx+dz*sz;lateral=fabsf(dx*sz-dz*sx);
    if(along< -1.0f&&lateral<3.0f&&dist<11&&g->ball.y<BALL_R+2.35f)g->cpu_attack=.65f;
    else g->cpu_attack=fmaxf(0,g->cpu_attack-dt);
    tx=bx-sx*CPU_SETUP_DISTANCE;tz=bz-sz*CPU_SETUP_DISTANCE;
    if(g->cpu_attack>0){tx=bx+sx*4;tz=bz+sz*4;}
    else if(along> -1&&dist<12){
        /* Circle around the ball rather than drive it into the orange goal. */
        float side=dx*sz-dz*sx>=0?1.0f:-1.0f;
        tx=bx-sx*5+sz*side*CPU_AROUND_DISTANCE;tz=bz-sz*5-sx*side*CPU_AROUND_DISTANCE;
    }
    tx=clamp(tx,-ARENA_X+3,ARENA_X-3);tz=clamp(tz,-ARENA_Z+3,ARENA_Z-3);
    {Vec3 target={tx,0,tz},zero={0,0,0};car_boundary(&target,&zero);tx=target.x;tz=target.z;}
    error=angle_wrap(atan2f(tx-g->cpu.x,tz-g->cpu.z)-g->cpu_heading);
    speed=sqrtf(vec_dot(g->cpu_velocity,g->cpu_velocity));
    forward=g->cpu_surface.attached?vec_dot(g->cpu_velocity,g->cpu_surface.forward):g->cpu_velocity.x*sinf(g->cpu_heading)+g->cpu_velocity.z*cosf(g->cpu_heading);
    in.steer=clamp(error*1.8f,-1,1);
    in.throttle=fabsf(error)>1.2f?.38f:1;
    if(speed>(fabsf(error)>1.2f?7: (fabsf(error)>.6f?13:23)))in.brake=1;
    in.boost=g->cpu_attack>0&&fabsf(error)<.20f&&dist>5&&g->cpu_boost>.2f;
    if(speed<1.4f)g->cpu_stuck+=dt;else g->cpu_stuck=0;
    if(g->cpu_stuck>1.0f){g->cpu_reverse=.9f;g->cpu_stuck=0;}
    if(g->cpu_reverse>0){
        g->cpu_reverse-=dt;in.throttle=-1;in.brake=forward>1;in.boost=0;in.steer=-in.steer;
    }
    if(g->cpu_reverse<=0&&g->ball.y>BALL_R+1.05f&&g->ball.y<BALL_R+3.65f&&dist<7&&along< -1&&lateral<3){
        float ball_error=angle_wrap(atan2f(g->ball.x-g->cpu.x,g->ball.z-g->cpu.z)-g->cpu_heading);
        if(fabsf(ball_error)<.55f)in.jump=1;
    }
    return in;
}
/* Contact feedback is observational; it never changes collision response. */
static void car_hit_feedback(Game *g,Vec3 normal,float strength){
    Vec3 point=vec_add(g->ball,vec_scale(normal,-BALL_R));int i,slot=0;
    if(strength>g->feedback.car_hit){g->feedback.car_hit=strength;g->feedback.hit_position=point;}
    if(strength<=5)return;
    for(i=0;i<CONTACT_SPARKS;i++){
        if(g->feedback.sparks[i].life<=0){slot=i;break;}
        if(g->feedback.sparks[i].life<g->feedback.sparks[slot].life)slot=i;
    }
    g->feedback.sparks[slot]=(ContactSpark){normal,CONTACT_SPARK_TIME,clamp(strength/BOOST_SPEED,0,1)};
}
static void clear_contact_sparks(Game *g){int i;for(i=0;i<CONTACT_SPARKS;i++)g->feedback.sparks[i].life=0;}
/* Air contacts use the rotated chassis ellipsoid; ground contacts keep their proven shape. */
static void aerial_car_ball(Game *g,Vec3 car,Vec3 *velocity,float heading,CarSurface *surface){
    Vec3 up=surface->up;
    Vec3 center=vec_add(car,vec_scale(up,.85f));
    Vec3 delta={g->ball.x-center.x,g->ball.y-center.y,g->ball.z-center.z};
    float along_up=vec_dot(delta,up),vertical_scale=(BALL_R+1.65f)/(BALL_R+.85f);
    float weight=vertical_scale*vertical_scale;
    float d=sqrtf(vec_dot(delta,delta)+(weight-1)*along_up*along_up),nx,ny,nz,norm,rel;
    Vec3 normal;
    if(d>=BALL_R+1.65f)return;
    if(d<.001f){delta=(Vec3){sinf(heading)*.001f,0,cosf(heading)*.001f};d=.001f;along_up=vec_dot(delta,up);}
    g->ball=vec_add(center,vec_scale(delta,(BALL_R+1.65f)/d));
    g->ball.y=fmaxf(BALL_R,g->ball.y);
    normal=vec_add(delta,vec_scale(up,(weight-1)*along_up));
    norm=sqrtf(vec_dot(normal,normal));nx=normal.x/norm;ny=normal.y/norm;nz=normal.z/norm;
    rel=(velocity->x-g->ball_velocity.x)*nx+(velocity->y-g->ball_velocity.y)*ny+(velocity->z-g->ball_velocity.z)*nz;
    /* A rotating flip can swat a ball even with little center-of-mass motion.
       Use the chassis contact arm, not the ball-center lever arm. */
    if(surface->flip_time>0&&surface->flip_hit_available){
        Vec3 arm=vec_scale(delta,1.65f/fmaxf(sqrtf(vec_dot(delta,delta)),.001f));
        Vec3 omega=surface->angular_velocity;
        Vec3 contact={omega.y*arm.z-omega.z*arm.y,omega.z*arm.x-omega.x*arm.z,omega.x*arm.y-omega.y*arm.x};
        rel+=fmaxf(0,contact.x*nx+contact.y*ny+contact.z*nz);
    }
    if(rel>0){
        float hit_gain=BALL_HIT_GAIN;
        if(surface->flip_time>0&&surface->flip_hit_available){hit_gain*=1.15f;surface->flip_hit_available=0;}
        g->sounds.hit=fmaxf(g->sounds.hit,rel);
        car_hit_feedback(g,(Vec3){nx,ny,nz},rel);
        g->ball_velocity.x+=nx*rel*hit_gain;g->ball_velocity.y+=ny*rel*hit_gain;g->ball_velocity.z+=nz*rel*hit_gain;
        if(velocity->y>0)g->ball_velocity.y=fmaxf(g->ball_velocity.y,fminf(13*GAME_PACE,(velocity->y*.7f+rel*.2f)*GAME_PACE));
        velocity->x-=nx*rel*.12f;velocity->z-=nz*rel*.12f;
    }
}
static void car_ball(Game *g,Vec3 car,Vec3 *velocity,float heading,CarSurface *surface){
    float nx=g->ball.x-car.x,nz=g->ball.z-car.z,distance=sqrtf(nx*nx+nz*nz),rel;
    if(car.y>.001f||surface->up.y<.999f){aerial_car_ball(g,car,velocity,heading,surface);return;}
    if(distance>=BALL_R+1.65f||g->ball.y>=BALL_R+1.35f)return;
    if(distance<.001f){nx=sinf(heading);nz=cosf(heading);}else{nx/=distance;nz/=distance;}
    g->ball.x=car.x+nx*(BALL_R+1.65f);g->ball.z=car.z+nz*(BALL_R+1.65f);
    rel=(velocity->x-g->ball_velocity.x)*nx+(velocity->z-g->ball_velocity.z)*nz;
    if(rel>0){
        g->sounds.hit=fmaxf(g->sounds.hit,rel);
        car_hit_feedback(g,(Vec3){nx,0,nz},rel);
        /* Continuous bumper aiming: soften frontal offset sensitivity while
           keeping side/rear hits physical. Smoothly fades out at 70 degrees.
           Only the shot direction changes; impulse strength/recoil stay intact. */
        {float fx=sinf(heading),fz=cosf(heading),front=nx*fx+nz*fz;
            float blend=clamp((front-.35f)/.65f,0,1),ax,az,length;
            blend=.35f*blend*blend*(3-2*blend);
            ax=nx*(1-blend)+fx*blend;az=nz*(1-blend)+fz*blend;length=hypotf(ax,az);
            g->ball_velocity.x+=ax/length*rel*BALL_HIT_GAIN;g->ball_velocity.z+=az/length*rel*BALL_HIT_GAIN;
        }
        g->ball_velocity.y=fmaxf(g->ball_velocity.y,fminf(10*GAME_PACE,rel*.28f*GAME_PACE));
        velocity->x-=nx*rel*.12f;velocity->z-=nz*rel*.12f;
    }
}
static void cars_collide(Game *g){
    if(g->car_surface.up.y<.999f||g->cpu_surface.up.y<.999f){
        Vec3 a=vec_add(g->car,vec_scale(g->car_surface.up,.85f));
        Vec3 b=vec_add(g->cpu,vec_scale(g->cpu_surface.up,.85f));
        Vec3 delta={b.x-a.x,b.y-a.y,b.z-a.z};float d=sqrtf(vec_dot(delta,delta)),rel;
        if(d>=3.3f)return;
        delta=d>.001f?vec_scale(delta,1/d):(Vec3){0,1,0};
        g->car=vec_add(g->car,vec_scale(delta,-(3.3f-d)*.5f));
        g->cpu=vec_add(g->cpu,vec_scale(delta,(3.3f-d)*.5f));
        rel=vec_dot(g->velocity,delta)-vec_dot(g->cpu_velocity,delta);
        if(rel>0){g->velocity=vec_add(g->velocity,vec_scale(delta,-rel*.65f));g->cpu_velocity=vec_add(g->cpu_velocity,vec_scale(delta,rel*.65f));}
        vehicle_resolve(&g->car,&g->velocity,&g->car_surface);
        vehicle_resolve(&g->cpu,&g->cpu_velocity,&g->cpu_surface);
        g->car.y=fmaxf(0,g->car.y);g->cpu.y=fmaxf(0,g->cpu.y);return;
    }

    float dx=g->cpu.x-g->car.x,dz=g->cpu.z-g->car.z,d=sqrtf(dx*dx+dz*dz),n,rel;
    if(d>=3.3f||fabsf(g->cpu.y-g->car.y)>1.7f)return;
    if(d<.001f){dx=1;dz=0;}else{dx/=d;dz/=d;}
    n=(3.3f-d)*.5f;
    g->cpu.x+=dx*n;g->cpu.z+=dz*n;g->car.x-=dx*n;g->car.z-=dz*n;
    rel=(g->velocity.x-g->cpu_velocity.x)*dx+(g->velocity.z-g->cpu_velocity.z)*dz;
    if(rel>0){
        float impulse=rel*.65f;
        g->velocity.x-=dx*impulse;g->velocity.z-=dz*impulse;
        g->cpu_velocity.x+=dx*impulse;g->cpu_velocity.z+=dz*impulse;
    }
    if(g->car_surface.dynamic)vehicle_resolve(&g->car,&g->velocity,&g->car_surface);
    else free_car_boundary(&g->car,&g->velocity);
    if(g->cpu_surface.dynamic)vehicle_resolve(&g->cpu,&g->cpu_velocity,&g->cpu_surface);
    else free_car_boundary(&g->cpu,&g->cpu_velocity);
}

void game_clear_events(Game *g){memset(&g->sounds,0,sizeof(g->sounds));g->feedback.car_hit=0;}
void game_reset(Game *g) {
    game_clear_events(g);memset(&g->feedback,0,sizeof(g->feedback));
    g->car=(Vec3){0,0,-SPAWN_Z}; g->velocity=(Vec3){0,0,0};
    g->ball=(Vec3){0,BALL_R,0}; g->ball_velocity=(Vec3){0,0,0};
    g->cpu=(Vec3){0,0,SPAWN_Z};g->cpu_velocity=(Vec3){0,0,0};
    g->cpu_heading=3.14159265f;g->cpu_boost=1;g->cpu_stuck=g->cpu_reverse=g->cpu_attack=0;
    g->jump_held=g->cpu_jump_held=0;
    memset(&g->car_surface,0,sizeof(g->car_surface));memset(&g->cpu_surface,0,sizeof(g->cpu_surface));
    g->car_surface.up=g->cpu_surface.up=(Vec3){0,1,0};
    g->car_surface.forward=(Vec3){0,0,1};g->cpu_surface.forward=(Vec3){0,0,-1};
    memset(&g->car_visual,0,sizeof(g->car_visual));memset(&g->cpu_visual,0,sizeof(g->cpu_visual));
    memset(&g->car_suspension,0,sizeof(g->car_suspension));memset(&g->cpu_suspension,0,sizeof(g->cpu_suspension));
    memset(&g->exhaust,0,sizeof(g->exhaust));memset(&g->cpu_exhaust,0,sizeof(g->cpu_exhaust));
    g->heading=0; g->boost=1; g->reset_timer=0;
}
void game_init(Game *g){memset(g,0,sizeof(*g));g->cpu_enabled=1;game_reset(g);}
static void limit_ball_speed(Game *g){
    float speed=sqrtf(vec_dot(g->ball_velocity,g->ball_velocity));
    if(speed>BALL_SPEED)g->ball_velocity=vec_scale(g->ball_velocity,BALL_SPEED/speed);
}
/* Cosmetic state follows fixed simulation ticks: independent of draw count,
   pause and camera changes. It never feeds back into vehicle forces. */
static void wheel_visual(Game *g,int cpu,Input in,float dt){
    CarVisual *visual=cpu?&g->cpu_visual:&g->car_visual;
    const CarSurface *surface=cpu?&g->cpu_surface:&g->car_surface;
    Vec3 velocity=cpu?g->cpu_velocity:g->velocity;
    float speed=vec_dot(velocity,surface->forward),blend,target;
    /* The driver already maintains this basis; reuse it without more trig. */
    blend=clamp((surface->up.y-.85f)/.15f,0,1);
    target=clamp(in.steer,-1,1)*.28f*(1+blend*(game_steering_scale(speed)-1));
    visual->steer+=(target-visual->steer)*fminf(1,dt*12);
    visual->roll=fmodf(visual->roll+speed*dt/.50f,6.2831853072f);
}
static void visual_spring(float *value,float *rate,float target,float stiffness,float damping,float lo,float hi,float dt){
    *rate+=(stiffness*(target-*value)-damping**rate)*dt;
    *value+=*rate*dt;
    if(*value<lo){*value=lo;if(*rate<0)*rate=0;}
    if(*value>hi){*value=hi;if(*rate>0)*rate=0;}
}
/* Small mesh-only deflections: never change the wheel/contact body or forces. */
static void suspension_visual(Game *g,int cpu,Vec3 previous,float dt){
    CarSuspension *visual=cpu?&g->cpu_suspension:&g->car_suspension;
    const CarSurface *surface=cpu?&g->cpu_surface:&g->car_surface;
    Vec3 velocity=cpu?g->cpu_velocity:g->velocity;
    Vec3 right=vec_cross(surface->up,surface->forward),acceleration;
    float weight=clamp((surface->up.y-.94f)/.06f,0,1),lateral=0,along=0,heave=0;
    int grounded=surface->grounded_wheels>0&&surface->up.y>.94f;
    if(dt<=0)return;
    acceleration=vec_scale((Vec3){velocity.x-previous.x,velocity.y-previous.y,velocity.z-previous.z},1/dt);
    if(grounded){
        lateral=clamp(vec_dot(acceleration,right),-50,50);
        along=clamp(vec_dot(acceleration,surface->forward),-90,60);
        heave=-fminf(.015f,(fabsf(lateral)+.3f*fabsf(along))*.00025f)*weight;
        if(!visual->grounded){
            float impact=-vec_dot(previous,surface->up);
            if(impact>1)visual->heave_rate-=fminf(1.1f,impact*.12f)*weight;
        }
    }
    visual->grounded=grounded;
    visual_spring(&visual->bank,&visual->bank_rate,clamp(lateral*.00065f,-.03f,.03f)*weight,100,18,-.03f,.03f,dt);
    visual_spring(&visual->pitch,&visual->pitch_rate,clamp(-along*.00035f,-.02f,.02f)*weight,110,18,-.02f,.02f,dt);
    visual_spring(&visual->heave,&visual->heave_rate,heave,380,20,-.04f,.025f,dt);
}
/* Read-only observer of boost input/fuel; never supplies a vehicle force. */
static void exhaust_visual(Game *g,int cpu,Input in,float fuel_before,float dt){
    BoostExhaust *e=cpu?&g->cpu_exhaust:&g->exhaust;
    float fuel=cpu?g->cpu_boost:g->boost;Vec3 right,up,forward;
    Vec3 velocity=cpu?g->cpu_velocity:g->velocity;
    if(!in.boost||in.throttle<=0||in.brake>0||fuel_before<=0||fuel<=0){e->strength=0;e->phase=0;return;}
    game_car_basis(g,cpu,&right,&up,&forward);
    e->strength=clamp(in.throttle,0,1)*(.65f+.35f*clamp(fabsf(vec_dot(velocity,forward))/BOOST_SPEED,0,1));
    e->phase=fmodf(e->phase+dt*4,1);
}
static void game_step_internal(Game *g,Input in,const Input *second,float dt){
    float fuel_before=g->boost,cpu_fuel_before=g->cpu_boost;
    float drag;int spark;Input ai={0};Vec3 previous=g->velocity,cpu_previous=g->cpu_velocity;
    for(spark=0;spark<CONTACT_SPARKS;spark++)g->feedback.sparks[spark].life=fmaxf(0,g->feedback.sparks[spark].life-dt);
    g->elapsed+=dt;
    if(g->reset_timer>0){g->exhaust=(BoostExhaust){0};g->cpu_exhaust=(BoostExhaust){0};clear_contact_sparks(g);g->reset_timer-=dt;if(g->reset_timer<=0)game_reset(g);return;}
    drive(&g->car,&g->velocity,&g->heading,&g->boost,&g->jump_held,&g->car_surface,&g->sounds,1,in,dt,1);
    if(g->cpu_enabled){ai=second?*second:cpu_input(g,dt);drive(&g->cpu,&g->cpu_velocity,&g->cpu_heading,&g->cpu_boost,&g->cpu_jump_held,&g->cpu_surface,&g->sounds,second?1.0f:.45f,ai,dt,second!=0);cars_collide(g);}
    exhaust_visual(g,0,in,fuel_before,dt);
    if(g->cpu_enabled)exhaust_visual(g,1,ai,cpu_fuel_before,dt);
    else g->cpu_exhaust=(BoostExhaust){0};
    suspension_visual(g,0,previous,dt);
    if(g->cpu_enabled)suspension_visual(g,1,cpu_previous,dt);
    wheel_visual(g,0,in,dt);
    if(g->cpu_enabled)wheel_visual(g,1,ai,dt);
    g->ball_velocity.y-=BALL_GRAVITY*dt;
    limit_ball_speed(g);
    g->ball.x+=g->ball_velocity.x*dt;g->ball.y+=g->ball_velocity.y*dt;g->ball.z+=g->ball_velocity.z*dt;
    if(g->ball.y>ARENA_HEIGHT-BALL_R){
        g->ball.y=ARENA_HEIGHT-BALL_R;
        if(g->ball_velocity.y>0){g->sounds.hit=fmaxf(g->sounds.hit,g->ball_velocity.y*.55f);g->ball_velocity.y*=-.64f;}
    }
    if(g->ball.y<BALL_R){if(g->ball_velocity.y< -2)g->sounds.hit=fmaxf(g->sounds.hit,-g->ball_velocity.y*.55f);g->ball.y=BALL_R;g->ball_velocity.y=fabsf(g->ball_velocity.y)>.9f?fabsf(g->ball_velocity.y)*.64f:0;}
    drag=g->ball.y<=BALL_R+.03f?.38f:.09f;
    g->ball_velocity.x*=1-drag*dt;g->ball_velocity.z*=1-drag*dt;
    ball_boundary(g);
    if(fabsf(g->ball.z)>ARENA_Z-BALL_R){
        int opening=fabsf(g->ball.x)+BALL_R<GOAL_W && g->ball.y+BALL_R<GOAL_H;
        if(!opening){g->sounds.hit=fmaxf(g->sounds.hit,fabsf(g->ball_velocity.z)*.65f);g->ball.z=copysignf(ARENA_Z-BALL_R,g->ball.z);g->ball_velocity.z*=-.8f;}
        else if(fabsf(g->ball.z)>ARENA_Z+BALL_R){
            g->sounds.goal=1;g->exhaust=(BoostExhaust){0};g->cpu_exhaust=(BoostExhaust){0};clear_contact_sparks(g);
            g->last_goal=g->ball.z>0?1:-1;
            if(g->last_goal>0)g->blue_score++;else g->orange_score++;
            g->reset_timer=2;return;
        }
    }
    car_ball(g,g->car,&g->velocity,g->heading,&g->car_surface);
    if(g->cpu_enabled)car_ball(g,g->cpu,&g->cpu_velocity,g->cpu_heading,&g->cpu_surface);
    /* Recheck banks after car hits push the ball toward the perimeter. */
    ball_boundary(g);
    limit_ball_speed(g);
}

void game_step(Game *g,Input in,float dt){game_step_internal(g,in,0,dt);}
void game_step_two(Game *g,Input blue,Input orange,float dt){game_step_internal(g,blue,&orange,dt);}
