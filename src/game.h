#ifndef VECTORBALL_GAME_H
#define VECTORBALL_GAME_H
#define RL_SCALE (2.2f/91.25f) /* Fixed world scale; independent of ball resizing. */
#define ARENA_X (4096.0f*RL_SCALE)
#define ARENA_Z (5120.0f*RL_SCALE)
#define ARENA_HEIGHT (2048.0f*RL_SCALE)
#define GOAL_W (892.755f*RL_SCALE)
#define SPAWN_Z (4608.0f*RL_SCALE)
#define CPU_SETUP_DISTANCE 7.0f
#define CPU_AROUND_DISTANCE 9.0f
#define GOAL_H (642.775f*RL_SCALE)
#define BALL_R 3.4375f /* Another 25 percent above v0.29; world scale stays fixed. */
#define CORNER_R 14.0f
#define BANK_W 6.0f
#define BANK_H 3.0f
#define BANK_SEGMENTS 6
#define ARENA_EDGES 30
#define WALL_TOP ARENA_HEIGHT
#define JUMP_SPEED 9.5f
#define AIR_JUMP_SPEED 7.6f
#define CAR_GRAVITY (650.0f*RL_SCALE)
#define BALL_GRAVITY CAR_GRAVITY
#define GAME_PACE 1.10f
#define BALL_HIT_GAIN (1.45f*GAME_PACE)
#define CAR_SPEED (1410.0f*RL_SCALE*GAME_PACE)
#define BOOST_SPEED (2300.0f*RL_SCALE*GAME_PACE)
#define BALL_SPEED (6000.0f*RL_SCALE*GAME_PACE)
/* First tuning pass: preserve response times with the higher speed caps. */
#define DRIVE_ACCEL (20.0f*CAR_SPEED/25.0f)
#define BOOST_ACCEL (34.0f*BOOST_SPEED/37.0f)
/* Flat/near-flat handling; wall carving keeps its accepted steering law. */
#define TURN_ACCEL_LIMIT (56.0f*GAME_PACE*GAME_PACE) /* Preserve the accepted fast-turn radius. */
/* Tighter fast turns; low-speed full lock stays 2.15 rad/s. */
#define BRAKE_DECEL 50.0f
#define STEP (1.0f/120.0f)
typedef struct { float x,y,z; } Vec3;
typedef struct { float steer,throttle,brake; int boost,jump; float pitch; } Input;
typedef struct {float hit,jump,land;int goal;} SoundEvents;
typedef struct {Vec3 point,normal;float distance;int hit;} WheelContact;
typedef struct {
    Vec3 up,forward,angular_velocity;
    Vec3 contact_up,contact_forward;
    int contact_frame_valid,wall_arc;
    WheelContact wheels[4];
    float cooldown;
    int attached,grounded_wheels,dynamic;
    float recovery_time;
    int recovery_direction,air_roll_active;
    int air_jump_available;
    Vec3 flip_axis;
    float flip_remaining,flip_time;
    int flip_hit_available;
    int upper_wall_sliding;
} CarSurface;
#define CONTACT_SPARKS 4
#define CONTACT_SPARK_TIME .14f
typedef struct {Vec3 normal;float life,power;} ContactSpark;
typedef struct {ContactSpark sparks[CONTACT_SPARKS];Vec3 hit_position;float car_hit;} ImpactFeedback;
typedef struct {float strength,phase;} BoostExhaust;
typedef struct {float roll,steer;} CarVisual;
typedef struct {float bank,pitch,heave,bank_rate,pitch_rate,heave_rate;int grounded;} CarSuspension;
typedef struct {
    CarSurface car_surface,cpu_surface;
    SoundEvents sounds;
    Vec3 car, velocity, ball, ball_velocity;
    Vec3 cpu, cpu_velocity;
    float cpu_heading, cpu_boost, cpu_stuck, cpu_reverse, cpu_attack;
    int cpu_enabled;
    int jump_held,cpu_jump_held;
    float heading, boost, reset_timer, elapsed;
    int blue_score, orange_score, last_goal;
    /* Cosmetic only; appended so the prior gameplay prefix stays comparable. */
    CarVisual car_visual,cpu_visual;
    CarSuspension car_suspension,cpu_suspension;
    ImpactFeedback feedback;
    BoostExhaust exhaust,cpu_exhaust;
} Game;
void arena_edge(int index,Vec3 *a,Vec3 *b);
float arena_floor(float x,float z);
void arena_bank_profile(int index,float *distance,float *height);
Vec3 arena_bank_vertex(int edge,int end,int section);
void game_car_basis(const Game *g,int cpu,Vec3 *right,Vec3 *up,Vec3 *forward);
float game_steering_scale(float speed);
float game_ground_force(float speed,Input in,int burning,float dt);
void game_clear_events(Game *g);
void game_init(Game *g);
void game_reset(Game *g);
void game_step(Game *g, Input in, float dt);
void game_step_two(Game *g,Input blue,Input orange,float dt);
#endif
