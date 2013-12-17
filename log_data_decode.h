/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#ifndef _LOG_DATA_DECODE_H_
#define _LOG_DATA_DECODE_H_

#pragma pack(1)

typedef struct {
    unsigned char cmd_code;
    unsigned char more;
    unsigned short length;
    unsigned short length2;
    unsigned short log_code;
    unsigned long long timestamp;
} log_item_header_t;

typedef struct {
    unsigned char *buf;
    int bufLen;
    int escape;
    unsigned char *res;
    int resLen;
} item_t;

typedef struct {
    int count;
    item_t items[0];
} item_list_t;

#pragma pack()

int log_data_decode(int cmd_code, int count, unsigned char *buf, int bufLen, unsigned char **res);

void* hdlc_decode(log_item_header_t *header, item_t *item);
void cdma_timestamp_decode(unsigned long long timestamp, struct timeval *tv);

#endif /* _LOG_DATA_DECODE_H_ */
