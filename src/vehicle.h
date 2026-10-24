#ifndef VECTORBALL_VEHICLE_H
#define VECTORBALL_VEHICLE_H
#include "game.h"
void vehicle_step(Vec3 *p,Vec3 *velocity,float *heading,float *boost,int *jump_held,CarSurface *s,SoundEvents *sounds,float gain,Input in,float dt,int allow_air_roll);
void vehicle_resolve(Vec3 *p,Vec3 *velocity,CarSurface *s);
int vehicle_probe(Vec3 origin,Vec3 direction,float reach,WheelContact *hit);
#endif
