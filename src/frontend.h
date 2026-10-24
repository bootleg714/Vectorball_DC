#ifndef VECTORBALL_FRONTEND_H
#define VECTORBALL_FRONTEND_H
typedef enum {FRONT_TITLE,FRONT_CONTROLS,FRONT_MATCH,FRONT_RESULTS} FrontPage;
enum {FRONT_PREV=1,FRONT_NEXT=2,FRONT_ACCEPT=4,FRONT_BACK=8};
typedef struct {FrontPage page;int selection,stick_latched,two_player;} Frontend;
void frontend_init(Frontend *f);
void frontend_return(Frontend *f);
/* Pressed button edges, current menu axis, controller availability. Returns 1 on new match. */
int frontend_step(Frontend *f,unsigned pressed,float axis,int connected);
#endif
