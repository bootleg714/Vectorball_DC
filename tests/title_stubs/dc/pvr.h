#ifndef TITLE_TEST_PVR_H
#define TITLE_TEST_PVR_H
#include <stddef.h>
#include <stdint.h>
typedef void *pvr_ptr_t;
typedef struct {int dummy;} pvr_poly_hdr_t;
typedef struct {struct {int culling;} gen;} pvr_poly_cxt_t;
typedef struct {int flags;float x,y,z,u,v;uint32_t argb,oargb;} pvr_vertex_t;
enum {PVR_CMD_VERTEX=0,PVR_CMD_VERTEX_EOL=1,PVR_LIST_OP_POLY=0,PVR_CULLING_NONE=0,PVR_TXRFMT_RGB565=1,PVR_TXRFMT_NONTWIDDLED=2,PVR_FILTER_BILINEAR=1};
typedef struct {int opb_sizes[5],vertex_buf_size,opb_overflow_count;} pvr_init_params_t;
enum {PVR_BINSIZE_16=16,PVR_BINSIZE_0=0};
int pvr_init(const pvr_init_params_t *p);
int pvr_init_defaults(void);
void pvr_set_bg_color(float a,float b,float c);
void pvr_poly_cxt_col(pvr_poly_cxt_t *c,int list);
void pvr_poly_compile(pvr_poly_hdr_t *h,const pvr_poly_cxt_t *c);
void pvr_poly_cxt_txr(pvr_poly_cxt_t *c,int list,int format,int w,int h,pvr_ptr_t memory,int filter);
pvr_ptr_t pvr_mem_malloc(size_t n);
void pvr_mem_free(pvr_ptr_t m);
void pvr_txr_load(const void *src,pvr_ptr_t dst,size_t n);
void pvr_prim(const void *p,size_t n);
void pvr_wait_ready(void);
void pvr_scene_begin(void);
void pvr_list_begin(int list);
void pvr_list_finish(void);
void pvr_scene_finish(void);
#endif
