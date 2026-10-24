#include "render.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static Camera cam;
static int viewport_top,viewport_height=480,split_player;
static float split_focal,frustum_x,frustum_y,sphere_x,sphere_y;
static float hud_depth;
static Vec3 car_right,car_up,car_forward,car_origin;
static Vec3 wheel_right,wheel_up,wheel_forward;
static Vec3 v(float x,float y,float z){return (Vec3){x,y,z};}
static Vec3 add(Vec3 a,Vec3 b){return v(a.x+b.x,a.y+b.y,a.z+b.z);}
static Vec3 sub(Vec3 a,Vec3 b){return v(a.x-b.x,a.y-b.y,a.z-b.z);}
static Vec3 mul(Vec3 a,float b){return v(a.x*b,a.y*b,a.z*b);}
static float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec3 cross(Vec3 a,Vec3 b){return v(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static Vec3 norm(Vec3 a){float l=sqrtf(dot(a,a));return l>.001f?mul(a,1/l):v(0,0,1);}
static float camera_clamp(float x,float lo,float hi){return fmaxf(lo,fminf(hi,x));}
static float camera_angle(float a){
    while(a>3.14159265f)a-=6.2831853f;
    while(a< -3.14159265f)a+=6.2831853f;
    return a;
}
static void camera_basis(Camera *c,Vec3 forward){
    c->forward=norm(forward);
    c->right=cross(v(0,1,0),c->forward);
    if(dot(c->right,c->right)<.0001f)c->right=v(cosf(c->orbit_yaw),0,-sinf(c->orbit_yaw));
    else c->right=norm(c->right);
    c->up=cross(c->forward,c->right);
}
/* Exact conservative screen-space bounds: eight corners around each object. */
static float framing_limit(const Camera *c,const Game *g,int ball,float margin){
    float limit=390;int i;Vec3 right,up,forward;
    game_car_basis(g,0,&right,&up,&forward);
    for(i=0;i<8;i++){
        float x=i&1?CAR_FRAME_WIDTH:-CAR_FRAME_WIDTH,z=i&4?CAR_FRAME_LENGTH:-CAR_FRAME_LENGTH;Vec3 p;
        if(ball)p=add(g->ball,v(i&1?BALL_R:-BALL_R,i&2?BALL_R:-BALL_R,i&4?BALL_R:-BALL_R));
        else p=add(g->car,add(add(mul(right,x),mul(up,i&2?CAR_FRAME_HEIGHT:0)),mul(forward,z)));
        p=sub(p,c->eye);
        {float d=dot(p,c->forward),px=dot(p,c->right),py=dot(p,c->up);
            if(d<=.25f)return 0;
            if(fabsf(px)>.001f)limit=fminf(limit,(320-margin)*d/fabsf(px));
            if(py>.001f)limit=fminf(limit,(240-55-margin)*d/py);
            if(py<-.001f)limit=fminf(limit,(480-240-margin)*d/-py);
        }
    }
    return limit;
}
void camera_update(Camera *c,const Game *g,int ball_camera,float dt,int snap){
    Vec3 dir=v(sinf(g->heading),0,cosf(g->heading)),target,eye;
    int teleport=c->initialized&&dot(sub(g->car,c->previous_car),sub(g->car,c->previous_car))>18*18;
    int first=!c->initialized||snap||teleport;
    float alpha=1-expf(-camera_clamp(dt,0,.1f)*7);
    if(!ball_camera){
        /* Preserve the accepted chase camera. */
        eye=add(g->car,add(mul(dir,-13),v(0,8,0)));eye.y=8+fmaxf(0,g->car.y-2);
        eye.x=camera_clamp(eye.x,-ARENA_X+1,ARENA_X-1);
        eye.z=camera_clamp(eye.z,-ARENA_Z+1,ARENA_Z-1);
        c->eye=first?eye:add(c->eye,mul(sub(eye,c->eye),fminf(1,dt*7)));
        target=add(g->car,add(mul(dir,8),v(0,1,0)));target.y=1+g->car.y;
        c->orbit_yaw=g->heading;camera_basis(c,sub(target,c->eye));c->focal=390;
    }else{
        Vec3 delta=sub(g->ball,g->car),car_direction,ball_direction,look;
        float horizontal=hypotf(delta.x,delta.z),high=fmaxf(0,delta.y-4);
        float desired_yaw,desired_distance,desired_height,room,weight=.52f;
        float car_limit,ball_limit,wanted,safety;
        if(first){c->orbit_yaw=horizontal>3?atan2f(delta.x,delta.z):g->heading;c->distance=BALL_CAM_DISTANCE;c->height=8;c->focal=390;}
        else if(!c->mode){c->orbit_yaw=atan2f(g->car.x-c->eye.x,g->car.z-c->eye.z);c->distance=BALL_CAM_DISTANCE;c->height=8;}
        desired_yaw=horizontal>3?atan2f(delta.x,delta.z):c->orbit_yaw;
        if(first)c->orbit_yaw=desired_yaw;
        else c->orbit_yaw+=camera_clamp(camera_angle(desired_yaw-c->orbit_yaw)*alpha,-3.5f*dt,3.5f*dt);
        desired_distance=BALL_CAM_DISTANCE+camera_clamp(high,0,30)*.40f+camera_clamp(8-horizontal,0,8)*.35f;
        c->distance=first?desired_distance:c->distance+(desired_distance-c->distance)*(1-expf(-dt*3));
        dir=v(sinf(c->orbit_yaw),0,cosf(c->orbit_yaw));
        eye=add(g->car,mul(dir,-c->distance));
        eye.x=camera_clamp(eye.x,-ARENA_X+3,ARENA_X-3);
        eye.z=camera_clamp(eye.z,-ARENA_Z+3,ARENA_Z-3);
        room=hypotf(eye.x-g->car.x,eye.z-g->car.z)/c->distance;
        desired_height=8+camera_clamp(high,0,25)*.10f+(1-room)*8;
        c->height=first?desired_height:c->height+(desired_height-c->height)*(1-expf(-dt*5));
        eye.y=c->height+fmaxf(0,g->car.y-2); /* Follow wall height; ordinary hops stay stable. */
        c->eye=first?eye:add(c->eye,mul(sub(eye,c->eye),alpha));
        car_direction=norm(sub(add(g->car,v(0,.9f,0)),c->eye));
        ball_direction=norm(sub(g->ball,c->eye));
        /* During a close pass the ball may briefly be behind the orbiting eye.
           Keep the car visible while the orbit catches up. */
        if(dot(car_direction,ball_direction)<-.15f)weight=.12f;
        look=norm(add(mul(car_direction,1-weight),mul(ball_direction,weight)));
        camera_basis(c,first?look:add(c->forward,mul(sub(look,c->forward),1-expf(-dt*10))));
        car_limit=framing_limit(c,g,0,45);
        if(car_limit<120){camera_basis(c,car_direction);car_limit=framing_limit(c,g,0,45);}
        ball_limit=framing_limit(c,g,1,45);
        wanted=fminf(car_limit,ball_limit>0?ball_limit:car_limit);
        wanted=camera_clamp(wanted,170,390);
        c->focal=first?wanted:c->focal+(wanted-c->focal)*(1-expf(-dt*(wanted<c->focal?7:2.5f)));
        /* A looser emergency border prevents the car clipping during fast hits.
           Normal zoom changes ease in/out inside this reserve. */
        safety=framing_limit(c,g,0,28);c->focal=fminf(c->focal,safety);
    }
    c->previous_car=g->car;c->initialized=1;c->mode=ball_camera!=0;
}
void camera_update_player(Camera *c,const Game *g,int player,int ball_camera,float dt,int snap){
    if(player){Game other=*g;other.car=g->cpu;other.velocity=g->cpu_velocity;other.heading=g->cpu_heading;other.car_surface=g->cpu_surface;camera_update(c,&other,ball_camera,dt,snap);}
    else camera_update(c,g,ball_camera,dt,snap);
}
/* Screen clipping prevents geometry or depth from leaking between views. */
static void submit(Screen a,Screen b,Screen c,uint32_t color){
    Screen in[10],out[10];int n=3,plane,i;
    if(viewport_height==480){backend_triangle(a,b,c,color);return;}
    {float minx=fminf(a.x,fminf(b.x,c.x)),maxx=fmaxf(a.x,fmaxf(b.x,c.x));
        float miny=fminf(a.y,fminf(b.y,c.y)),maxy=fmaxf(a.y,fmaxf(b.y,c.y));
        if(maxx<0||minx>640||maxy<viewport_top||miny>viewport_top+viewport_height)return;
        if(minx>=0&&maxx<=640&&miny>=viewport_top&&maxy<=viewport_top+viewport_height){backend_triangle(a,b,c,color);return;}
    }
    in[0]=a;in[1]=b;in[2]=c;
    for(plane=0;plane<4&&n;plane++){
        int count=0;float edge=plane==0?0:plane==1?640:plane==2?(float)viewport_top:(float)(viewport_top+viewport_height);
        for(i=0;i<n;i++){
            Screen s=in[i],e=in[(i+1)%n];float sv=plane<2?s.x:s.y,ev=plane<2?e.x:e.y;
            int si=plane%2?sv<=edge:sv>=edge,ei=plane%2?ev<=edge:ev>=edge;
            if(si)out[count++]=s;
            if(si!=ei){float t=(edge-sv)/(ev-sv);Screen q={s.x+(e.x-s.x)*t,s.y+(e.y-s.y)*t,s.z+(e.z-s.z)*t};if(plane<2)q.x=edge;else q.y=edge;out[count++]=q;}
        }
        n=count;for(i=0;i<n;i++)in[i]=out[i];
    }
    for(i=1;i+1<n;i++)backend_triangle(in[0],in[i],in[i+1],color);
}
static Vec3 view(Vec3 p){p=sub(p,cam.eye);return v(dot(p,cam.right),dot(p,cam.up),dot(p,cam.forward));}
/* Conservative object rejection saves all mesh work outside a split frustum. */
static int visible(Vec3 center,float radius){
    Vec3 p;
    if(viewport_height==480)return 1;
    p=view(center);
    return p.z+radius>=.25f&&fabsf(p.x)<=frustum_x*p.z+radius*sphere_x&&fabsf(p.y)<=frustum_y*p.z+radius*sphere_y;
}
static Screen project(Vec3 p){
    if(viewport_height<480){float inverse=1/p.z,f=split_focal*inverse;return (Screen){320+p.x*f,viewport_top+120-p.y*f,inverse};}
    return (Screen){320+p.x*(cam.focal>0?cam.focal:390)/p.z,240-p.y*(cam.focal>0?cam.focal:390)/p.z,1/p.z};
}
static void triangle_view(Vec3 a,Vec3 b,Vec3 c,uint32_t color){
    Vec3 in[4]={a,b,c},out[5];int n=0,i;
    if(viewport_height<480){
        if(a.z<.25f&&b.z<.25f&&c.z<.25f)return;
        if((a.x>frustum_x*a.z&&b.x>frustum_x*b.z&&c.x>frustum_x*c.z)||(a.x< -frustum_x*a.z&&b.x< -frustum_x*b.z&&c.x< -frustum_x*c.z)||
           (a.y>frustum_y*a.z&&b.y>frustum_y*b.z&&c.y>frustum_y*c.z)||(a.y< -frustum_y*a.z&&b.y< -frustum_y*b.z&&c.y< -frustum_y*c.z))return;
        if(a.z>=.25f&&b.z>=.25f&&c.z>=.25f){submit(project(a),project(b),project(c),color);return;}
    }
    /* Clip against the near plane instead of dropping crossing polygons. */
    for(i=0;i<3;i++){
        Vec3 s=in[i],e=in[(i+1)%3];int si=s.z>=.25f,ei=e.z>=.25f;
        if(si)out[n++]=s;
        if(si!=ei){float t=(.25f-s.z)/(e.z-s.z);out[n++]=add(s,mul(sub(e,s),t));}
    }
    for(i=1;i+1<n;i++)submit(project(out[0]),project(out[i]),project(out[i+1]),color);
}
static void triangle(Vec3 a,Vec3 b,Vec3 c,uint32_t color){triangle_view(view(a),view(b),view(c),color);}
static void quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,uint32_t col){
    if(viewport_height<480){a=view(a);b=view(b);c=view(c);d=view(d);triangle_view(a,b,c,col);triangle_view(a,c,d,col);}
    else {triangle(a,b,c,col);triangle(a,c,d,col);}
}
static uint32_t shade(uint32_t col,float k){
    unsigned r=((col>>16)&255)*k,g=((col>>8)&255)*k,b=(col&255)*k;
    return 0xff000000u|(r<<16)|(g<<8)|b;
}
static void box(float x,float y,float z,float w,float h,float d,uint32_t color){
    Vec3 a=v(x-w,y,z-d),b=v(x+w,y,z-d),c=v(x+w,y,z+d),e=v(x-w,y,z+d);
    Vec3 A=add(a,v(0,h,0)),B=add(b,v(0,h,0)),C=add(c,v(0,h,0)),E=add(e,v(0,h,0));
    quad(A,B,C,E,color);quad(a,b,B,A,shade(color,.7f));quad(b,c,C,B,shade(color,.85f));
    quad(c,e,E,C,shade(color,.7f));quad(e,a,A,E,shade(color,.85f));
}
static void floor_rect(float x,float z,float w,float d,uint32_t col){quad(v(x-w,.02f,z-d),v(x+w,.02f,z-d),v(x+w,.02f,z+d),v(x-w,.02f,z+d),col);}
static void night_sky(void){
    int i;const uint32_t colors[8]={0xff040915,0xff060d1c,0xff091323,0xff0c1a2a,0xff102131,0xff142838,0xff182e3e,0xff1b3442};
    for(i=0;i<8;i++){
        float y0=viewport_top+i*viewport_height/8.0f,y1=viewport_top+(i+1)*viewport_height/8.0f;
        Screen a={0,y0,.00001f},b={640,y0,.00001f},c={640,y1,.00001f},d={0,y1,.00001f};
        submit(a,b,c,colors[i]);submit(a,c,d,colors[i]);
    }
    if(viewport_height<480)return;
    /* World-fixed sky details: no textures and no per-frame random shimmer. */
    for(i=0;i<32;i++){
        float a=i*2.399963f,r=.35f+(i%3)*.13f;
        Vec3 p=v(sinf(a)*290,55+(i*37%110),cosf(a)*290);
        Vec3 right=mul(cam.right,r),up=mul(cam.up,r);
        quad(sub(sub(p,right),up),add(sub(p,up),right),add(add(p,right),up),add(sub(p,right),up),i%4==0?0xffa7c4cc:0xff627d92);
    }
    {Vec3 center=v(-108,82,172);int k;
        for(k=0;k<16;k++){
            float a=k*6.2831853f/16,b=(k+1)*6.2831853f/16;
            Vec3 p=add(center,add(mul(cam.right,cosf(a)*8),mul(cam.up,sinf(a)*8)));
            Vec3 q=add(center,add(mul(cam.right,cosf(b)*8),mul(cam.up,sinf(b)*8)));
            triangle(center,p,q,0xffb6cbd1);
        }
    }
}
static void city_building(float x,float z,float w,float d,float h,int seed,int side_axis,int side){
    int row,col;uint32_t paint=seed%3==0?0xff243b4b:(seed%3==1?0xff1c3041:0xff2a4050);
    box(x,0,z,w,h,d,paint);
    if(seed%3==0)box(x,h,z,w*.68f,3+seed%5,d*.68f,shade(paint,.85f));
    if(viewport_height<480)return;
    for(row=0;row<4;row++)for(col=0;col<3;col++){
        float a=(col-1)*w*.52f,y=4+row*(h-6)/4;
        uint32_t lamp=(seed+row+col)%4==0?0xff8a8058:0xff4e7282;
        if((seed*7+row*11+col*3)%5<2)continue;
        if(side_axis){
            float face=x-side*(w+.02f),zz=z+(col-1)*d*.52f;
            quad(v(face,y,zz-.40f),v(face,y,zz+.40f),v(face,y+1,zz+.40f),v(face,y+1,zz-.40f),lamp);
        }else{
            float face=z-side*(d+.02f);
            quad(v(x+a-.40f,y,face),v(x+a+.40f,y,face),v(x+a+.40f,y+1,face),v(x+a-.40f,y+1,face),lamp);
        }
    }
}
static void fence_panel(int side_axis,float fixed,float from,float to,float bottom,float top){
    int i,n=(int)((to-from)/4.5f);const uint32_t wire=0xff294853;
    for(i=0;i<=n;i++){
        float p=from+(to-from)*i/n;
        if(side_axis)quad(v(fixed,bottom,p-.025f),v(fixed,bottom,p+.025f),v(fixed,top,p+.025f),v(fixed,top,p-.025f),wire);
        else quad(v(p-.025f,bottom,fixed),v(p+.025f,bottom,fixed),v(p+.025f,top,fixed),v(p-.025f,top,fixed),wire);
    }
    for(i=0;i<3;i++){
        float y=bottom+(top-bottom)*i/2;
        if(side_axis)quad(v(fixed,y,from),v(fixed,y+.045f,from),v(fixed,y+.045f,to),v(fixed,y,to),wire);
        else quad(v(from,y,fixed),v(to,y,fixed),v(to,y+.045f,fixed),v(from,y+.045f,fixed),wire);
    }
}
static void floodlight(int side,float z){
    int row,col;float x=side*(ARENA_X+4.5f),face=x-side*.72f;
    box(x,0,z,.18f,19,.18f,0xff455b68);
    box(x,18.8f,z,.70f,2.2f,2.8f,0xff283e4a);
    for(row=0;row<2;row++)for(col=0;col<4;col++){
        float zz=z-2.25f+col*1.5f,y=19.0f+row*1.05f;
        quad(v(face,y,zz-.48f),v(face,y,zz+.48f),v(face,y+.68f,zz+.48f),v(face,y+.68f,zz-.48f),0xfffff1cf);
    }
}
static void stadium_environment(void){
    int side,i,tier;
    quad(v(-160,-.05f,-180),v(160,-.05f,-180),v(160,-.05f,180),v(-160,-.05f,180),0xff0e1c29);
    if(viewport_height<480){
        /* Four simple skyline blocks and low stands; no windows/lamps/mesh. */
        for(side=-1;side<=1;side+=2){
            for(i=-1;i<=1;i+=2){Vec3 center=v(side*(ARENA_X+18),12,i*(ARENA_Z*.70f));
                if(visible(center,24))box(center.x,0,center.z,6,22+(i+1)*3,6,0xff243b4b);
            }
            box(side*(ARENA_X+5),0,0,3,5,ARENA_Z,0xff263e4b);
            box(0,0,side*(ARENA_Z+9),ARENA_X+4,5,2,0xff263e4b);
        }
        return;
    }
    for(side=-1;side<=1;side+=2){
        for(i=0;i<8;i++){
            int seed=i+13+(side+1)*7;
            city_building(side*(ARENA_X+23+(i%3)*7),-ARENA_Z-12+i*((2*ARENA_Z+24)/7),3.5f+i%3,4.0f,13+(seed*13%24),seed,1,side);
        }
        for(i=0;i<5;i++){
            int seed=i+41+(side+1)*5;
            city_building(-ARENA_X-12+i*((2*ARENA_X+24)/4),side*(ARENA_Z+26+(i%2)*9),4.5f,4,16+(seed*9%26),seed,0,side);
        }
        /* Stepped spectator terraces outside the collision walls. */
        for(tier=0;tier<3;tier++){
            uint32_t seat=tier%2?0xff263e4b:0xff304c57;
            box(side*(ARENA_X+2+tier*2),0,0,1,3+tier*1.1f,ARENA_Z+3,seat);
            box(0,0,side*(ARENA_Z+8+tier*2),ARENA_X+7,3+tier*1.1f,1,seat);
        }
        /* End mesh starts above the crossbar, leaving both goal mouths clear. */
        fence_panel(0,side*ARENA_Z,-GOAL_W,GOAL_W,GOAL_H+.55f,WALL_TOP);
        for(i=0;i<3;i++)floodlight(side,(-.70f+i*.70f)*ARENA_Z);
        /* Architectural spire gives each end a recognizable skyline landmark. */
        box(side*(ARENA_X+21),0,side*(ARENA_Z+30),5.5f,41,5.5f,0xff2b4355);
        box(side*(ARENA_X+21),41,side*(ARENA_Z+30),3.7f,7,3.7f,0xff38586a);
        box(side*(ARENA_X+21),48,side*(ARENA_Z+30),.18f,6,.18f,0xff74939f);
    }
}
static Vec3 car_point(const Game *g,float x,float y,float z){(void)g;return add(car_origin,add(add(mul(car_right,x),mul(car_up,y)),mul(car_forward,z)));}
static void car_box(const Game *g,float x,float y,float z,float w,float h,float d,uint32_t col){
    Vec3 a=car_point(g,x-w,y,z-d),b=car_point(g,x+w,y,z-d),c=car_point(g,x+w,y,z+d),e=car_point(g,x-w,y,z+d);
    Vec3 A=add(a,mul(car_up,h)),B=add(b,mul(car_up,h)),C=add(c,mul(car_up,h)),E=add(e,mul(car_up,h));
    quad(A,B,C,E,col);quad(a,b,B,A,shade(col,.55f));quad(b,c,C,B,shade(col,.8f));quad(c,e,E,C,shade(col,.7f));quad(e,a,A,E,shade(col,.8f));
}
/* Original compact hatchback, modelled directly in local coordinates. */
static void car_quad(const Game *g,Vec3 a,Vec3 b,Vec3 c,Vec3 d,uint32_t color){
    quad(car_point(g,a.x,a.y,a.z),car_point(g,b.x,b.y,b.z),car_point(g,c.x,c.y,c.z),car_point(g,d.x,d.y,d.z),color);
}
static void car_triangle(const Game *g,Vec3 a,Vec3 b,Vec3 c,uint32_t color){
    triangle(car_point(g,a.x,a.y,a.z),car_point(g,b.x,b.y,b.z),car_point(g,c.x,c.y,c.z),color);
}
static const float wheel_cos[11]={1,.809017f,.309017f,-.309017f,-.809017f,-1,-.809017f,-.309017f,.309017f,.809017f,1};
static const float wheel_sin[11]={0,.587785f,.951057f,.951057f,.587785f,0,-.587785f,-.951057f,-.951057f,-.587785f,0};
/* Cached once per car; no additional geometry or per-segment trig. */
static float roll_c,roll_s,steer_c,steer_s;
static uint32_t spoke_light,spoke_dark;
static Vec3 wheel_point(int side,float axle,float across,float height,float along){
    float c=axle>0?steer_c:1,s=axle>0?steer_s:0;
    return v(side*1.205f+across*c+along*s,.52f+height,axle+along*c-across*s);
}
static void wheel(const Game *g,int side,float z){
    int k;float inside=-side*.165f,outside=side*.165f,rim=outside+side*.008f;
    Vec3 saved_right=car_right,saved_up=car_up,saved_forward=car_forward,saved_origin=car_origin;
    /* Wheels stay on the accepted physical frame while the shell flexes. */
    car_right=wheel_right;car_up=wheel_up;car_forward=wheel_forward;car_origin=g->car;
    for(k=0;k<10;k+=viewport_height<480?2:1){
        int next=k+(viewport_height<480?2:1);
        float a=wheel_cos[k],b=wheel_sin[k],c=wheel_cos[next],d=wheel_sin[next];
        float ra=a*roll_c+b*roll_s,rb=b*roll_c-a*roll_s;
        float rc=c*roll_c+d*roll_s,rd=d*roll_c-c*roll_s;
        uint32_t tread=b+d>0?0xff293039:0xff141b25;
        /* Keep the circular tire silhouette steady; the contrasting rim spins. */
        car_quad(g,wheel_point(side,z,inside,b*.50f,a*.50f),wheel_point(side,z,outside,b*.50f,a*.50f),wheel_point(side,z,outside,d*.50f,c*.50f),wheel_point(side,z,inside,d*.50f,c*.50f),tread);
        car_triangle(g,wheel_point(side,z,outside,0,0),wheel_point(side,z,outside,b*.50f,a*.50f),wheel_point(side,z,outside,d*.50f,c*.50f),0xff0d1420);
        car_triangle(g,wheel_point(side,z,rim,0,0),wheel_point(side,z,rim,rb*.33f,ra*.33f),wheel_point(side,z,rim,rd*.33f,rc*.33f),k%2?spoke_light:spoke_dark);
        if(viewport_height==480)car_triangle(g,wheel_point(side,z,rim+side*.003f,0,0),wheel_point(side,z,rim+side*.003f,rb*.11f,ra*.11f),wheel_point(side,z,rim+side*.003f,rd*.11f,rc*.11f),0xff263b51);
    }
    car_right=saved_right;car_up=saved_up;car_forward=saved_forward;car_origin=saved_origin;
}
static const float arch_cos[9]={1,.923880f,.707107f,.382683f,0,-.382683f,-.707107f,-.923880f,-1};
static const float arch_sin[9]={0,.382683f,.707107f,.923880f,1,.923880f,.707107f,.382683f,0};
static void arch(const Game *g,int side,float z,uint32_t color){
    int k;
    /* Side panels have real cutouts. A thin painted lip surrounds each opening. */
    for(k=0;k<8;k++){
        float za=z+arch_cos[k]*.62f,zb=z+arch_cos[k+1]*.62f;
        float ya=.52f+arch_sin[k]*.62f,yb=.52f+arch_sin[k+1]*.62f;
        float x=side*1.16f;
        car_quad(g,v(x,ya,za),v(x,yb,zb),v(x,1.15f,zb),v(x,1.15f,za),shade(color,side<0?.72f:.87f));
        car_quad(g,v(x+side*.018f,ya,za),v(x+side*.018f,yb,zb),v(x+side*.018f,yb+.055f,zb),v(x+side*.018f,ya+.055f,za),shade(color,.52f));
    }
}
/* Two small solid tapered jets, using the shell basis and existing opaque pass. */
static void boost_exhaust(const Game *g){
    int side,k;float pulse,length;
    static const float rx[4]={1,0,-1,0},ry[4]={0,1,0,-1};
    if(g->exhaust.strength<=0)return;
    pulse=1-.10f*fabsf(g->exhaust.phase*2-1);
    length=(.65f+1.20f*g->exhaust.strength)*pulse;
    for(side=-1;side<=1;side+=2){
        Vec3 tip=car_point(g,side*.48f,.41f,-2.055f-length);
        for(k=0;k<4;k++){
            int next=(k+1)%4;
            Vec3 a=car_point(g,side*.48f+rx[k]*.10f,.41f+ry[k]*.09f,-2.055f);
            Vec3 b=car_point(g,side*.48f+rx[next]*.10f,.41f+ry[next]*.09f,-2.055f);
            Vec3 c=car_point(g,side*.48f+rx[next]*.14f,.41f+ry[next]*.12f,-2.055f-length*.30f);
            Vec3 d=car_point(g,side*.48f+rx[k]*.14f,.41f+ry[k]*.12f,-2.055f-length*.30f);
            quad(a,b,c,d,0xffffeed0);
            triangle(d,c,tip,0xffffa02e);
        }
    }
}
static void draw_car_low(const Game *g,uint32_t color){
    int side;
    car_box(g,0,.35f,0,1.13f,.7f,1.85f,color);
    car_quad(g,v(-.95f,1.05f,-1.25f),v(.95f,1.05f,-1.25f),v(.75f,1.7f,-.8f),v(-.75f,1.7f,-.8f),0xff213b4d);
    car_quad(g,v(-.75f,1.7f,-.8f),v(.75f,1.7f,-.8f),v(.75f,1.7f,.35f),v(-.75f,1.7f,.35f),color);
    car_quad(g,v(-.75f,1.7f,.35f),v(.75f,1.7f,.35f),v(.95f,1.05f,1),v(-.95f,1.05f,1),0xff2c5265);
    for(side=-1;side<=1;side+=2){
        car_quad(g,v(side*.95f,1.05f,-1.25f),v(side*.95f,1.05f,1),v(side*.75f,1.7f,.35f),v(side*.75f,1.7f,-.8f),0xff213b4d);
        car_quad(g,v(side*.48f,.65f,1.86f),v(side*.85f,.65f,1.86f),v(side*.85f,.83f,1.86f),v(side*.48f,.83f,1.86f),0xffffefbf);
        car_quad(g,v(side*.48f,.65f,-1.86f),v(side*.85f,.65f,-1.86f),v(side*.85f,.83f,-1.86f),v(side*.48f,.83f,-1.86f),0xffff4a39);
        wheel(g,side,-1.12f);wheel(g,side,1.12f);
    }
    car_quad(g,v(-.12f,1.056f,1),v(.12f,1.056f,1),v(.12f,1.056f,1.8f),v(-.12f,1.056f,1.8f),0xffd5e3de);
    car_quad(g,v(-.12f,1.708f,-.8f),v(.12f,1.708f,-.8f),v(.12f,1.708f,.35f),v(-.12f,1.708f,.35f),0xffd5e3de);
    car_box(g,0,1.2f,-1.6f,1.02f,.08f,.18f,color);boost_exhaust(g);
}
static void draw_car(const Game *g,uint32_t color){
    int side;uint32_t dark=0xff172532,glass=0xff213b4d;
    game_car_basis(g,0,&car_right,&car_up,&car_forward);
    wheel_right=car_right;wheel_up=car_up;wheel_forward=car_forward;car_origin=g->car;
    roll_c=cosf(g->car_visual.roll);roll_s=sinf(g->car_visual.roll);
    steer_c=cosf(g->car_visual.steer);steer_s=sinf(g->car_visual.steer);
    {float speed=fabsf(dot(g->velocity,car_forward)),contrast=fmaxf(.06f,1-speed/20);
        unsigned r=(unsigned)(44*contrast),green=(unsigned)(42*contrast),b=(unsigned)(39*contrast);
        /* Lower contrast at speed reduces the wagon-wheel/strobing effect. */
        spoke_light=0xff000000u|((136+r)<<16)|((159+green)<<8)|(175+b);
        spoke_dark=0xff000000u|((136-r)<<16)|((159-green)<<8)|(175-b);
    }
    {float cr=cosf(g->car_suspension.bank),sr=sinf(g->car_suspension.bank);
        float cp=cosf(g->car_suspension.pitch),sp=sinf(g->car_suspension.pitch);
        Vec3 rolled_up=sub(mul(wheel_up,cr),mul(wheel_right,sr));
        car_right=add(mul(wheel_right,cr),mul(wheel_up,sr));
        car_up=add(mul(rolled_up,cp),mul(wheel_forward,sp));
        car_forward=sub(mul(wheel_forward,cp),mul(rolled_up,sp));
        car_origin=add(g->car,sub(mul(wheel_up,.85f+g->car_suspension.heave),mul(car_up,.85f)));
    }
    if(viewport_height<480){draw_car_low(g,color);return;}
    /* Upper body: bevelled ends, rising shoulders and a long sloping hood. */
    car_quad(g,v(-1.16f,1.15f,-1.72f),v(1.16f,1.15f,-1.72f),v(1.16f,1.15f,.95f),v(-1.16f,1.15f,.95f),shade(color,.94f));
    car_quad(g,v(-1.16f,1.15f,.95f),v(1.16f,1.15f,.95f),v(.90f,.88f,1.95f),v(-.90f,.88f,1.95f),color);
    car_quad(g,v(-1.16f,1.15f,-1.72f),v(1.16f,1.15f,-1.72f),v(.98f,.91f,-1.95f),v(-.98f,.91f,-1.95f),shade(color,.82f));
    car_quad(g,v(-.90f,.88f,1.95f),v(.90f,.88f,1.95f),v(.95f,.38f,1.86f),v(-.95f,.38f,1.86f),shade(color,.78f));
    car_quad(g,v(-.98f,.91f,-1.95f),v(.98f,.91f,-1.95f),v(1,.38f,-1.85f),v(-1,.38f,-1.85f),shade(color,.65f));
    for(side=-1;side<=1;side+=2){
        float x=side*1.16f;
        arch(g,side,-1.12f,color);arch(g,side,1.12f,color);
        car_quad(g,v(x,.38f,-.50f),v(x,.38f,.50f),v(x,1.15f,.50f),v(x,1.15f,-.50f),shade(color,side<0?.72f:.87f));
        car_quad(g,v(side*.98f,.38f,-1.95f),v(x,.38f,-1.74f),v(x,1.15f,-1.74f),v(side*.98f,.91f,-1.95f),shade(color,.70f));
        car_quad(g,v(x,.38f,1.74f),v(side*.90f,.38f,1.95f),v(side*.90f,.88f,1.95f),v(x,1.15f,1.74f),shade(color,.80f));
        /* Rocker trim, door handle and mirrors. */
        car_box(g,side*1.17f,.36f,0,.035f,.12f,.58f,dark);
        car_box(g,side*1.18f,.99f,-.26f,.018f,.035f,.14f,0xffc3d5db);
        car_box(g,side*1.10f,1.21f,.57f,.15f,.12f,.14f,shade(color,.75f));
        /* Cabin side shell and inset glass leave body-coloured pillars visible. */
        car_quad(g,v(side*.98f,1.15f,-1.39f),v(side*.98f,1.15f,.99f),v(side*.79f,1.74f,.35f),v(side*.79f,1.70f,-.85f),shade(color,.85f));
        car_quad(g,v(side*.978f,1.23f,-1.20f),v(side*.978f,1.23f,.83f),v(side*.811f,1.65f,.30f),v(side*.811f,1.62f,-.80f),glass);
        /* Dark B-pillar, following the same side-window slope. */
        car_quad(g,v(side*.983f,1.23f,-.29f),v(side*.983f,1.23f,-.20f),v(side*.815f,1.64f,-.20f),v(side*.815f,1.64f,-.29f),dark);
        wheel(g,side,-1.12f);wheel(g,side,1.12f);
    }
    /* Roof, windshield, and sloped hatch glass. */
    car_quad(g,v(-.79f,1.70f,-.85f),v(.79f,1.70f,-.85f),v(.79f,1.74f,.35f),v(-.79f,1.74f,.35f),color);
    car_quad(g,v(-.98f,1.15f,.99f),v(.98f,1.15f,.99f),v(.79f,1.74f,.35f),v(-.79f,1.74f,.35f),shade(color,.75f));
    car_quad(g,v(-.86f,1.23f,.914f),v(.86f,1.23f,.914f),v(.72f,1.68f,.421f),v(-.72f,1.68f,.421f),0xff2c5265);
    car_quad(g,v(-.98f,1.15f,-1.39f),v(.98f,1.15f,-1.39f),v(.79f,1.70f,-.85f),v(-.79f,1.70f,-.85f),shade(color,.70f));
    car_quad(g,v(-.88f,1.22f,-1.325f),v(.88f,1.22f,-1.325f),v(.72f,1.63f,-.928f),v(-.72f,1.63f,-.928f),glass);
    /* Narrow roof/hood stripe and front grille. */
    car_quad(g,v(-.12f,1.748f,.32f),v(.12f,1.748f,.32f),v(.12f,1.708f,-.82f),v(-.12f,1.708f,-.82f),0xffd5e3de);
    car_quad(g,v(-.12f,1.156f,1),v(.12f,1.156f,1),v(.12f,.915f,1.84f),v(-.12f,.915f,1.84f),0xffd5e3de);
    car_box(g,0,.45f,1.925f,.53f,.20f,.045f,dark);
    car_box(g,0,.35f,1.86f,.99f,.09f,.12f,dark);
    car_box(g,0,.35f,-1.87f,1.02f,.13f,.10f,dark);
    /* Lamp colours provide front/back cues without extra lights or transparency. */
    for(side=-1;side<=1;side+=2){
        car_box(g,side*.67f,.72f,1.938f,.19f,.105f,.035f,0xffffefbf);
        car_box(g,side*.65f,.78f,-1.953f,.25f,.10f,.025f,0xffff4a39);
        car_box(g,side*.48f,.41f,-1.98f,.10f,.09f,.07f,0xff91a9b1);
        car_box(g,side*.70f,1.16f,-1.59f,.055f,.28f,.07f,dark);
    }
    car_box(g,0,.57f,-1.963f,.19f,.09f,.018f,0xffc4dde2);
    car_box(g,0,1.44f,-1.60f,1.02f,.075f,.19f,shade(color,.82f));
    boost_exhaust(g);
}
static void ball(const Game *g){
    int i,j,rows=viewport_height<480?4:6,columns=viewport_height<480?8:10;const float pi=3.14159265f;
    for(i=0;i<rows;i++)for(j=0;j<columns;j++){
        float a=-pi/2+i*pi/rows,b=a+pi/rows,t=j*2*pi/columns,u=t+2*pi/columns;
        Vec3 p=add(g->ball,v(cosf(a)*sinf(t)*BALL_R,sinf(a)*BALL_R,cosf(a)*cosf(t)*BALL_R));
        Vec3 q=add(g->ball,v(cosf(a)*sinf(u)*BALL_R,sinf(a)*BALL_R,cosf(a)*cosf(u)*BALL_R));
        Vec3 r=add(g->ball,v(cosf(b)*sinf(u)*BALL_R,sinf(b)*BALL_R,cosf(b)*cosf(u)*BALL_R));
        Vec3 s=add(g->ball,v(cosf(b)*sinf(t)*BALL_R,sinf(b)*BALL_R,cosf(b)*cosf(t)*BALL_R));
        float light=.5f+.5f*fmaxf(0,dot(norm(sub(p,g->ball)),norm(v(-1,2,-1))));
        quad(p,q,r,s,shade((i+j)%3==0?0xff203549:0xffedf6dc,light));
    }
}
/* Tiny opaque camera-facing contact glints in the existing depth-tested pass. */
static void contact_sparks(const Game *g){
    static const float dx[6]={1,.5f,-.5f,-1,-.5f,.5f};
    static const float dy[6]={0,.8660254f,.8660254f,0,-.8660254f,-.8660254f};
    int i,k;
    for(i=0;i<CONTACT_SPARKS;i++){
        const ContactSpark *spark=&g->feedback.sparks[i];float phase,radius,width;Vec3 center;
        if(spark->life<=0)continue;
        phase=camera_clamp(1-spark->life/CONTACT_SPARK_TIME,0,1);
        radius=(.18f+.35f*spark->power)*(.5f+phase);width=.10f*(1-phase);
        /* Follow the struck surface patch so a fast shot leaves no rearward flare. */
        center=sub(add(g->ball,mul(spark->normal,-BALL_R-.03f)),mul(cam.forward,.20f));
        for(k=0;k<6;k++){
            Vec3 direction=add(mul(cam.right,dx[k]),mul(cam.up,dy[k]));
            Vec3 across=mul(add(mul(cam.right,-dy[k]),mul(cam.up,dx[k])),width);
            Vec3 inner=add(center,mul(direction,radius*.22f)),outer=add(center,mul(direction,radius));
            triangle(sub(inner,across),add(inner,across),outer,phase<.45f?0xffffc04a:0xffff8e28);
        }
        {Vec3 r=mul(cam.right,.08f*(1-phase)),u=mul(cam.up,.08f*(1-phase));
            quad(add(center,r),add(center,u),sub(center,r),sub(center,u),0xfffff8dc);
        }
    }
}
/* Analytic goal fireworks: no particle pool, RNG, heap or simulation changes.
   The existing two-second goal timer supplies age and freezes naturally on pause. */
static void goal_fireworks(const Game *g){
    static const float dx[12]={1,.8660254f,.5f,0,-.5f,-.8660254f,-1,-.8660254f,-.5f,0,.5f,.8660254f};
    static const float dy[12]={0,.5f,.8660254f,1,.8660254f,.5f,0,-.5f,-.8660254f,-1,-.8660254f,-.5f};
    static const float offset[3]={-.55f,.55f,0};
    int burst,k;float age=2-g->reset_timer,sign=g->last_goal>0?1.0f:-1.0f;
    if(g->reset_timer<=0||!g->last_goal)return;
    for(burst=0;burst<3;burst++){
        float t=age-burst*.32f,life=1.08f,size;
        Vec3 center=v(offset[burst]*GOAL_W,GOAL_H*.48f+burst*1.2f,sign*(ARENA_Z-3));
        if(t<0||t>=life)continue;
        size=.38f*(1-t/life);
        for(k=0;k<12;k+=viewport_height<480?2:1){
            float speed=9+burst*1.2f,previous=fmaxf(0,t-.13f);
            Vec3 velocity=v(dx[k]*speed,dy[k]*speed,sign*(k%3-1)*3);
            Vec3 head=add(center,add(mul(velocity,t),v(0,-3*t*t,0)));
            Vec3 tail=add(center,add(mul(velocity,previous),v(0,-3*previous*previous,0)));
            Vec3 r=mul(cam.right,size),u=mul(cam.up,size);
            uint32_t color=k%3==0?0xffffd166:(g->last_goal>0?0xff61cfff:0xffff944d);
            triangle(sub(head,r),add(head,r),tail,color);
            quad(add(head,r),add(head,u),sub(head,r),sub(head,u),k%3==1?0xfffff3cf:color);
        }
    }
}
static void rect(float x,float y,float w,float h,uint32_t col){
    float z=hud_depth;y+=viewport_top;
    Screen a={x,y,z},b={x+w,y,z},c={x+w,y+h,z},d={x,y+h,z};
    hud_depth+=.001f;submit(a,b,c,col);submit(a,c,d,col);
}
/* Compact built-in 5x7 font: no texture or filesystem dependency. */
static const char letters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-";
static const unsigned char glyphs[][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},
{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,15},{17,17,17,31,17,17,17},
{31,4,4,4,4,4,31},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},
{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},
{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},
{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14},
{0,4,4,0,4,4,0},{0,0,0,31,0,0,0}};
static void text(float x,float y,const char *s,int scale,uint32_t col){
    for(;*s;s++,x+=6*scale){const char *p=strchr(letters,*s);int r,c;if(!p)continue;
        for(r=0;r<7;r++)for(c=0;c<5;c++)if(glyphs[p-letters][r]&(1<<(4-c))){
            int run=1;
            if(viewport_height<480)while(c+run<5&&(glyphs[p-letters][r]&(1<<(4-c-run))))run++;
            rect(x+c*scale,y+r*scale,run*scale,scale,col);c+=run-1;
        }
    }
}
void render_frontend(FrontPage page,int selection,int connected,int artwork){
    static const char *actions[8]={"STEER","ACCELERATE","BRAKE OR REVERSE","BOOST","JUMP","CAMERA","RESET","PAUSE"};
    static const char *inputs[8]={"STICK OR DPAD","R TRIGGER","L TRIGGER","A","B","X","Y","START"};
    int i;hud_depth=100;
    if(!artwork){rect(0,0,640,480,0xff071522);text(140,120,"VECTORBALL DC",3,0xffe6f3ff);}
    if(page==FRONT_CONTROLS){
        rect(62,65,516,382,0xff101d2a);text(272,82,"CONTROLS",2,0xffeef1d4);
        for(i=0;i<8;i++){text(84,122+i*30,actions[i],2,0xffcfedf3);text(345,122+i*30,inputs[i],2,0xffeef1d4);}
        text(176,369,"FLIP         SECOND B + STICK UP/DOWN",1,0xffcfedf3);
        text(176,385,"BACKFLIP     STICK DOWN ON SECOND B",1,0xffcfedf3);
        text(176,401,"RECOVER      TAP B WHEN OVERTURNED",1,0xffcfedf3);
        text(176,417,"AIR ROLL     FIRST JUMP: B + STEER",1,0xffcfedf3);
        text(260,435,"B TO RETURN",1,0xffcfedf3);
    }else{
        int count=page==FRONT_RESULTS?2:3;
        for(i=0;i<count;i++){
            float width=count==3?170:210,x=count==3?45+i*190:100+i*230;uint32_t accent=i==1?0xffffa342:0xff39a6ff;
            const char *label=page==FRONT_RESULTS?(i?"TITLE":"REMATCH"):(i==0?"START MATCH":i==1?"CONTROLS":"2P VERSUS");
            rect(x,384,width,44,selection==i?accent:0xff254052);rect(x+2,386,width-4,40,0xff101d2a);
            text(x+(width-(float)strlen(label)*12)*.5f,398,label,2,selection==i?0xfff2f6eb:0xff8faebd);
        }
    }
    if(!connected)text(206,446,"CONNECT CONTROLLER",1,0xfff2e5c5);
}
void render_match_clock(const Match *m){
    char clock[24];int seconds;
    hud_depth=150;
    if(m->phase==MATCH_OVERTIME||m->overtime_ticks>0){
        seconds=m->overtime_ticks/120;
        snprintf(clock,sizeof(clock),"OT %02d:%02d",(seconds/60)%100,seconds%60);
        rect(210,21,114,26,0xff101d2a);text(219,28,clock,2,0xffffd166);
    }else{
        seconds=(m->ticks_left+119)/120;
        snprintf(clock,sizeof(clock),"%02d:%02d",seconds/60,seconds%60);
        rect(222,21,84,26,0xff101d2a);text(234,28,clock,2,seconds<=10?0xffffa342:0xffeef1d4);
    }
}
void render_results(const Game *g,int selection,int connected,int artwork){
    char score[40];const char *winner=g->blue_score>g->orange_score?"BLUE WINS":"ORANGE WINS";
    render_frontend(FRONT_RESULTS,selection,connected,artwork);
    rect(145,140,350,170,0xff101d2a);text(236,161,"MATCH COMPLETE",2,0xffeef1d4);
    text(320-(float)strlen(winner)*6,202,winner,2,g->blue_score>g->orange_score?0xff39a6ff:0xffffa342);
    snprintf(score,sizeof(score),"BLUE %02d - %02d ORANGE",g->blue_score%100,g->orange_score%100);
    text(320-(float)strlen(score)*6,254,score,2,0xffeef1d4);
}
static void render_scene(const Game *g,const Camera *c,int paused,int ball_camera){
    int x,z,k,nx=viewport_height<480?2:8,nz=viewport_height<480?4:14;char score[48];cam=*c;hud_depth=8;
    split_focal=(cam.focal>0?cam.focal:390)*.5f;frustum_x=320/split_focal;frustum_y=120/split_focal;sphere_x=sqrtf(1+frustum_x*frustum_x);sphere_y=sqrtf(1+frustum_y*frustum_y);
    night_sky();
    stadium_environment();
    /* Fixed tile count: enlarging the pitch does not multiply floor triangles. */
    for(x=0;x<nx;x++)for(z=0;z<nz;z++){
        float x0=-ARENA_X+x*(2*ARENA_X/nx),x1=-ARENA_X+(x+1)*(2*ARENA_X/nx);
        float z0=-ARENA_Z+z*(2*ARENA_Z/nz),z1=-ARENA_Z+(z+1)*(2*ARENA_Z/nz);
        quad(v(x0,0,z0),v(x1,0,z0),v(x1,0,z1),v(x0,0,z1),((x+z)&1)?0xff214c40:0xff285647);
    }
    for(k=-1;k<=1;k+=2){
        float edge=k*ARENA_Z;
        uint32_t tint=k>0?0xff504631:0xff20465e;
        quad(v(-GOAL_W-3,.01f,edge),v(GOAL_W+3,.01f,edge),v(GOAL_W+3,.01f,edge-k*8),v(-GOAL_W-3,.01f,edge-k*8),tint);
    }
    floor_rect(0,0,ARENA_X,.10f,0xffc4d7b9);
    floor_rect(-ARENA_X+1,0,.10f,ARENA_Z-1,0xff397d88);floor_rect(ARENA_X-1,0,.10f,ARENA_Z-1,0xff397d88);
    for(k=0;k<32;k+=viewport_height<480?2:1){
        float a=k*6.2831853f/32,b=(k+(viewport_height<480?2:1))*6.2831853f/32;
        quad(v(sinf(a)*10,.04f,cosf(a)*10),v(sinf(b)*10,.04f,cosf(b)*10),v(sinf(b)*10.15f,.04f,cosf(b)*10.15f),v(sinf(a)*10.15f,.04f,cosf(a)*10.15f),0xffa0c5b0);
    }
    /* Shared boundary vertices keep the visible banks and rounded walls aligned. */
    for(k=0;k<ARENA_EDGES;k++){
        Vec3 a,b,at,bt;int section;
        arena_edge(k,&a,&b);
        if(viewport_height<480&&!visible(v((a.x+b.x)*.5f,WALL_TOP*.5f,(a.z+b.z)*.5f),sqrtf(dot(sub(b,a),sub(b,a)))*.5f+WALL_TOP*.5f+BANK_W))continue;
        for(section=0;section<BANK_SEGMENTS;section++)
            quad(arena_bank_vertex(k,0,section),arena_bank_vertex(k,1,section),arena_bank_vertex(k,1,section+1),arena_bank_vertex(k,0,section+1),section%2?0xff426c65:0xff3c625d);
        a.y=b.y=BANK_H;
        at=a;bt=b;at.y=bt.y=4.15f;quad(a,b,bt,at,0xff223343);
        a.y=b.y=4;quad(a,b,bt,at,0xff39c3d9);
        {int rail,j,posts=(int)(sqrtf((b.x-a.x)*(b.x-a.x)+(b.z-a.z)*(b.z-a.z))/4.5f)+1;
            Vec3 delta=sub(b,a),strip=mul(norm(v(delta.x,0,delta.z)),.025f);
            if(viewport_height<480)posts=1;
            for(rail=0;rail<3;rail+=viewport_height<480?2:1){
                float h=4.15f+(WALL_TOP-4.15f)*rail/2;
                Vec3 p=v(a.x,h,a.z),q=v(b.x,h,b.z);
                quad(p,q,add(q,v(0,.045f,0)),add(p,v(0,.045f,0)),0xff294853);
            }
            for(j=0;j<=posts;j++){
                Vec3 p=add(a,mul(delta,(float)j/posts));p.y=4.15f;
                quad(sub(p,strip),add(p,strip),add(add(p,strip),v(0,WALL_TOP-4.15f,0)),add(sub(p,strip),v(0,WALL_TOP-4.15f,0)),0xff294853);
            }
        }
    }
    /* Close each curved bank end beside the flat goal corridors. */
    {const int goal_edges[4]={6,28,20,29},ends[4]={1,0,1,0};int i,section;
        for(i=0;i<4;i++){
            Vec3 a,b,center;arena_edge(goal_edges[i],&a,&b);center=ends[i]?b:a;
            for(section=0;section<BANK_SEGMENTS;section++)
                triangle(center,arena_bank_vertex(goal_edges[i],ends[i],section),arena_bank_vertex(goal_edges[i],ends[i],section+1),0xff345650);
        }
    }
    for(k=-1;k<=1;k+=2){
        uint32_t col=k>0?0xffffa342:0xff39a6ff;
        float end=k*ARENA_Z;
        box(-GOAL_W,0,end,.23f,GOAL_H,.3f,col);box(GOAL_W,0,end,.23f,GOAL_H,.3f,col);box(0,GOAL_H,end,GOAL_W+.23f,.25f,.3f,col);
        floor_rect(0,end+k*2,GOAL_W,2,col);
        box(0,0,end+k*4,GOAL_W,GOAL_H,.2f,k>0?0xff54412c:0xff163d58);
        box(-GOAL_W-.50f,0,end,.12f,GOAL_H+.30f,.48f,col);
        box(GOAL_W+.50f,0,end,.12f,GOAL_H+.30f,.48f,col);
        box(-GOAL_W,0,end+k*2,.15f,GOAL_H,2,0xff243e4e);box(GOAL_W,0,end+k*2,.15f,GOAL_H,2,0xff243e4e);
        /* Goal-area outlines make distance and approach angle easier to judge. */
        floor_rect(0,end-k*12,GOAL_W+5,.10f,0xffa0c5b0);
        floor_rect(-GOAL_W-5,end-k*6,.10f,6,0xffa0c5b0);
        floor_rect(GOAL_W+5,end-k*6,.10f,6,0xffa0c5b0);
        floor_rect(0,end-k*.15f,GOAL_W+5,.10f,col);
    }
    /* Opaque floor silhouettes avoid a translucent pass. */
    floor_rect(g->ball.x,g->ball.z,BALL_R*.85f,BALL_R*.85f,0xff102426);
    floor_rect(g->car.x,g->car.z,1.7f,2.2f,0xff102426);
    if(visible(add(g->car,v(0,1,0)),5))draw_car(g,0xff248de8);
    if(g->cpu_enabled){
        Game other=*g;other.car=g->cpu;other.heading=g->cpu_heading;other.car_surface=g->cpu_surface;other.velocity=g->cpu_velocity;other.car_visual=g->cpu_visual;other.car_suspension=g->cpu_suspension;other.exhaust=g->cpu_exhaust;
        floor_rect(other.car.x,other.car.z,1.7f,2.2f,0xff102426);
        if(visible(add(other.car,v(0,1,0)),5))draw_car(&other,0xffff953b);
    }
    if(visible(g->ball,BALL_R))ball(g);
    contact_sparks(g);goal_fireworks(g);
    if(viewport_height<480){
        float boost=split_player?g->cpu_boost:g->boost;
        rect(12,8,616,20,0xff101d2a);text(20,14,split_player?"P2 ORANGE":"P1 BLUE",1,split_player?0xffffa342:0xff39a6ff);
        snprintf(score,sizeof(score),"%02d - %02d",g->blue_score%100,g->orange_score%100);text(552,14,score,1,0xfff2e5c5);
        rect(16,215,102,10,0xff101d2a);rect(18,217,98*boost,6,split_player?0xffffa342:0xff36c6f3);
        text(132,217,ball_camera?"BALL CAM":"CHASE CAM",1,0xffcfedf3);
        if(g->reset_timer>0){rect(230,68,180,26,0xff101d2a);text(244,77,g->last_goal>0?"BLUE SCORES":"ORANGE SCORES",1,g->last_goal>0?0xff39a6ff:0xffffa342);}
        return;
    }
    rect(20,17,600,34,0xff101d2a);text(30,26,"VECTORBALL DC",2,0xffb6dfe8);
    snprintf(score,sizeof(score),"BLUE %02d - %02d ORANGE",g->blue_score%100,g->orange_score%100);
    text(340,29,score,1,0xfff2e5c5);
    rect(24,430,126,16,0xff101d2a);rect(27,433,120*g->boost,10,0xff36c6f3);text(24,452,"BOOST",1,0xffcfedf3);
    text(200,452,ball_camera?"BALL CAM":"CHASE CAM",1,0xffcfedf3);
    if(g->reset_timer>0){rect(190,180,260,55,0xff101d2a);text(g->last_goal>0?254:242,199,g->last_goal>0?"BLUE SCORES":"ORANGE SCORES",2,g->last_goal>0?0xff39a6ff:0xffffa342);}
    if(paused){rect(195,180,250,120,0xff101d2a);text(248,196,"PAUSED",2,0xffeef1d4);text(225,232,"START TO RESUME",1,0xffcfedf3);text(225,250,"Y TO RESET MATCH",1,0xffcfedf3);text(225,268,"B TO TITLE",1,0xffcfedf3);}
}

void render_frame(const Game *g,const Camera *c,int paused,int ball_camera){
    viewport_top=0;viewport_height=480;render_scene(g,c,paused,ball_camera);
}
void render_split_frame(const Game *g,const Camera cameras[2],const Match *m,const int ball_camera[2],int paused,int missing){
    int player;char clock[24];int seconds=m->phase==MATCH_OVERTIME||m->overtime_ticks>0?m->overtime_ticks/120:(m->ticks_left+119)/120;
    int ot=m->phase==MATCH_OVERTIME||m->overtime_ticks>0;
    snprintf(clock,sizeof(clock),ot?"OT %02d:%02d":"%02d:%02d",(seconds/60)%100,seconds%60);
    viewport_height=240;
    for(player=0;player<2;player++){
        viewport_top=player*240;split_player=player;render_scene(g,&cameras[player],0,ball_camera[player]);
        hud_depth=150;text(ot?270:284,13,clock,1,ot?0xffffd166:0xffeef1d4);
        if(paused||missing){
            rect(185,100,270,72,0xff101d2a);
            if(missing)text(202,113,missing==1?"CONNECT CONTROLLER A":missing==2?"CONNECT CONTROLLER B":"CONNECT CONTROLLERS A B",1,0xffeef1d4);
            else {text(284,112,"PAUSED",2,0xffeef1d4);text(219,140,"START TO RESUME",1,0xffcfedf3);text(219,156,"P1 B TITLE  Y RESET",1,0xffcfedf3);}
        }
    }
    viewport_top=0;viewport_height=480;hud_depth=200;rect(0,239,640,2,0xff6c8793);
}

/* Diagnostic for the renewed versus test; actual recent drawn frames / real time. */
void render_split_fps(float fps){
    char label[24];viewport_top=0;viewport_height=480;hud_depth=220;
    if(fps>0)snprintf(label,sizeof(label),"FPS %02d",(int)(fps+.5f));else snprintf(label,sizeof(label),"FPS --");
    rect(291,233,58,14,0xff101d2a);text(300,237,label,1,0xffcfedf3);
}
