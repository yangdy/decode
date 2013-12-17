/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#pragma pack(1)

typedef struct {
	unsigned char bitFlags;
	short txOpenLoopPower;
	short txClosedLoopPower;
	short txPilotPower;
	short txTotalPower;
	short rxAGC0;
	short rxAGC1;
} power_t;

#pragma pack()

int log_data_decode_0x1069(int log_code, item_t *item)
{
    int rc;
    log_item_header_t *header = (log_item_header_t*)item->buf;
    if (header->log_code != log_code) {
        return -1;
    }

	unsigned char *data = hdlc_decode(header, item);

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
    res_header->id = DIAG_HTONS(RES_0X1069_ID);
    cdma_timestamp_decode(header->timestamp, &res_header->timestamp);

	res_header->timestamp.tv_sec = DIAG_HTONL(res_header->timestamp.tv_sec);
	res_header->timestamp.tv_usec = DIAG_HTONL(res_header->timestamp.tv_usec);

    res_0x1069_t *res_0x1069 = (res_0x1069_t*)(res_header+1);

	int txTotalPower = 0;

	power_t* power = (power_t*)data;
	LOG_MSG("[I] log_code: 0x%04x 0 TxTotalPower: %.2f dBm RxAGC0: %.2f dBm RxAGC1: %.2f dBm",
		header->log_code,
		power->txTotalPower/256.0f, power->rxAGC0/256.0f, power->rxAGC1/256.0f);

    res_0x1069->power[0].txTotalPower = DIAG_HTONS(power->txTotalPower);
    res_0x1069->power[0].rxAGC0 = DIAG_HTONS(power->rxAGC0);
    res_0x1069->power[0].rxAGC1 = DIAG_HTONS(power->rxAGC1);

	txTotalPower += power->txTotalPower;

	power ++;
	LOG_MSG("[I] log_code: 0x%04x 1 TxTotalPower: %.2f dBm RxAGC0: %.2f dBm RxAGC1: %.2f dBm",
		header->log_code,
		power->txTotalPower/256.0f, power->rxAGC0/256.0f, power->rxAGC1/256.0f);

    res_0x1069->power[1].txTotalPower = DIAG_HTONS(power->txTotalPower);
    res_0x1069->power[1].rxAGC0 = DIAG_HTONS(power->rxAGC0);
    res_0x1069->power[1].rxAGC1 = DIAG_HTONS(power->rxAGC1);

	//txTotalPower += power->txTotalPower;
	//txTotalPower /= 2;

    //LOG_MSG("[I] log_code: 0x%04x TxTotalPower: %d dBm", header->log_code, txTotalPower/256);

    //res_0x1069->txTotalPower = DIAG_HTONS(txTotalPower);
    res_header->length += RES_0X1069_SIZE;

    /* raw log data */
    res_rawlog_t *rawlog = (res_rawlog_t*)(res_0x1069+1);
    rawlog->length = DIAG_HTONL(item->bufLen);
    res_header->length += RES_RAWLOG_SIZE;

	/* copy raw log adat */
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

