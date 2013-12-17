/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#pragma pack(1)

typedef struct {
	unsigned char sectorId[16];
	unsigned char subnetMask;
	unsigned char colorCode;
	unsigned short pnOffset;
} sector_t;

#pragma pack()

int log_data_decode_0x1080(int log_code, item_t *item)
{
    log_item_header_t *header = (log_item_header_t*)item->buf;
    if (header->log_code != log_code) {
        return -1;
    }

	unsigned char *data = hdlc_decode(header, item);

	sector_t *sector = (sector_t*)data;

	LOG_MSG("[I] log_code: 0x%04x PN: 0x%04x", header->log_code, sector->pnOffset);

	if (item->escape) {
		DIAG_FREE(data);
	}

    return 0;
}

