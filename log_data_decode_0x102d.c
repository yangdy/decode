/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#include "eQxdm.h"
#include "log_data_decode.h"

#include <math.h>

#pragma pack(1)

typedef struct {
    unsigned char pilot_type_info;
    unsigned char band_class;
    unsigned char pregain;
    unsigned char integration;
    unsigned char non_coherent;
    unsigned short pilot_offset;
    unsigned int window_position;
    unsigned short window_size;
    unsigned char rx_agc;
    unsigned char tx_agc;
    unsigned char tx_gain_adj;
    unsigned char tx_pwr_limit;
    unsigned char srch_state;
    unsigned short for_ch_rc;
    unsigned char chan_set_mask;
    unsigned char num_paths;
    unsigned char num_fings;
} searcher_and_finger_t;

typedef struct {
    unsigned short path_position;
    unsigned short path_energy;
} path_info_t;

typedef struct {
    unsigned short pilot;
    unsigned char sector;
    unsigned char rssi;
    unsigned int position;
    unsigned short status_mask;
} finger_t;

#pragma pack()

int log_data_decode_0x102d(int log_code, item_t *item)
{
	int i;

    log_item_header_t *header = (log_item_header_t*)item->buf;
    if (header->log_code != log_code) {
        return -1;
    }

	unsigned char *data = hdlc_decode(header, item);

	searcher_and_finger_t *search_fingers = (searcher_and_finger_t*)data;

	short rxPower = (short)(search_fingers->rx_agc/3 - 63.248);
	short txPower = 0;
    switch (search_fingers->band_class) {
    case 0:
    case 2:
    case 3:
    case 5:
    case 7:
    case 9:
        txPower = -52.25 + (search_fingers->tx_agc/3);
        break;
    case 1:
    case 4:
    case 6:
    case 8:
        txPower = -55.25 + (search_fingers->tx_agc/3);
        break;
    default:
        LOG_MSG("unsupported band_class: %d", search_fingers->band_class);
        return 0;
    }

	path_info_t *path_info = (path_info_t*)(search_fingers+1);
	for (i = 0; i < search_fingers->num_paths; i++) {
		float adj = 0.0f;
		adj = 20 * log10f((float)(search_fingers->pregain * search_fingers->integration))
			+ 10 * log10f((float)(search_fingers->non_coherent));
		float eng = -60.89f + (10 * log10f(path_info->path_energy) + 78.445f - adj);
		LOG_MSG("path_position: %d path_energy: %d adj=%.2f energy: %d dB",
			path_info->path_position,
			path_info->path_energy,
			adj,
			(int)eng);

		path_info ++;
	}

	short pn = 0;
	float ecio = 0.0f;
	float ecioTotal = 0.0f;

	finger_t *finger = (finger_t*)(path_info+1);
	for (i = 0; i < search_fingers->num_fings; i++) {
		if (i == 0) {
			pn = finger->pilot;
		}

		ecio = 10 * log10f(finger->rssi) - 24.6f;

		LOG_MSG("PN: 0x%04d Ec/Io: %d dB", finger->pilot, (int)ecio);

		ecioTotal += pow(10.0, ecio/10);

		finger++;
	}

	ecioTotal = 10 * log10(ecioTotal);

	LOG_MSG("PN: %d Ec/Io Total: %d", pn, (int)ecioTotal);

    /* fill decode result */
    unsigned char *res = (unsigned char*)DIAG_MALLOC(2048);
    if (res == NULL) {
        LOG_MSG("[E] malloc failed");
        return 0;
    }

    memset(res, 0, 2048);
    res_header_t *res_header = (res_header_t*)res;
    res_header->length = RES_HEADER_SIZE;
    res_header->id = DIAG_HTONS(RES_0X102D_ID);
    cdma_timestamp_decode(header->timestamp, &res_header->timestamp);

	res_header->timestamp.tv_sec = DIAG_HTONL(res_header->timestamp.tv_sec);
	res_header->timestamp.tv_usec = DIAG_HTONL(res_header->timestamp.tv_usec);

	/* fill res */
	res_0x102d_t *res_0x102d = (res_0x102d_t*)(res_header+1);
	res_0x102d->pn = DIAG_HTONS(pn);
	res_0x102d->rxPower = DIAG_HTONS(rxPower);
	res_0x102d->txPower = DIAG_HTONS(txPower);
	res_0x102d->ecio = DIAG_HTONS(ecio);

	res_header->length += RES_0X102D_SIZE;

    /* raw log data */
    res_rawlog_t *rawlog = (res_rawlog_t*)(res_0x102d+1);
    rawlog->length = DIAG_HTONS(item->bufLen);
    res_header->length += RES_RAWLOG_SIZE;

	/* copy raw log adat */
	memcpy(res+res_header->length, item->buf, item->bufLen);
    res_header->length += item->bufLen;

    /* end of decode */
    item->res = res;
    item->resLen = res_header->length;

	int rc = res_header->length;

	res_header->length = DIAG_HTONL(res_header->length);

	if (item->escape) {
		DIAG_FREE(data);
	}

	return rc;
}

