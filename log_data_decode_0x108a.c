/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#include <math.h>
#include <time.h>
#include <sys/time.h>

#pragma pack(1)

typedef struct {
    unsigned char srchState;
    unsigned int mstr;
    unsigned short mstrError;
    unsigned short mstrPilotPN;
    unsigned char numFingers;
} finger_v2_t;

typedef struct {
    unsigned short pilotPN;
    unsigned int rtcOffset;
    unsigned short C2I;
    unsigned char lock:1;
	unsigned char antenna:1;
	unsigned char diversity:1;
	unsigned char fingerIndex:4;
    unsigned char index;
    unsigned short antenna0C2I;
    unsigned short antenna1C2I;
} finger_t;

#pragma pack()

int log_data_decode_0x108a(int log_code, item_t *item)
{
    int rc;
    log_item_header_t *header = (log_item_header_t*)item->buf;
    if (header->log_code != log_code) {
        return -1;
    }

	unsigned char *data = hdlc_decode(header, item);

	finger_v2_t *finger_list = (finger_v2_t*)data;

	int count = finger_list->numFingers;
	LOG_MSG("[I] log_code: 0x%04x fingers: %d", header->log_code, count);

    /* fill decode result */
    unsigned char *res = (unsigned char*)DIAG_MALLOC(2048);
    if (res == NULL) {
        LOG_MSG("[E] malloc failed");
        rc = 0;
        goto err_exit;
    }

    memset(res, 0, 2048);
    res_header_t *res_header = (res_header_t*)res;
    res_header->length = RES_HEADER_SIZE;
    res_header->id = DIAG_HTONS(RES_0X108A_ID);
    cdma_timestamp_decode(header->timestamp, &res_header->timestamp);

	res_header->timestamp.tv_sec = DIAG_HTONL(res_header->timestamp.tv_sec);
	res_header->timestamp.tv_usec = DIAG_HTONL(res_header->timestamp.tv_usec);

    res_0x108a_t *res_0x108a = (res_0x108a_t*)(res_header +1);
    res_header->length += RES_0X108A_SIZE;

    //res_0x108a_ecio_t *res_ecio = (res_0x108a_ecio_t*)(res_0x108a +1);

	short pn = -1;
	short ecio = 0;

    /* begin decode */
	finger_t *finger = (finger_t*)(finger_list+1);
	while (count--) {
		if (finger->pilotPN == 0xFFFF || finger->pilotPN == 0) {
			finger ++;
			continue;
		}

		if (finger->lock) {
			pn = finger->pilotPN;
			float fecio = 10.0 * log10f(finger->C2I/512.0f);

			ecio = (short)(fecio * 100);
			res_0x108a->pn = DIAG_HTONS(pn);
			res_0x108a->ecio = DIAG_HTONS(ecio);

            //res_0x108a->count ++;
            //res_ecio->pn = DIAG_HTONS(finger->pilotPN);
            //res_ecio->c2i = DIAG_HTONS(finger->C2I);
            //res_ecio ++;

			LOG_MSG("[I] log_code: 0x%04x PN: %d RSSI: %d Ec/Io: %.2f dB",
				header->log_code, finger->pilotPN, finger->C2I, fecio);
		}

		finger ++;
	}

	if (pn == -1) {
		LOG_MSG("[W] no avalid finger info");
        DIAG_FREE(res);
		rc = 0;
		goto err_exit;
	}

#if 0
    if (res_0x108a->count == 0) {
        LOG_MSG("[W] no avalid pn info");
        DIAG_FREE(res);
        rc = 0;
        goto err_exit;
    }

    res_header->length += res_0x108a->count * RES_0X108A_ECIO_SIZE;
	res_0x108a->count = DIAG_HTONS(res_0x108a->count);

#endif

    /* raw log data */
    res_rawlog_t *rawdata = (res_rawlog_t*)(res + res_header->length);
    rawdata->length = DIAG_HTONL(item->bufLen);
    if (2048 - res_header->length < (item->bufLen + 4)) {
        LOG_MSG("[E] res buff not enough");
        DIAG_FREE(res);
        rc = 0;
        goto err_exit;
    }

    res_header->length += RES_RAWLOG_SIZE;
    memcpy(res+res_header->length, item->buf, item->bufLen);
    res_header->length += item->bufLen;

    /* end of decode */
    item->res = res;
    item->resLen = res_header->length;

	rc = res_header->length;
	res_header->length = DIAG_HTONL(res_header->length);

err_exit:

	if (item->escape) {
		DIAG_FREE(data);
	}

/*
    LOG_MSG("[D] log_code: 0x%04x res dump", header->log_code);
    int x;
    for (x = 0; x < res_header->length; x++) {
        printf("%02x ", res[x]);
        if ((x+1)%16==0) printf("\n");
    }
    printf("\n");
*/

    return rc;
}

