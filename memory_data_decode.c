/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "memory_data_decode.h"
#include "log_data_decode.h"

typedef int (*MEMORY_DATA_DECODE_FUNC)(int, int, unsigned char*, int, unsigned char**);
typedef struct {
    int cmd_code;
    MEMORY_DATA_DECODE_FUNC func;
    const char *name;
} memory_data_decode_funcs_t;

memory_data_decode_funcs_t memory_data_decode_funcs[] = {
    {   0x10,   &log_data_decode,   "log data"  },
    {   -1,     NULL,               NULL        }
};

int memory_data_decode(int type, unsigned char *buf, int bufLen, unsigned char **res)
{
    int rc;
    data_header_t *data_header = (data_header_t*)buf;
    if (data_header->type != type) {
        return -1;
    }

    buf += sizeof(data_header_t);
    bufLen -= sizeof(data_header_t);

    memory_data_decode_funcs_t *call = memory_data_decode_funcs;
    while (1) {
        if (call->cmd_code == -1) {
            return -1;
        }

        rc = call->func(call->cmd_code, data_header->count, buf, bufLen, res);
        if (rc >= 0) {
            return rc;
        }

        call++;
    }
}
