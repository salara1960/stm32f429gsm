/*
 * mod.c
 *
 *  Created on: Aug 15, 2024
 *      Author: alarm
 */

#include "mod.h"


const ats_t at_cmd[GSM_CMD_MAX] = {//INIT
		{"AT","OK"},
		{"ATE0","OK"},
		{"ATI","OK"},//Manufactrer: ....  Model: ..... Revision: .....  IMEI: .....  OK
		{"AT+CMEE=1","OK"},
		{"AT+CPIN?","OK"},//+CPIN: READY
		{"AT+CMGF=1","OK"},
		{"AT+CNMI=1,2,0,1,0","OK"},
		{"AT+GMR","OK"},//PR3.02-MODEM_AT-terminal, 2022/09/06 03:58
		{"AT+CGSN","OK"},//864369032292264
		{"AT+CSQ","OK"},//+CSQ: 14,0
		{"AT+CREG?","OK"},//+CREG: 0,1
		{"AT+CCID", "OK"},
		{"AT+MFSTM","OK"},
		{"AT+CSTT?","OK"},//+CSTT: "IP","internet","",""
		{"AT+CIICR","OK"},
		{"AT+CGDATA=\"M-MBIM\",1,1","OK"},
		{"AT+CIPMUX=0","OK"},
		{"AT+CIPSTART=","OK"},//"AT+CIPSTART=\"TCP\",\"194.58.79.41\",\"28344\"\r\n"
		{"AT+CIPSEND",">"},
		{"AT+CIPCLOSE","OK"},
		{"AT+CIPSHUT","OK"},
		{"AT+CUSD=1,","OK"} // "#102#" + eol | +CUSD: 0, " Vash balans 200.00 r.", 15
};
//{\"protocol\":\"service_information\",\"lsncmd\":\"00\",\"sourceDeviceId\":\"2-07-77-0-8\",\"payload\":\"$LSNCMD,00,00,00,00,00*37\"}

const char *gsmEvent[GSM_EVENT_MAX] = {
		"+CFUN: ",  //1
		"+CPIN: ",  //READY
		"+CSQ: ",   //14,0
		"+CREG: ",  //0,1
		"+CGMR: ",  //PR3.02-MODEM_AT-terminal, 2022/09/06 03:58
		"+CNTP: ",  //1
		"+CCLK: ",  //"21/11/01,12:49:31+02"
		"+CMEE: ",  //1
		"+CUSD: ",  //+CUSD: 0, "003200300030002E003000300020 .... 340023", 72
		"+CMT: ",
		"STATE: ",  //"IP INITIAL","IP START","IP GPRSACT","IP STATUS"
		"CONNECT OK",
		"CONNECT FAIL",
		">",
		"CLOSE",
		"CLOSE OK",
		"SHUT OK",
		"SEND OK",
		"ERROR",
		"OK",
		"TCPIP OK",
		"+EIND:",   //1 ... 128
		"+RECEIVE," //9 bytes
};

const char *gsmState[GSM_STATE_MAX] = {
		"IP INITIAL",
		"IP START",
		"IP GPRSACT",
		"IP STATUS",
		"TCP CLOSED",
		"???"
};

const int8_t dBmRSSI[GSM_RSSI_MAX] = {
	-113,-111,-109,-107,-105,-103,-101,-99,
	-97 ,-95 ,-93 ,-91 ,-89 ,-87 ,-85 ,-83,
	-81 ,-79 ,-77 ,-75 ,-73 ,-71 ,-69 ,-67,
	-65 ,-63 ,-61 ,-59 ,-57 ,-55 ,-53 ,-51
};

int cmdID = -1;
int evtID = -1;
int iNone = -1;


//-------------------------------------------------------------------------------------------

int8_t getCmdInd(const char *at)
{
int8_t i = GSM_CMD_MAX;

	while (--i >= 0) {
		if (strstr(at, at_cmd[i].cmd)) return i;
	}

	return i;
}

//-------------------------------------------------------------------------------------------



