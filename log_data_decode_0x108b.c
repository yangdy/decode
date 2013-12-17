/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#include <math.h>

#pragma pack(1)

typedef struct {
	unsigned char pnInc;
	unsigned char aSETCount;
	unsigned char aSETWindow;
	unsigned short channel;
	unsigned char srchState;
	unsigned char cSETCount;
	unsigned char cSETWindow;
	unsigned char nSETCount;
	unsigned short rSETWindow;
} pilot_sets_t;

typedef struct {
	unsigned short pilotPN;
	unsigned short pilotEng;
	unsigned short field1;
	unsigned short field2;
	unsigned short field3;
} fields_t;

#pragma pack()

int log_data_decode_0x108b(int log_code, item_t *item)
{
    log_item_header_t *header = (log_item_header_t*)item->buf;
    if (header->log_code != log_code) {
        return -1;
    }

	unsigned char *data = hdlc_decode(header, item);

	pilot_sets_t *sets = (pilot_sets_t*)data;
	fields_t *fields = (fields_t*)(sets+1);

	int count = sets->aSETCount + sets->cSETCount + sets->nSETCount;
	int aCount = sets->aSETCount;
    int cCount = sets->cSETCount;
    int nCount = sets->nSETCount;

	LOG_MSG("[I] log_code: 0x%04x PN count: %d ASet: %d CSet: %d NSet: %d",
        header->log_code, count, aCount, cCount, nCount);

    //float eng;
	while (aCount--) {
        //eng = 10 * log10(fields->pilotEng/512.0f);
		LOG_MSG("[I] log_code: 0x%04x A PN: 0x%04x S: 0x%04x",
            header->log_code, fields->pilotPN, fields->pilotEng);
		fields++;
	}

    while (cCount--) {
        //eng = 10 * log10(fields->pilotEng/512.0f);
		LOG_MSG("[I] log_code: 0x%04x C PN: 0x%04x S: 0x%04x",
            header->log_code, fields->pilotPN, fields->pilotEng);
		fields++;
    }

/*

    while (nCount--) {
        //eng = 10 * log10(fields->pilotEng/512.0f);
		LOG_MSG("[I] log_code: 0x%04x N PN: 0x%04x S: 0x%04x",
            header->log_code, fields->pilotPN, fields->pilotEng);
		fields++;
    }
*/

	if (item->escape) {
		DIAG_FREE(data);
	}

    return 0;
}

