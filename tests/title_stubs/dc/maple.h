#ifndef TITLE_TEST_MAPLE_H
#define TITLE_TEST_MAPLE_H
#define MAPLE_FUNC_CONTROLLER 1
typedef struct {struct {unsigned functions;} info;} maple_device_t;
maple_device_t *maple_enum_dev(int port,int unit);
maple_device_t *maple_enum_type(int port,int func);
void *maple_dev_status(maple_device_t *dev);
#endif
