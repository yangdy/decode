/*
Copoyright (C) 2013, yangdy <yangdy AT outlook.com>.
All rights reserved.
*/

#ifndef _DIAG_DECODE_RESULT_H_
#define _DIAG_DECODE_RESULT_H_

#define RES_0X108A_ID 0x008A
#define RES_0X108B_ID 0x008B
#define RES_0X1069_ID 0x0069
#define RES_0X1080_ID 0x0080
#define RES_0X102D_ID 0x002D
#define RES_0X119A_ID 0x019A

#pragma pack(1)

typedef struct {
    unsigned int length;
    unsigned char data[0];
} res_t;

typedef struct {
    unsigned int length;
    unsigned short id;
    struct timeval timestamp;
} res_header_t;

#define RES_HEADER_SIZE sizeof(res_header_t)

typedef struct {
    unsigned int length;
    unsigned char data[0]; // length
} res_rawlog_t;

#define RES_RAWLOG_SIZE sizeof(res_rawlog_t)

/* 0x108a */
typedef struct {
    unsigned short pn;
	unsigned short c2i;
} res_0x108a_ecio_t;

#define RES_0X108A_ECIO_SIZE sizeof(res_0x108a_ecio_t)

typedef struct {
	short pn;
	short ecio; // *100;
} res_0x108a_t;

#define RES_0X108A_SIZE sizeof(res_0x108a_t)

/* 0x108b */
typedef struct {
    unsigned short pilotPN;
    unsigned short piloeEng;
    unsigned short field1;
    unsigned short field2;
    unsigned short field3;
} res_0x108b_pn_t;

#define RES_0X108B_PN_SIZE sizeof(res_0x108b_pn_t)

typedef struct {
    unsigned char aSETCount;
    unsigned char cSETCount;
    unsigned char nSETCount;
    res_0x108b_pn_t pns[0];
} res_0x108b_t;

/* 0x1069 */
typedef struct {
    short txTotalPower;
    short rxAGC0;
    short rxAGC1;
} res_0x1069_power_t;

#define RES_0X1069_POWER_SIZE sizeof(res_0x1069_power_t)

typedef struct {
//    short txTotalPower;
    res_0x1069_power_t power[2];
} res_0x1069_t;

#define RES_0X1069_SIZE sizeof(res_0x1069_t)

/* 0x1080 */
typedef struct {
    unsigned short pn;
} res_0x1080_t;

typedef struct {
	short rxPower;
	short txPower;
	short pn;
	short ecio;
} res_0x102d_t;

#define RES_0X102D_SIZE sizeof(res_0x102d_t)

typedef struct {
    short rxPower;
    short txPower;
    short pn;
    short ecio;
} res_0x119a_t;

#define RES_0X119A_SIZE sizeof(res_0x119a_t)

#pragma pack()

#endif /* _DIAG_DECODE_RESULT_H_ */
