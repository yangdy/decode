/*
Copyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#include <math.h>

#pragma pack(1)

typedef struct {
    unsigned char version;
    unsigned char numOfSubPackets;
    short reserved;
} srch_tng_finger_status_header_t;

typedef struct {
    unsigned char id;
    unsigned char version;
    unsigned short size;
    unsigned char rx0_band_class;
    unsigned char rx1_band_class;
    short rx0_cdma_ch;
    short rx1_cdma_ch;
    char rx0_agc;
    char rx1_agc;
    short tx_power;
    char tx_gain_adj;
    char tx_power_limit;
} rf_sub_packet_t;

typedef struct {
    unsigned char id;
    unsigned char version;
    unsigned short length;
    unsigned char pilotFilterGainFactor;
    unsigned char r1; // reserved
    unsigned char antennaConfig;
    unsigned char numOfFingers;
} finger_info_header_t;

typedef struct {
    unsigned short pilotPN;
    short rssi;
    short rx0_rssi;
    short rx1_rssi;
    int position;
	int enabledChannelMask;
	unsigned short refFinger:1;
	unsigned short assigned:1;
	unsigned short locked:1;
	unsigned short rx0_locked:1;
	unsigned short rx1_locked:1;
	unsigned short td_mode:2;
	unsigned short td_power:2;
	short reserved;
} finger_info_t;

typedef struct {
	short pn;
	float ecio;
} res_finger_info_t;

#pragma pack()

int log_data_decode_0x119a(int log_code, item_t *item)
{
    int rc, i;

    log_item_header_t *header = (log_item_header_t*)item->buf;
    if (header->log_code != log_code) {
        return -1;
    }

    //bdump(item->buf, item->bufLen);

	unsigned char *data = hdlc_decode(header, item);

    srch_tng_finger_status_header_t *status_header =
        (srch_tng_finger_status_header_t*)(header+1);

    if (status_header->numOfSubPackets < 2) {
        //LOG_MSG("[W] not avalid number of sub packets");
        rc = 0;
        goto err_exit;
    }

    unsigned char *p = (unsigned char *)(status_header +1);

    // RF (2)
    if (p[0] != 2) {
        //LOG_MSG("[W] first sub packet not RF(2)");
        rc = 0;
        goto err_exit;
    }

    rf_sub_packet_t *rf_pkt = (rf_sub_packet_t*)p;

    p = (unsigned char*)(rf_pkt+1);
    if (p[0] != 4) {
        //LOG_MSG("[W] second sub packet not Finger Info(4)");
        rc = 0;
        goto err_exit;
    }

    finger_info_header_t *finger_info_header = (finger_info_header_t*)(rf_pkt+1);
    if (finger_info_header->numOfFingers == 0) {
        //LOG_MSG("[W] invalid nuber of finger info");
        rc = 0;
        goto err_exit;
    }

/*
	res_finger_info_t *res_fi =
		(res_finger_info_t*)DIAG_MALLOC(sizeof(res_finger_info_t)*finger_info_header->numOfFingers);
	memset(res_fi, 0, sizeof(res_finger_info_t)*finger_info_header->numOfFingers);
	short *pnSet = (short*)DIAG_MALLOC(2*finger_info_header->numOfFingers);
	int pnIdx;
	int isExisit = 0;
*/

	short pn = -1;
	short rssi = 0;

	finger_info_t *finger_info = (finger_info_t*)(finger_info_header+1);
	for (i = 0; i < finger_info_header->numOfFingers; i++) {
		if (finger_info->pilotPN == 0xFFFF || finger_info->pilotPN == 0) {
			finger_info ++;
			continue;
		}

		if (finger_info->assigned && finger_info->locked && finger_info->refFinger) {
			pn = finger_info->pilotPN;
			rssi = finger_info->rssi;
		}
	}

	if (pn == -1) {
		LOG_MSG("[W] no avalid pn");
		rc = 0;
		goto err_exit;
	}

    float fRxPower = -63.248 + (rf_pkt->rx0_agc-256)*334/1000.0;
    short rxPower = (short)(fRxPower*100);
    short txPower = (short)(rf_pkt->tx_power);

    float fecio = 0.0f;
    switch (rf_pkt->rx0_band_class) {
    case 0:
        /*
            10 * log10(max[ 10^-5, rssi/18*64^2 - 1/64 * 1/8 / 2-1/8])
        */
        fecio = rssi/73728.0f - 0.001041667f;
		if (fecio < 0.0001) {
			fecio = 0.0001;
		}
		fecio = 10 * log10(fecio);
        break;
    case 1:
        /*
            10 * log10(max[ 10^-5, rssi/18*64^2 - 1/64 * 1/4 / 2-1/4])
        */
        fecio = rssi/73728.0f - 0.002232143f;
		if (fecio < 0.0001) {
			fecio = 0.0001;
		}
        fecio = 10 * log10(fecio);
        break;
    }

    short ecio = (short)(fecio*100);

    LOG_MSG("log_code: 0x%04x PN: %d RxPower: %.2f dbm TxPower: %.2f dBm Ec/Io: %.2f",
        header->log_code, pn, fRxPower, (float)(txPower/10.0), fecio);

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
    res_header->id = DIAG_HTONS(RES_0X119A_ID);
    cdma_timestamp_decode(header->timestamp, &res_header->timestamp);

	res_header->timestamp.tv_sec = DIAG_HTONL(res_header->timestamp.tv_sec);
	res_header->timestamp.tv_usec = DIAG_HTONL(res_header->timestamp.tv_usec);

	/* fill res */
	res_0x119a_t *res_0x119a = (res_0x119a_t*)(res_header+1);
	res_0x119a->pn = DIAG_HTONS(pn);
	res_0x119a->rxPower = DIAG_HTONS(rxPower);
	res_0x119a->txPower = DIAG_HTONS(txPower);
	res_0x119a->ecio = DIAG_HTONS(ecio);

	res_header->length += RES_0X119A_SIZE;

    /* raw log data */
    res_rawlog_t *rawlog = (res_rawlog_t*)(res_0x119a+1);
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

	return rc;
}

