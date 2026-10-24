#include "vehicle.h"
#include <math.h>
#include <string.h>
/* Original lightweight contact solver. No Unity/PhysX or imported source. */
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 add(Vec3 a,Vec3 b){return (Vec3){a.x+b.x,a.y+b.y,a.z+b.z};}
static Vec3 mul(Vec3 a,float k){return (Vec3){a.x*k,a.y*k,a.z*k};}
static Vec3 sub(Vec3 a,Vec3 b){return add(a,mul(b,-1));}
static Vec3 cross(Vec3 a,Vec3 b){return (Vec3){a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static float clamp(float x,float lo,float hi){return fmaxf(lo,fminf(hi,x));}
static Vec3 unit(Vec3 a){float l=sqrtf(dot(a,a));return l>.00001f?mul(a,1/l):(Vec3){0,1,0};}
static Vec3 rotate_about(Vec3 vector,Vec3 axis,float angle){
    float c=cosf(angle),si=sinf(angle);
    return add(add(mul(vector,c),mul(cross(axis,vector),si)),mul(axis,dot(axis,vector)*(1-c)));
}
/* Minimal rotation transports heading through a bend without adding a steering twist. */
static Vec3 transport(Vec3 vector,Vec3 from,Vec3 to){
    Vec3 axis=cross(from,to);float c=dot(from,to);
    if(c<-.95f)return unit(sub(vector,mul(to,dot(vector,to))));
    return add(add(vector,cross(axis,vector)),mul(cross(axis,cross(axis,vector)),1/(1+c)));
}
typedef struct {Vec3 a,n,t;float length,bottom;} Edge;
#define CONTACT_EDGES (ARENA_EDGES+2)
typedef struct {float d,h,dx,dy,length;Vec3 n;} Section;
static Edge edges[CONTACT_EDGES];static Section sections[BANK_SEGMENTS];static int ready;
static void initialize(void){
    int i;if(ready)return;
    for(i=0;i<CONTACT_EDGES;i++){
        Vec3 b;
        if(i<ARENA_EDGES){arena_edge(i,&edges[i].a,&b);edges[i].bottom=BANK_H;}
        else{
            int side=i==ARENA_EDGES?1:-1;
            edges[i].a=(Vec3){side*GOAL_W,0,side*ARENA_Z};b=(Vec3){-side*GOAL_W,0,side*ARENA_Z};
            edges[i].bottom=GOAL_H+.25f;
        }edges[i].t=unit(sub(b,edges[i].a));
        edges[i].n=(Vec3){edges[i].t.z,0,-edges[i].t.x};edges[i].length=sqrtf(dot(sub(b,edges[i].a),sub(b,edges[i].a)));
    }
    for(i=0;i<BANK_SEGMENTS;i++){
        float b,h;Section *s=&sections[i];arena_bank_profile(i,&s->d,&s->h);arena_bank_profile(i+1,&b,&h);
        s->dx=b-s->d;s->dy=h-s->h;s->length=hypotf(s->dx,s->dy);s->n=(Vec3){-s->dy/s->length,s->dx/s->length,0};
    }
    ready=1;
}
static int edge_contains(const Edge *e,Vec3 point){
    float along=dot(sub(point,e->a),e->t);
    return along>=-.12f&&along<=e->length+.12f;
}
static void ray_plane(Vec3 o,Vec3 dir,float limit,Vec3 point,Vec3 normal,WheelContact *hit){
    float denom=dot(dir,normal),distance;
    if(denom>=-.00001f)return;
    distance=dot(sub(point,o),normal)/denom;
    if(distance<0||distance>limit||distance>=hit->distance)return;
    hit->distance=distance;hit->point=add(o,mul(dir,distance));hit->normal=normal;hit->hit=1;
}
int vehicle_probe(Vec3 origin,Vec3 direction,float reach,WheelContact *hit){
    int i,j;initialize();memset(hit,0,sizeof(*hit));hit->distance=reach+.001f;
    if(direction.y<-.00001f){
        WheelContact floor_hit={0};floor_hit.distance=hit->distance;
        ray_plane(origin,direction,reach,(Vec3){0,0,0},(Vec3){0,1,0},&floor_hit);
        if(floor_hit.hit&&arena_floor(floor_hit.point.x,floor_hit.point.z)<.015f)*hit=floor_hit;
    }
    for(i=0;i<CONTACT_EDGES;i++){
        const Edge *e=&edges[i];float d=dot(sub(origin,e->a),e->n),along=dot(sub(origin,e->a),e->t);
        if(d< -BANK_W-reach||d>reach||along< -reach||along>e->length+reach)continue;
        for(j=0;j<BANK_SEGMENTS&&i<ARENA_EDGES;j++){
            Section *s=&sections[j];WheelContact candidate={0};float hd;
            Vec3 n={e->n.x*s->n.x,s->n.y,e->n.z*s->n.x};
            Vec3 point=add(e->a,mul(e->n,s->d));point.y=s->h;candidate.distance=hit->distance;
            ray_plane(origin,direction,reach,point,n,&candidate);
            if(!candidate.hit||!edge_contains(e,candidate.point))continue;
            hd=dot(sub(candidate.point,e->a),e->n);
            if(hd>=s->d-.015f&&hd<=s->d+s->dx+.015f)*hit=candidate;
        }
        {WheelContact candidate={0};Vec3 p=e->a;p.y=BANK_H;candidate.distance=hit->distance;
            ray_plane(origin,direction,reach,p,mul(e->n,-1),&candidate);
            if(candidate.hit&&candidate.point.y>=e->bottom-.02f&&candidate.point.y<=ARENA_HEIGHT&&edge_contains(e,candidate.point))*hit=candidate;
        }
    }
    return hit->hit;
}
static Vec3 local_point(Vec3 p,const CarSurface *s,Vec3 local){
    Vec3 right=cross(s->up,s->forward);
    return add(p,add(add(mul(right,local.x),mul(s->up,local.y)),mul(s->forward,local.z)));
}
static Vec3 point_velocity(Vec3 velocity,const CarSurface *s,Vec3 arm){return add(velocity,cross(s->angular_velocity,arm));}
static void force_at(Vec3 force,Vec3 arm,Vec3 *accel,Vec3 *torque){*accel=add(*accel,force);*torque=add(*torque,cross(arm,force));}
static Vec3 inertia_inverse(const CarSurface *s,Vec3 torque){
    Vec3 r=cross(s->up,s->forward);
    return add(add(mul(r,dot(torque,r)/1.6f),mul(s->up,dot(torque,s->up)/1.9f)),mul(s->forward,dot(torque,s->forward)/.85f));
}
/* Body collision spheres stop the chassis from passing through a surface after contact is lost. */
static void sphere_contacts(Vec3 point,float radius,Vec3 *normal,float *penetration){
    int i,j;float best=radius-point.y;*normal=(Vec3){0,1,0};*penetration=fmaxf(0,best);
    if(point.y+radius-ARENA_HEIGHT>*penetration){*penetration=point.y+radius-ARENA_HEIGHT;*normal=(Vec3){0,-1,0};}
    for(i=0;i<CONTACT_EDGES;i++){
        Edge *e=&edges[i];float d=dot(sub(point,e->a),e->n);
        if(d< -BANK_W-radius||d>radius+2||!edge_contains(e,point))continue;
        for(j=0;j<BANK_SEGMENTS&&i<ARENA_EDGES;j++){
            Section *s=&sections[j];float along=((d-s->d)*s->dx+(point.y-s->h)*s->dy)/s->length;
            float separation=(d-s->d)*s->n.x+(point.y-s->h)*s->n.y;
            if(along>=-.05f&&along<=s->length+.05f&&radius-separation>*penetration&&point.y<BANK_H+radius){
                *penetration=radius-separation;*normal=(Vec3){e->n.x*s->n.x,s->n.y,e->n.z*s->n.x};
            }
        }
        if(point.y+radius>=e->bottom&&radius+d>*penetration){*penetration=radius+d;*normal=mul(e->n,-1);}
    }
}
void vehicle_resolve(Vec3 *p,Vec3 *velocity,CarSurface *s){
    int pass,i;initialize();
    for(pass=0;pass<2;pass++)for(i=0;i<8;i++){
        Vec3 center=add(*p,mul(s->up,.85f));
        float radius=i<4?.62f:.5f;
        Vec3 local=i<4?(Vec3){i&1?.72f:-.72f,.88f,i&2?1.10f:-1.10f}:(Vec3){i&1?1.10f:-1.10f,.52f,i&2?1.08f:-1.08f};
        Vec3 point=local_point(*p,s,local),n;float penetration;
        sphere_contacts(point,radius,&n,&penetration);
        if(penetration>0){
            Vec3 arm=sub(point,center),v=point_velocity(*velocity,s,arm);float vn=dot(v,n);
            *p=add(*p,mul(n,fminf(penetration,.8f)));
            if(vn<0){
                Vec3 spin=inertia_inverse(s,cross(arm,n));float effective=1+dot(n,cross(spin,arm));
                Vec3 impulse=mul(n,-vn*1.05f/effective);
                *velocity=add(*velocity,impulse);s->angular_velocity=add(s->angular_velocity,inertia_inverse(s,cross(arm,impulse)));
                {Vec3 tangent=sub(v,mul(n,vn));float speed=sqrtf(dot(tangent,tangent));
                    /* Tire grip/rolling resistance is solved at the probes, once. */
                    if(i<4&&speed>.001f){Vec3 friction=mul(tangent,-fminf(speed/effective,sqrtf(dot(impulse,impulse))*.45f)/speed);
                        *velocity=add(*velocity,friction);s->angular_velocity=add(s->angular_velocity,inertia_inverse(s,cross(arm,friction)));}
                }
            }
        }
    }
    /* Bound high-speed collision impulses before they feed the next probe solve.
       Normal driving keeps its existing pre-integration spin limit of 8. */
    {float spin=sqrtf(dot(s->angular_velocity,s->angular_velocity));if(spin>16)s->angular_velocity=mul(s->angular_velocity,16/spin);}
    {float speed=sqrtf(dot(*velocity,*velocity));if(speed>BOOST_SPEED)*velocity=mul(*velocity,BOOST_SPEED/speed);}
    /* Convex root backstop catches residual corner/seam penetration after the
       finite sphere passes. It does not replace the normal wheel/body contacts. */
    for(i=0;i<ARENA_EDGES;i++){
        float d=dot(sub(*p,edges[i].a),edges[i].n);
        if(d>-.06f){
            float outward=dot(*velocity,edges[i].n);
            *p=add(*p,mul(edges[i].n,-d-.06f));
            if(outward>0)*velocity=add(*velocity,mul(edges[i].n,-1.05f*outward));
        }
    }
    /* Retain the prototype's closed car goal boundary, independently of ball scoring. */
    if(fabsf(p->z)>ARENA_Z-.06f&&fabsf(p->x)<GOAL_W){
        p->z=copysignf(ARENA_Z-.06f,p->z);if(p->z*velocity->z>0)velocity->z*=-.3f;
    }
}
/* Lower banks/walls retain full grip. High contact loses driving traction;
   collision springs still keep the chassis outside solid geometry. */
static float surface_grip(float height,Vec3 normal){
    float t,steep;
    if(normal.y<-.5f)return 0; /* No driven/adhesive ceiling contact. */
    if(height<=ARENA_HEIGHT*.40f||normal.y>=.85f)return 1;
    t=clamp((height/ARENA_HEIGHT-.40f)/.38f,0,1);
    steep=clamp((.85f-normal.y)/.50f,0,1);
    return 1-steep*t*t*(3-2*t);
}
void vehicle_step(Vec3 *p,Vec3 *velocity,float *heading,float *boost,int *jump_held,CarSurface *s,SoundEvents *sounds,float gain,Input in,float dt,int allow_air_roll){
    Vec3 center=add(*p,mul(s->up,.85f)),accel={0,-CAR_GRAVITY,0},torque={0,0,0};
    Vec3 right=unit(cross(s->up,s->forward)),average={0,0,0};
    float support_sum=0,support=1;
    int i,count=0,burning=in.boost&&*boost>0&&in.throttle>0,was_grounded=s->grounded_wheels;
    initialize();s->cooldown=fmaxf(0,s->cooldown-dt);
    /* Keep uphill drive off until the car reaches the lower bank, not mid-wall. */
    if(center.y<BANK_H+1)s->upper_wall_sliding=0;
    for(i=0;i<4;i++){
        Vec3 origin=local_point(*p,s,(Vec3){i&1?1.10f:-1.10f,.77f,i&2?1.08f:-1.08f});
        WheelContact *hit=&s->wheels[i];vehicle_probe(origin,mul(s->up,-1),1.12f,hit);
        if(hit->hit&&s->cooldown<=0){
            Vec3 arm=sub(hit->point,center),v=point_velocity(*velocity,s,arm),n=hit->normal;
            float contact_grip=surface_grip(center.y,n);
            float compression=.82f-hit->distance;
            if(n.y<.7f&&(center.y>ARENA_HEIGHT*.70f||(center.y>ARENA_HEIGHT*.45f&&velocity->y<=.25f)))s->upper_wall_sliding=1;
            if(s->upper_wall_sliding&&n.y<.7f)contact_grip=fminf(contact_grip,.10f);
            float spring=clamp(compression*160-dot(v,n)*8,0,100);
            Vec3 f=s->forward,lateral;float speed,side,motor,alignment;
            force_at(mul(n,spring),arm,&accel,&torque);
            if(i&2){float blend=clamp((n.y-.85f)/.15f,0,1);
                float a=in.steer*.28f*(1+blend*(game_steering_scale(dot(*velocity,s->forward))-1));f=add(mul(f,cosf(a)),mul(right,sinf(a)));}
            f=sub(f,mul(n,dot(f,n)));if(dot(f,f)<.03f)continue;f=unit(f);lateral=unit(cross(n,f));
            speed=dot(v,f);side=dot(v,lateral);
            motor=clamp(in.throttle,-1,1)*(burning?BOOST_ACCEL:DRIVE_ACCEL)*.25f;
            if(speed>(burning?BOOST_SPEED:CAR_SPEED)&&motor>0)motor=0;
            if(speed< -10&&motor<0)motor=0;
            if(s->upper_wall_sliding&&f.y*motor>0)motor=0;
            alignment=fmaxf(0,dot(n,s->up));alignment*=alignment;
            {float blend=clamp((n.y-.85f)/.15f,0,1);
                float original=motor-speed*(in.brake>0?7:.65f)*.25f;
                float floor_force=game_ground_force(speed,in,burning&&in.brake<=0,dt)*.25f;
                force_at(mul(f,(original+blend*(floor_force-original))*alignment*contact_grip),arm,&accel,&torque);
            }
            force_at(mul(lateral,clamp(-side*(.7f+1.3f*n.y*n.y),-22,22)*alignment*contact_grip),arm,&accel,&torque);
            average=add(average,n);support_sum+=n.y<-.5f?0:1;count++;
        }
    }
    s->grounded_wheels=count;s->attached=count>0;
    if(count)support=support_sum/count;
    if(!in.jump&&s->air_roll_active<0)s->air_roll_active=0;
    /* Roof/side resting on the flat floor: B gives world-up clearance, not an
       impulse along the inverted chassis up vector. Edge-triggered, no reset. */
    if(allow_air_roll&&!count&&s->flip_time<=0&&s->recovery_time<=0&&in.jump&&!*jump_held&&s->up.y<.35f&&dot(*velocity,*velocity)<225){
        float ground=arena_floor(p->x,p->z),lowest=1000;
        for(i=0;i<4;i++){
            Vec3 body=local_point(*p,s,(Vec3){i&1?.72f:-.72f,.88f,i&2?1.10f:-1.10f});
            lowest=fminf(lowest,body.y-.62f);
        }
        if(ground<.05f&&lowest<=ground+.14f&&center.y-ground<2.1f){
            float direction=dot(cross(s->up,(Vec3){0,1,0}),s->forward);
            s->recovery_direction=fabsf(in.steer)>.15f?(in.steer>0?-1:1):(direction<0?-1:1);
            s->recovery_time=1.3f;s->air_jump_available=0;s->air_roll_active=-1;s->cooldown=.30f;
            velocity->y=fmaxf(velocity->y,6.5f);sounds->jump=fmaxf(sounds->jump,gain);
            s->contact_frame_valid=0;s->wall_arc=0;
        }
    }
    if(count){
        s->flip_remaining=0;s->flip_time=0;s->flip_hit_available=0;
        if(allow_air_roll)s->air_jump_available=0;
        /* Retain wheel-side hold and orientation support while tire traction slips.
           Low driving grip must not turn a wall slide into a free fall. */
        accel=add(accel,mul(s->up,-10*support));
        torque=add(torque,mul(s->angular_velocity,-10*support));
        if(count>=2){
            Vec3 normal=unit(average),actual,target,plane_velocity;
            float forward_speed=dot(*velocity,s->forward),yaw_error;
            float steering_rate=clamp(in.steer,-1,1)*2.15f*clamp(fabsf(forward_speed)/7,0,1)*(forward_speed<-.5f?-1:1);
            int manual=fabsf(in.steer)>=.1f&&normal.y<.7f;
            steering_rate*=1+clamp((normal.y-.85f)/.15f,0,1)*(game_steering_scale(forward_speed)-1);
            if(!s->contact_frame_valid){
                s->contact_up=s->up;s->contact_forward=s->forward;s->contact_frame_valid=1;
            }
            target=transport(s->contact_forward,s->contact_up,normal);
            target=unit(sub(target,mul(normal,dot(target,normal))));
            /* A crest can leave mostly sideways motion: longitudinal speed
               alone would stall steering or reverse it during a counterturn.
               Wall steering uses tangent speed; explicit reverse keeps its
               reverse steering sense. The body still turns through torque. */
            if(manual){
                Vec3 tangent=sub(*velocity,mul(normal,dot(*velocity,normal)));
                steering_rate=clamp(in.steer,-1,1)*2.15f*clamp(sqrtf(dot(tangent,tangent))/7,0,1);
                if(in.throttle<0&&forward_speed<-.5f)steering_rate=-steering_rate;
                target=unit(sub(s->forward,mul(normal,dot(s->forward,normal))));
            }
            target=rotate_about(target,normal,steering_rate*dt);
            plane_velocity=sub(*velocity,mul(normal,dot(*velocity,normal)));
            /* On an angled wall run, follow a continuous travel arc through its crest.
               Head-on stops and reverse input do not trigger a forced 180-degree turn. */
            {Vec3 uphill=sub((Vec3){0,1,0},mul(normal,normal.y));
                Vec3 sideways=cross(normal,unit(uphill));
                float lateral=dot(plane_velocity,sideways),speed=sqrtf(dot(plane_velocity,plane_velocity));
                if(normal.y>.85f||in.throttle<0||in.brake>.1f)s->wall_arc=0;
                if(normal.y<.7f&&fabsf(lateral)>2.5f&&fabsf(dot(target,sideways))>.2f&&in.throttle>=0&&in.brake<.1f)s->wall_arc=1;
                if(normal.y<.7f&&s->wall_arc&&speed>2&&in.throttle>=0&&in.brake<.1f&&fabsf(in.steer)<.1f){
                    Vec3 travel=mul(plane_velocity,1/speed);
                    float error=atan2f(dot(normal,cross(target,travel)),dot(target,travel));
                    float weight=clamp((.7f-normal.y)/.7f,0,1);
                    target=rotate_about(target,normal,clamp(error*4*dt,-3.5f*dt,3.5f*dt)*weight);
                }
            }
            s->contact_up=normal;s->contact_forward=unit(target);
            actual=unit(sub(s->forward,mul(normal,dot(s->forward,normal))));
            yaw_error=atan2f(dot(normal,cross(actual,target)),dot(actual,target));
            torque=add(torque,mul(cross(s->up,normal),80*support));
            torque=add(torque,mul(normal,(yaw_error*65-dot(s->angular_velocity,normal)*4)*support));
            if(manual){
                float yaw_rate=dot(s->angular_velocity,normal);
                /* Player yaw replaces tire/assist yaw, retaining pitch/roll forces.
                   Apply a bounded rate motor; orientation still integrates normally. */
                float command=(steering_rate-yaw_rate)*25+steering_rate*3.04f;
                torque=add(torque,mul(normal,(clamp(command,-65,65)-dot(torque,normal))*support));
            }
        }
        if(!was_grounded&&dot(*velocity,unit(average))< -2)sounds->land=fmaxf(sounds->land,-dot(*velocity,unit(average))*gain);
        if(in.jump&&!*jump_held){
            *velocity=add(*velocity,mul(s->up,JUMP_SPEED));if(allow_air_roll)s->air_jump_available=1;s->cooldown=.22f;s->attached=0;s->grounded_wheels=0;
            s->contact_frame_valid=0;s->wall_arc=0;
            sounds->jump=fmaxf(sounds->jump,gain);accel=(Vec3){0,-CAR_GRAVITY,0};torque=(Vec3){0,0,0};
        }
    }else{
        if(allow_air_roll&&s->recovery_time<=0&&s->air_jump_available&&in.jump&&!*jump_held){
            velocity->y=fmaxf(velocity->y,0)+AIR_JUMP_SPEED;s->air_jump_available=0;sounds->jump=fmaxf(sounds->jump,gain);
            /* Positive pitch about local right tips the nose forward/down. */
            s->flip_axis=mul(right,fabsf(in.pitch)>=.25f?(in.pitch<0?-1:1):((in.throttle<-.1f||in.brake>.1f)?-1:1));
            s->flip_remaining=6.2831853f;s->flip_time=1.3f;s->flip_hit_available=1;s->air_roll_active=-1;
            s->angular_velocity=(Vec3){0,0,0};s->cooldown=fmaxf(s->cooldown,.18f);
        }
        s->contact_frame_valid=0;s->wall_arc=0;
        {Vec3 thrust=mul(s->forward,in.throttle*(burning?BOOST_ACCEL:DRIVE_ACCEL)*.25f);
            /* Upward air thrust cannot pin a sliding car or hold it at the roof. */
            if(thrust.y>0)thrust.y*=s->upper_wall_sliding?0:clamp((ARENA_HEIGHT-center.y-3)/5,0,1);
            accel=add(accel,thrust);
        }
        if(allow_air_roll&&s->flip_time<=0&&s->air_roll_active>=0&&in.jump&&fabsf(in.steer)>.15f)s->air_roll_active=1;
        if(s->flip_time>0){
            /* Bounded pitch motor; brake near one full revolution. */
            float target=clamp(s->flip_remaining*18,-10,10);
            s->air_roll_active=-1;
            Vec3 error=sub(mul(s->flip_axis,target),s->angular_velocity);
            float magnitude=sqrtf(dot(error,error));
            torque=mul(error,magnitude>2?120/magnitude:60);
        }else if(s->recovery_time<=0&&s->air_roll_active>0){
            /* Faster manual roll stays below the 8 rad/s spin cap; release brakes it. */
            float target=in.jump&&fabsf(in.steer)>.15f?-in.steer*7.5f:0;
            float rate=dot(s->angular_velocity,s->forward);
            torque=add(torque,mul(s->forward,clamp((target-rate)*18+target*.65f*.85f,-55,55)));
        }else torque=add(torque,mul(s->up,in.steer*.8f));
        /* Contact-assisted recovery is limited to a slow overturned car near the floor. */
        if(s->flip_time<=0&&s->recovery_time<=0&&s->air_roll_active<=0&&p->y<2.3f&&s->up.y<.4f&&dot(*velocity,*velocity)<100&&fabsf(in.throttle)>.1f){
            Vec3 axis=cross(s->up,(Vec3){0,1,0});if(dot(axis,axis)<.001f)axis=s->forward;
            torque=add(torque,mul(unit(axis),32));
        }
    }
    if(s->recovery_time>0){
        Vec3 axis=cross(s->up,(Vec3){0,1,0});float angle=acosf(clamp(s->up.y,-1,1));
        if(dot(axis,axis)<.0001f)axis=mul(s->forward,(float)s->recovery_direction);
        else axis=unit(axis);
        {Vec3 target=mul(axis,fminf(5,angle*7)),error=sub(target,s->angular_velocity);
            float magnitude=sqrtf(dot(error,error));
            /* Bounded angular motor; collision integration still owns orientation. */
            torque=mul(error,magnitude>3?40/magnitude:13.333333f);
        }
        s->recovery_time=fmaxf(0,s->recovery_time-dt);
        if(count>=2&&s->up.y>.9f)s->recovery_time=0;
    }
    if(count&&s->recovery_time<=0&&s->air_roll_active>0)s->air_roll_active=0;
    *jump_held=in.jump!=0;*boost=clamp(*boost+(burning?-.30f:.12f)*dt,0,1);
    *velocity=add(*velocity,mul(accel,dt));
    {float speed=sqrtf(dot(*velocity,*velocity));if(speed>BOOST_SPEED)*velocity=mul(*velocity,BOOST_SPEED/speed);}
    s->angular_velocity=add(s->angular_velocity,mul(inertia_inverse(s,torque),dt));
    s->angular_velocity=mul(s->angular_velocity,expf(-(count?1.6f:.65f)*dt));
    {float spin=sqrtf(dot(s->angular_velocity,s->angular_velocity)),limit=s->flip_time>0?11:8;if(spin>limit)s->angular_velocity=mul(s->angular_velocity,limit/spin);}
    center=add(center,mul(*velocity,dt));
    if(s->flip_time>0){
        float advance=atanf(dot(s->angular_velocity,s->flip_axis)*dt);
        s->flip_remaining-=advance;s->flip_time=fmaxf(0,s->flip_time-dt);
        if(s->flip_time<=0||(fabsf(s->flip_remaining)<.012f&&dot(s->angular_velocity,s->angular_velocity)<.09f)){
            s->flip_remaining=0;s->flip_time=0;s->flip_hit_available=0;s->angular_velocity=(Vec3){0,0,0};
        }
    }
    s->up=unit(add(s->up,mul(cross(s->angular_velocity,s->up),dt)));
    s->forward=add(s->forward,mul(cross(s->angular_velocity,s->forward),dt));
    right=unit(cross(s->up,s->forward));s->forward=unit(cross(right,s->up));
    *p=sub(center,mul(s->up,.85f));vehicle_resolve(p,velocity,s);
    /* Pitch inversion must not swing the chase camera through 180 degrees. */
    if(s->flip_time<=0&&s->forward.x*s->forward.x+s->forward.z*s->forward.z>.05f)*heading=atan2f(s->forward.x,s->forward.z);
}
