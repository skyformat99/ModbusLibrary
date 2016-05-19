/*
 * mbs.c
 *
 *  Created on: Nov 7, 2013
 *      Author: hyang
 */
/*************************************************************************
**Copyright (c) 2013 Trojan Technologies Inc. 
**All rights reserved.
**
** File name:		mbs.c
** Descriptions:	process of modbus-RTU slave communication
**------------------------------------------------------------------------
** Version:			1.0
** Created by:		Hengguang 
** Created date:	2013-10-12
** Descriptions:	The original version
**------------------------------------------------------------------------
** Version:				
** Modified by:			
** Modified date:		
** Descriptions:		
**************************************************************************/
#define MBS_C

#include "common.h"
#include "mbs.h"


/*!
 * Constants which defines the format of a modbus frame. The example is
 * shown for a Modbus RTU/ASCII frame. Note that the Modbus PDU is not
 * dependent on the underlying transport.
 *
 * <code>
 * <------------------------ MODBUS SERIAL LINE PDU (1) ------------------->
 *              <----------- MODBUS PDU (1') ---------------->
 *  +-----------+---------------+----------------------------+-------------+
 *  | Address   | Function Code | Data                       | CRC/LRC     |
 *  +-----------+---------------+----------------------------+-------------+
 *  |           |               |                                   |
 * (2)        (3/2')           (3')                                (4)
 *
 * (1)  ... MB_SER_PDU_SIZE_MAX = 256
 * (2)  ... MB_SER_PDU_ADDR_OFF = 0
 * (3)  ... MB_SER_PDU_PDU_OFF  = 1
 * (4)  ... MB_SER_PDU_SIZE_CRC = 2
 *
 * (1') ... MB_PDU_SIZE_MAX     = 253
 * (2') ... MB_PDU_FUNC_OFF     = 0
 * (3') ... MB_PDU_DATA_OFF     = 1
 * </code>
 */

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

#define MB_PDU_SIZE_MAX     253 /*!< Maximum size of a PDU. */
#define MB_PDU_SIZE_MIN     1   /*!< Function Code */
#define MB_PDU_FUNC_OFF     0   /*!< Offset of function code in PDU. */
#define MB_PDU_DATA_OFF     1   /*!< Offset for response data in PDU. */

//modbus device address related defines
#define MB_ID_BROAD	       ( 0 )   /*! Modbus broadcasting address. */
#define MB_ID_MIN          ( 1 )   /*! Smallest possible slave address. */
#define MB_ID_MAX          ( 247 ) /*! Biggest possible slave address. */
#define MB_ID_UNIVE	       ( 255 ) /*! Universal device address  */


//Modbus exception codes define
#define MBE_UNKNOW_ERR			0
#define MBE_ILLEGAL_FUNCTION	1
#define MBE_ILLEGAL_DATA_ADDR	2
#define MBE_ILLEGAL_DATA_VALUE	3
#define MBE_SLAVE_DEVICE_FAIL	4
#define MBE_ACKNOWLEDGE			5
#define MBE_SLAVE_DEVICE_BUSY	6

//max data quantity about Modbus access at one time
#define MAX_READ_COIL			2000
#define MAX_READ_REG			125
#define MAX_FORCE_MULTI_COIL	1968
#define MAX_PRESET_MULTI_REG	123


//Modbus standard function code define
#define	READ_COIL			0x01
#define READ_INPUT_DIS		0x02
#define READ_HOLD_REG		0x03
#define READ_INPUT_REG		0x04
#define FORCE_SINGLE_COIL	0x05
#define PRESET_SINGLE_REG	0x06
#define FORCE_MULTI_COIL	0x0E
#define PRESET_MULTI_REG	0x10
//user defined function code. (valid range: 0x41~0x48, 0x64~0x6E)
#define GET_INNER_VER		0x41
#define GET_DEV_INFO		0x42
#define GET_STAT_INFO		0x43
#define SYS_DIAGN			0x44
#define GET_ERR				0x45
#define CLR_ERR				0x46


/********************extern variable declaration**************************/


/********************extern function declaration**************************/
extern  uint32 	GetInterval(uint32 prevTime); //unit: 1ms


/*********************variable declaration********************************/


/********************function declaration*********************************/
static void 	MbsRcvSrv(MB_RTU_SLV *pMbs);
static void 	MbsSndSrv(MB_RTU_SLV *pMbs);
static void 	MbsRsp(MB_RTU_SLV *pMbs);
static void 	MbsCorrectRsp(MB_RTU_SLV *pMbs);
static void 	MbsErrRsp(MB_RTU_SLV *pMbs);
static void 	ReadHoldReg(MB_RTU_SLV *pMbs);
static void 	PresetSingleReg(MB_RTU_SLV *pMbs);
static void 	PresetMultiReg(MB_RTU_SLV *pMbs);
static void 	GetInnerVer(MB_RTU_SLV *pMbs);


/*************************************************************************
Function Name :	MbsInit
Arguments 	  : [out]pMbs: pointer to the modbus control structure to be initialized
				[in]baud: baud rate(2400-115200 bps)
				[in]parity: parity setting, ('N' 'E' or 'O')
				[in]devAddr: address of slave device, (1-247) 
				[in]pReg: pointer to the modbus registers
				[in]regSize: size of modbus register area (word)
				[in]regBase: register base address for modbus access
				[in]rwBase: RW register base address for modbus access
				[in]rwSize: size of RW register area (word)
				[in]sndBuf: pointer of send buffer provided to modbus communication
				[in]sbufSize: size of send buffer
				[in]funcPortInit: pointer to function of 'Port Initialization'
				[in]funcGetRcvCnt: pointer to function of 'Get Receive data Count'
				[in]funcGetLastRcvTime: pointer to function of 'Get Time of Last Received data'
				[in]funcRcvFrm: pointer to function of 'get Received Frame out'
				[in]funcIsSndIdle: pointer to function of 'check if port Sending Is Idle'
				[in]funcSndFrm: pointer to function of 'Sending Frame'			
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Modbus-RTU slave initialization.
**************************************************************************/
uint8 MbsInit(MB_RTU_SLV *pMbs, uint32 baud, uint8 parity, uint8 devAddr, 
			  uint16 *pReg, uint16 regSize, uint16 regBase, uint16 rwBase, 
			  uint16 rwSize, uint8 *sndBuf, uint8 sbufSize,
			  BOOL8 	(*funcPortInit) (uint32, uint8, uint16), 
			  uint16	(*funcGetRcvCnt) (void),
			  uint32 	(*funcGetLastRcvTime) (void),
			  uint16 	(*funcRcvFrm) (uint8*),
			  BOOL8 	(*funcIsSndIdle) (void),
			  BOOL8 	(*funcSndFrm) (uint8*, uint16))
{
	/*ASSERT((pMbs != NULL) && (pReg != NULL) && (regSize) && (sndBuf != NULL) && (sbufSize) 
		&& (funcPortInit != NULL) && (funcGetRcvCnt != NULL) && (funcGetLastRcvTime != NULL) 
		&& (funcRcvFrm != NULL) && (funcIsSndIdle != NULL) && (funcSndFrm != NULL));*/

	/*	initialize modubs register areas */
	if ( (rwBase < regBase) || ((rwBase + rwSize) > (regBase + regSize)) )
	{
		return FALSE; //invalid register area define
	}
	pMbs->pMbReg = pReg;
	pMbs->regSize = regSize;
	pMbs->regBaseAddr = regBase;
	pMbs->rwRegBaseAddr = rwBase;
	pMbs->rwRegSize = rwSize;

	/*	initialize variables for modbus receiving and sending */
	if ((devAddr < MB_ID_MIN) || (devAddr > MB_ID_MAX))
	{
		return FALSE; //invalid device address
	}
	if (sbufSize < MIN_SBUF_SIZE)
	{
		return FALSE; //size of send buffer is not enough
	}		
	pMbs->rtuSpace = (3500 * 11) / baud + 2; //unit: ms, 3500=3.5*1000(1ms/1s)
	pMbs->devID = devAddr;
	pMbs->rcvLen = 0;
	pMbs->sbuf = sndBuf;
	pMbs->sbufSize = sbufSize;
	pMbs->sndLen = 0;
	pMbs->errCode = MBE_UNKNOW_ERR;
	pMbs->rspType = NO_RSP;
	
	/*	initialize port supporting operation interface */
	pMbs->pfPortInit = funcPortInit;
	pMbs->pfGetRcvCnt = funcGetRcvCnt;
	pMbs->pfGetLastRcvTime = funcGetLastRcvTime;
	pMbs->pfRcvFrm = funcRcvFrm;
	pMbs->pfIsSndIdle = funcIsSndIdle;
	pMbs->pfSndFrm = funcSndFrm;
	
	/*	initialize real port */
	if (!(pMbs->pfPortInit(baud, parity, pMbs->rtuSpace)))
	{
		return FALSE; //port initialization failed.
	}

	return TRUE;
}


/*************************************************************************
Function Name :	MbsRsp
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : modbus slave response process.
**************************************************************************/
void MbsRsp(MB_RTU_SLV *pMbs)
{
	pMbs->rspType = NO_RSP;
	
	switch (pMbs->rbuf[1])
	{
		case  READ_HOLD_REG:
			ReadHoldReg(pMbs);
			break;
		case  PRESET_SINGLE_REG:
			PresetSingleReg(pMbs);
			break;
		case  PRESET_MULTI_REG:
			PresetMultiReg(pMbs);
			break;
		case  GET_INNER_VER:
			GetInnerVer(pMbs);
			break;
		case  GET_DEV_INFO:
			break;
		case  GET_STAT_INFO:
			break;
		case  GET_ERR:
			break;
		case  CLR_ERR:
			break;
		default:
			pMbs->errCode = MBE_ILLEGAL_FUNCTION; //unsupported function code
			pMbs->rspType = ERR_RSP;
			break;
	}
	
	/*	if broadcasting, no need to response */
	if (pMbs->rbuf[0] == MB_ID_BROAD)
	{
		pMbs->rspType = NO_RSP;
		pMbs->sndLen = 0;
		return;
	}

	switch (pMbs->rspType)
	{
		case  COR_RSP:
			MbsCorrectRsp(pMbs);
			break;
		case  ERR_RSP:
			MbsErrRsp(pMbs);
			break;
		case  NO_RSP:
			pMbs->sndLen = 0;
			break;
		default:
			break;
	}
}

/*************************************************************************
Function Name :	MbsCorrectRsp
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null 
Description	  : Modbus slave correct response.
**************************************************************************/
void MbsCorrectRsp(MB_RTU_SLV *pMbs)
{
	uint16	crcValue = 0;
	uint8	*pch = pMbs->sbuf;

	crcValue = CRC16(pch, pMbs->sndLen);

	pch += pMbs->sndLen;
	*pch++ = crcValue >> 8;
	*pch = crcValue & 0xff;
	pMbs->sndLen += 2;
}

/*************************************************************************
Function Name :	MbsErrRsp
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null 
Description	  : Modbus slave correct response.
**************************************************************************/
void MbsErrRsp(MB_RTU_SLV *pMbs)
{
	uint16	crcValue = 0;
	uint8	*pch = pMbs->sbuf;

	*pch++ = pMbs->rbuf[0]; //pMbs->devID;
	*pch++ = pMbs->rbuf[1] + 0x80;
	*pch++ = pMbs->errCode;
	crcValue = CRC16(pMbs->sbuf, 3);
	*pch++ = crcValue >> 8;
	*pch++ = crcValue & 0xFF;
	pMbs->sndLen = 5;
}

/*************************************************************************
Function Name :	ReadHoldReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]pReqMsg: head pointer of request message
				[in]reqMsgLen: length of request message
				[out]pRspType: pointer of response type 
				[in]pRspMsg: head pointer of buffer to save response message
				[out]pErrCode: pointer of exception code if exceptional response
Return		  : Null. 
Description	  : Read hold registers.
**************************************************************************/
void ReadHoldReg(MB_RTU_SLV *pMbs)
{
	uint16	stAddr = (pMbs->rbuf[2]<<8) + pMbs->rbuf[3];
	uint16	dataLen = (pMbs->rbuf[4]<<8) + pMbs->rbuf[5];
	uint8	*pch = pMbs->sbuf; //pMbs->sbuf + pMbs->sndLen;

	if (pMbs->rcvLen != 8) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (dataLen == 0) 
		|| (dataLen > MAX_READ_REG) ) //transnormal data quantity 
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if (stAddr < pMbs->regBaseAddr) //access slop over
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (stAddr + dataLen) > (pMbs->regBaseAddr + pMbs->regSize) )
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}

	memcpy(pch, pMbs->rbuf, 2);
	pch += 2;

	*pch++ = dataLen << 1;
	while (dataLen--)
	{
		*pch++ = pMbs->pMbReg[stAddr - pMbs->regBaseAddr] >> 8;
		*pch++ = pMbs->pMbReg[stAddr - pMbs->regBaseAddr] & 0xFF;
		stAddr++;
	}
	pMbs->rspType = COR_RSP;
	pMbs->sndLen = pch - pMbs->sbuf;
}

/*************************************************************************
Function Name :	PresetSingleReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null. 
Description	  : Preset single register.
**************************************************************************/
void PresetSingleReg(MB_RTU_SLV *pMbs)
{
	uint16	dataAddr = (pMbs->rbuf[2]<<8) + pMbs->rbuf[3];
	uint16	presetData = (pMbs->rbuf[4]<<8) + pMbs->rbuf[5];

	if (pMbs->rcvLen != 8) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (dataAddr < pMbs->rwRegBaseAddr) 
		|| (dataAddr >= (pMbs->rwRegBaseAddr + pMbs->rwRegSize)) )//access slop over
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}

	pMbs->pMbReg[dataAddr - pMbs->regBaseAddr] = presetData;
	memcpy(pMbs->sbuf, pMbs->rbuf, 6);
	pMbs->rspType = COR_RSP;
	pMbs->sndLen = 6;
}

/*************************************************************************
Function Name :	PresetMultiReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null 
Description	  : Preset multiple register.
**************************************************************************/
void PresetMultiReg(MB_RTU_SLV *pMbs)
{
	uint16	stAddr = (pMbs->rbuf[2]<<8) + pMbs->rbuf[3];
	uint16	dataLen = (pMbs->rbuf[4]<<8) + pMbs->rbuf[5];
	uint8	*pch;

	if (pMbs->rcvLen != 9 + pMbs->rbuf[6]) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (stAddr < pMbs->rwRegBaseAddr) 
		|| ((stAddr + dataLen) > (pMbs->rwRegBaseAddr + pMbs->rwRegSize)) )//access slop over
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}

	memcpy(pMbs->sbuf, pMbs->rbuf, 6);
	pch = pMbs->rbuf + 7;
	while (dataLen--)
	{
		pMbs->pMbReg[stAddr - pMbs->regBaseAddr] = *pch++ << 8;
		pMbs->pMbReg[stAddr - pMbs->regBaseAddr] += *pch++;
		stAddr++;
	}
	pMbs->rspType = COR_RSP;
	pMbs->sndLen = 6;
}

/*************************************************************************
Function Name :	GetInnerVer
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : Get inner firmware version of product.
**************************************************************************/
void GetInnerVer(MB_RTU_SLV *pMbs)
{
}


/*************************************************************************
Function Name :	ReadMbReg
Arguments 	  : [in]pMbs: pointer to the modbus object
				[in]index: data index in the modbus register array
Return		  : data to be read, if 'index' overrun, return 'FAIL'(0xFFFF) 
Description	  : read a single modbus register.
**************************************************************************/
uint16 ReadMbReg(MB_RTU_SLV *pMbs, uint8 index)
{
	if (index >= pMbs->regSize)
		return FAIL;
	
	return pMbs->pMbReg[index];
}

/*************************************************************************
Function Name :	WriteMbReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]index: data index in the modbus register array
				[in]data: data to be written in
Return		  : TRUE-success
				FALSE-failed, 'inedx' overrun 
Description	  : write a single modbus register.
**************************************************************************/
uint8 WriteMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 data)
{
	if (index >= pMbs->regSize)
		return FALSE;
	
	pMbs->pMbReg[index] = data;
	return TRUE;
}

/*************************************************************************
Function Name :	ReadMassMbReg
Arguments 	  : [in]pMbs: pointer to the modbus object
				[in]index: start data index in the modbus register array
				[out]pDes: desitination point to save data to be read
				[in]size: amount of words to be read
Return		  : If failed, return FALSE
				If successful, return 'TRUE'
Description	  : read a serial mass of modbus registers.
**************************************************************************/
uint8 ReadMassMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 *pDes, uint8 size)
{
	if ((index + size) >= pMbs->regSize)
		return FALSE;
	
	while (size--)
	{
		*pDes++ = pMbs->pMbReg[index++];
	}
	return TRUE;
}

/*************************************************************************
Function Name :	WriteMassMbReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]index: start data index in the modbus register array
				[in]pSrc: point to source data to be writen in
				[in]size: amount of words to be read
Return		  : If failed, return FALSE
				If successful, return 'TRUE'
Description	  : write a serial mass of modbus registers.
**************************************************************************/
uint8 WriteMassMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 *pSrc, uint8 size)
{
	if ((index + size) >= pMbs->regSize)
		return FALSE;
	
	while (size--)
	{
		pMbs->pMbReg[index++] = *pSrc++;
	}
	return TRUE;
}

/*************************************************************************
Function Name :	MbsRcvSrv
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : modbus-RTU slave receiving service.
**************************************************************************/
void MbsRcvSrv(MB_RTU_SLV *pMbs)
{
	uint8	rcvFrm[256];

	/* if receiving end (judged by free line time) */
	if ( (pMbs->pfGetRcvCnt() > 0) && (GetInterval(pMbs->pfGetLastRcvTime()) >= pMbs->rtuSpace) ) 
	{
		pMbs->rbuf = rcvFrm;
		pMbs->rcvLen = pMbs->pfRcvFrm(pMbs->rbuf);
		
		/* if size of frame not enough, ignore it. */
		if (pMbs->rcvLen < MB_SER_PDU_SIZE_MIN)
			return;

		/* device address match or broad-casting address or universal address */
		if ( (rcvFrm[0] == pMbs->devID) 
			|| (rcvFrm[0] == MB_ID_UNIVE)
			|| ((rcvFrm[0] == MB_ID_BROAD) && ((rcvFrm[1] == PRESET_SINGLE_REG) 
											   || (rcvFrm[1] == PRESET_MULTI_REG))) )
		{
			/* CRC check */
			if ( CRC16(rcvFrm, pMbs->rcvLen - 2) 
				== ((rcvFrm[pMbs->rcvLen - 2] << 8) + rcvFrm[pMbs->rcvLen - 1]) )
			{
				MbsRsp(pMbs);
			}
		}
	}
}

/*************************************************************************
Function Name :	MbsSndSrv
Arguments 	  : [in]pMbs: pointer to the modbus object
Return		  : Null
Description	  : modbus-RTU slave sending service.
**************************************************************************/
void MbsSndSrv(MB_RTU_SLV *pMbs)
{
	if (pMbs->sndLen)
	{
		if (pMbs->pfIsSndIdle())
		{
			if (pMbs->pfSndFrm(pMbs->sbuf, pMbs->sndLen))
			{
				pMbs->sndLen = 0;
			}			
		}
	}
}

/*************************************************************************
Function Name :	MbsSrv
Arguments 	  : [in]pMbs: pointer to the modbus object
Return		  : Null
Description	  : Main entrence of modbus-RTU slave service.
**************************************************************************/
void MbsSrv(MB_RTU_SLV *pMbs)
{
	MbsRcvSrv(pMbs); //analyze receiving
	MbsSndSrv(pMbs); //analyze sending
}


