#include <stdint.h>
typedef struct {uint32_t size,cmd,timestamp,cmd_id,misc[4];uint8_t cmd_data[];} aica_cmd_t;
typedef struct {uint32_t cmd,base,type,length,loop,loopstart,loopend,freq,vol,pan,pos,pad[5];} aica_channel_t;
#define AICA_CMDSTR_CHANNEL(T,C,H) uint32_t T[(sizeof(aica_cmd_t)+sizeof(aica_channel_t))/4];aica_cmd_t *C=(aica_cmd_t *)T;aica_channel_t *H=(aica_channel_t *)C->cmd_data
#define AICA_CMDSTR_CHANNEL_SIZE ((sizeof(aica_cmd_t)+sizeof(aica_channel_t))/4)
#define AICA_CMD_CHAN 2
#define AICA_CH_CMD_UPDATE 3
#define AICA_CH_UPDATE_SET_FREQ 0x1000
#define AICA_CH_UPDATE_SET_VOL 0x2000
