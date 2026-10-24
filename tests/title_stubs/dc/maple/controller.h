#ifndef TITLE_TEST_CONTROLLER_H
#define TITLE_TEST_CONTROLLER_H
#include <stdint.h>
enum {CONT_A=1,CONT_B=2,CONT_X=4,CONT_Y=8,CONT_START=16,CONT_DPAD_LEFT=32,CONT_DPAD_RIGHT=64,CONT_DPAD_UP=128,CONT_DPAD_DOWN=256};
typedef struct {uint32_t buttons;int joyx,joyy,rtrig,ltrig;} cont_state_t;
#endif
