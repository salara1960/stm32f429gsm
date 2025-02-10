/*
 * mod.h
 *
 *  Created on: Aug 15, 2024
 *      Author: alarm
 */

#ifndef INC_MOD_H_
#define INC_MOD_H_


#include "main.h"

#define CMD_LEN 40
#define ACK_LEN 32

#define GSM_CMD_MAX    22 // количество команд в цепочке команд
#define GSM_EVENT_MAX  30 // количество сообщений от модуля GSM, подлежащих обработке
#define GSM_STATE_MAX   6 // количество различных значений у сообщения 'STATUS'
#define GSM_RSSI_MAX   32

enum {
	iAT = 0,
	iATE,
	iATI,
	iCMEE,
	iCPIN,
	iCMGF,
	iCNMI,
	iGMR,
	iCGSN,
	iCSQ,
	iCREG,
	iCCID,
	iMFSTM,
	iCSTT,
	iCIICR,
	iCGDATA,
	iCIPMUX,
	iCIPSTART,
	iCIPSEND,
	iCIPCLOSE,
	iCIPSHUT,
	iCUSD
};
enum {//+....
	mCFUN,
	mCPIN,
	mCSQ,
	mCREG,
	mCGMR,
	mCNTP,
	mCCLK,
	mCMEE,
	mCUSD,
	mCMT,
	mSTATE,
	mCON_OK,
	mCON_FAIL,
	mPROMPT,
	mCLOSE,
	mCLOSE_OK,
	mSHUT_OK,
	mSEND_OK,
	mERROR,
	mOK,
	mTCPIP_OK,
	mEIND,
	mRECV
};

#pragma pack(push,1)
typedef struct {
	char cmd[CMD_LEN];
	char ack[ACK_LEN];
} ats_t;
#pragma pack(pop)



int8_t getCmdInd(const char *at);




#endif /* INC_MOD_H_ */
