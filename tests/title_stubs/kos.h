#ifndef TITLE_TEST_KOS_H
#define TITLE_TEST_KOS_H
#include <stdint.h>
#include <stddef.h>
#define KOS_INIT_FLAGS(x)
#define INIT_DEFAULT 0
#define DM_640x480 0
#define PM_RGB565 0
void vid_set_mode(int a,int b);
uint64_t timer_ms_gettime64(void);
#endif
