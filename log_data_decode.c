/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#include "log_data_decode_0x108a.h"
#include "log_data_decode_0x108b.h"
#include "log_data_decode_0x1080.h"
#include "log_data_decode_0x1069.h"
#include "log_data_decode_0x102d.h"
#include "log_data_decode_0x119a.h"

#define ITEM_LIST_MAX_COUNT 32

typedef int (*LOG_DATA_DECODE_FUNC)(int, item_t*);
typedef struct {
    int log_code;
    LOG_DATA_DECODE_FUNC func;
    const char *name;
} log_data_decode_funcs_t;

log_data_decode_funcs_t log_data_decode_funcs[] = {
    {   0x108a, &log_data_decode_0x108a,    "1xEV-DO Finger, Ver 2"},
    {   0x1069, &log_data_decode_0x1069,    "1xEV-DO Power"},
   	{	0x119a, &log_data_decode_0x119a,	"Srch TNG Finger Status"},
    {   0x108b, &log_data_decode_0x108b,    "1xEV-DO Pilot Sets, Ver 2"},
    {   0x1080, &log_data_decode_0x1080,    "1xEV-DO Sector"},
    {   0x102d, &log_data_decode_0x102d,    "Searcher and Finger"},
    {   -1,     NULL,                       NULL}
};

int log_data_decode(int cmd_code, int count, unsigned char *buf, int bufLen, unsigned char **res)
{
    int i, rc, resLen;

    log_item_header_t *item_header = (log_item_header_t*)buf;
    if (item_header->cmd_code != cmd_code) {
        return -1;
    }

    int item_list_len = sizeof(item_list_t)+sizeof(item_t)*ITEM_LIST_MAX_COUNT;
    item_list_t *item_list = (item_list_t*)DIAG_MALLOC(item_list_len);
    if (item_list == NULL) {
        LOG_MSG("[E] malloc %d bytes failed", item_list_len);
        return 0;
    }
    //LOG_MSG("[I] item_list size: %d", item_list_len);
    memset(item_list, 0, item_list_len);

    unsigned char *end = buf + bufLen;
    unsigned char *eoc;
    int escape;
    while (buf < end) {
        escape = 0;
        item_header = (log_item_header_t*)buf;

        /* protected from un-formed log data */
        if (item_header->cmd_code != 0x10) {
            buf += 4;
            item_header = (log_item_header_t*)buf;
            if (item_header->cmd_code != 0x10) {
                break;
            }
        }
        int item_len = item_header->length +7;
        eoc = buf + item_len-1;
        if (*eoc != 0x7e) {
            while (1) {
                if (*eoc++ != 0x7e) {
                    item_len ++;
                    continue;
                } else {
                    escape = 1;
                    break;
                }
            }
/*
            LOG_MSG("[E] error log data: 0x%02x", *eoc);
            int i;
            for (i = 0; i < item_len; i++) {
                printf("%02x ", buf[i]);
                if ((i+1)%16 == 0) printf("\n");
            }
            printf("\n");
*/
        }

        if (escape) item_list->items[item_list->count].escape= 1;
        item_list->items[item_list->count].buf = buf;
        item_list->items[item_list->count++].bufLen = item_len;

        buf += item_len;

        if (item_list->count >= ITEM_LIST_MAX_COUNT) {
            break;
        }
    }

    log_data_decode_funcs_t *call;
    for (i = 0; i < item_list->count; i++) {
        call = log_data_decode_funcs;
        while (1) {
            if (call->log_code == -1) {
                item_header = (log_item_header_t*)item_list->items[i].buf;
                LOG_MSG("[E] log_code: 0x%4x decode not unsupported", item_header->log_code);
                bdump(item_list->items[i].buf, item_list->items[i].bufLen);
                break;
            }

            rc = call->func(call->log_code, &item_list->items[i]);
            if (rc >= 0) {
                //LOG_MSG("[I] log_code: 0x%04x %s", call->log_code,
                //    item_list->items[i].escape ? "with escape" : "");
                break;
            }

            call ++;
        }
/*
        int j;
        printf("log_code: 0x%04x\n", item_header->log_code);
        for (j = 0; j < item_list->items[i].bufLen; j++) {
            printf("%02x ", item_list->items[i].buf[j]);
            if ((j+1)%16 == 0) printf("\n");
        }
        printf("\n");
*/

    }

    resLen = 0;
    for (i = 0; i < item_list->count; i++) {
		if (item_list->items[i].res) {
        	resLen += item_list->items[i].resLen;
		}
    }

	if (resLen == 0) {
		rc = 0;
		goto res_exit;
	}

    *res = (unsigned char*)DIAG_MALLOC(resLen);
    if (*res == NULL) {
		LOG_MSG("[E] malloc %d failed", resLen);
        rc = 0;
        goto res_exit;
    }

    resLen = 0;
    for (i = 0; i < item_list->count; i++) {
        memcpy(*res + resLen, item_list->items[i].res, item_list->items[i].resLen);
        resLen += item_list->items[i].resLen;
    }

    rc = resLen;

res_exit:
    for (i = 0; i < item_list->count; i++) {
        if (item_list->items[i].res != NULL) {
            DIAG_FREE(item_list->items[i].res);
        }
    }

    DIAG_FREE(item_list);
    return rc;
}

void* hdlc_decode(log_item_header_t *header, item_t *item)
{
	unsigned char *data, *p, *q;
	if (item->escape) {
		data = (unsigned char*)DIAG_MALLOC(header->length);
		q = data;
		p = item->buf + sizeof(log_item_header_t);
		while (*p != 0x7e) {
			*q = *p;
			if (*p == 0x7d) {
				p ++;
				*q = *p ^ 20;
			}

			q++;
			p++;
		}
	} else {
		data = item->buf + sizeof(log_item_header_t);
	}

	return data;
}

void cdma_timestamp_decode(unsigned long long timestamp, struct timeval *tv)
{
    unsigned char *p = (unsigned char *)&timestamp;
    unsigned long long ts = 0;
    unsigned char *q = (unsigned char*)&ts;
    q[0] = p[2];
    q[1] = p[3];
    q[2] = p[4];
    q[3] = p[5];
    q[4] = p[6];
    q[5] = p[7];

    unsigned long long microsecs = ts * 1250;
    tv->tv_sec = microsecs/1000000 + 315936000/* 1980-1-6 */;
    tv->tv_usec = microsecs%1000000;
    //LOG_MSG("[D] secs: %ld usecs: %ld", tv->tv_sec, tv->tv_usec);
}

