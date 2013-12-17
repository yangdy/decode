/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#ifndef _DIAG_DECODE_H_
#define _DIAG_DECODE_H_

#define MEMORY_DATA_TYPE 0x20

#pragma pack(1)

typedef struct {
    unsigned int type;
    unsigned count;
    unsigned length;
} data_header_t;

#pragma pack()

typedef int (*DATA_DECODE_FUNC)(int, unsigned char*, int, unsigned char**);
typedef struct {
    int type;
    DATA_DECODE_FUNC func;
    const char *name;
} data_decode_funcs_t;

extern data_decode_funcs_t data_decode_funcs[];

#endif /* _DIAG_DECODE_H_ */
