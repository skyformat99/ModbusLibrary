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
#define __MBS_C

#include "common.h"
#include "mbs.h"


#define LIB_VER			0x00000200 //library version, only low word be useful, BCD format, V**.**
#define LIB_CMPL_TIME	0x20160519 //library compiled time, BCD format, yyyy.mm.dd

/*************************extern variable declaration*************************/


/*************************extern function declaration*************************/
extern  uint32 	GetInterval(uint32 prevTime); //unit: ms


/****************************variable declaration*****************************/


/****************************function declaration*****************************/
static void 	MbsRcvSrv(MB_RTU_SLV *pMbs);
static void 	MbsSndSrv(MB_RTU_SLV *pMbs);
static void 	MbsRsp(MB_RTU_SLV *pMbs);
static void 	MbsCorrectRsp(MB_RTU_SLV *pMbs);
static void 	MbsErrRsp(MB_RTU_SLV *pMbs);
static void 	ReadHoldReg(MB_RTU_SLV *pMbs);
static void 	WriteSingleReg(MB_RTU_SLV *pMbs);
static void 	WriteMultiReg(MB_RTU_SLV *pMbs);
static void 	GetInnerVer(MB_RTU_SLV *pMbs);
static void 	GetLibVer(MB_RTU_SLV *pMbs);


/******************************************************************************
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
******************************************************************************/
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
	ASSERT((pMbs != NULL) && (pReg != NULL) && (regSize) && (sndBuf != NULL) && (sbufSize) 
		&& (funcPortInit != NULL) && (funcGetRcvCnt != NULL) && (funcGetLastRcvTime != NULL) 
		&& (funcRcvFrm != NULL) && (funcIsSndIdle != NULL) && (funcSndFrm != NULL));

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
	if (baud > 19200)
	{
		pMbs->rtuSpace = 3; //unit: ms, 1750(us) / 1000(us/ms) + 2 = 3
	}
	else
	{
		pMbs->rtuSpace = (3500 * 11) / baud + 2; //unit: ms, 3500=3.5*1000(ms/s)
	}
	pMbs->devID = devAddr;
	pMbs->rcvSize = 0;
	pMbs->sbuf = sndBuf;
	pMbs->sbufSize = sbufSize;
	pMbs->sndSize = 0;
	pMbs->errCode = MBE_UNKNOW_ERR;
	pMbs->rspType = NO_RSP;
	
	/*	initialize port supporting operation interface */
	pMbs->PortInit = funcPortInit;
	pMbs->GetRcvCnt = funcGetRcvCnt;
	pMbs->GetLastRcvTime = funcGetLastRcvTime;
	pMbs->RcvFrm = funcRcvFrm;
	pMbs->IsSndIdle = funcIsSndIdle;
	pMbs->SndFrm = funcSndFrm;
	pMbs->pFuncHandler = NULL;
	
	/*	initialize real port */
	if (!(pMbs->PortInit(baud, parity, pMbs->rtuSpace)))
	{
		return FALSE; //port initialization failed.
	}

	return TRUE;
}


/******************************************************************************
Function Name :	MbsRsp
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : modbus slave response process.
******************************************************************************/
void MbsRsp(MB_RTU_SLV *pMbs)
{
	pMbs->rspType = NO_RSP;
	
	switch (pMbs->rbuf[1])
	{
		case  MBC_READ_HOLD_REG:
			ReadHoldReg(pMbs);
			break;
		case  MBC_WRITE_SINGLE_REG:
			WriteSingleReg(pMbs);
			break;
		case  MBC_WRITE_MULTI_REG:
			WriteMultiReg(pMbs);
			break;
		case  MBC_GET_INNER_VER:
			GetInnerVer(pMbs);
			break;
		case  MBC_GET_LIB_VER:
			GetLibVer(pMbs);
			break;
		case  MBC_GET_DEV_INFO:
			break;
		case  MBC_GET_STAT_INFO:
			break;
		case  MBC_GET_ERR:
			break;
		case  MBC_CLR_ERR:
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
		pMbs->sndSize = 0;
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
			pMbs->sndSize = 0;
			break;
		default:
			break;
	}
}

/******************************************************************************
Function Name :	MbsCorrectRsp
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null 
Description	  : Modbus slave correct response.
******************************************************************************/
void MbsCorrectRsp(MB_RTU_SLV *pMbs)
{
	uint16	crcValue = 0;
	uint8	*pch = pMbs->sbuf;

	crcValue = CRC16(pch, pMbs->sndSize);

	pch += pMbs->sndSize;
	*pch++ = crcValue >> 8;
	*pch = crcValue & 0xff;
	pMbs->sndSize += 2;
}

/******************************************************************************
Function Name :	MbsErrRsp
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null 
Description	  : Modbus slave correct response.
******************************************************************************/
void MbsErrRsp(MB_RTU_SLV *pMbs)
{
	uint16	crcValue = 0;
	uint8	*pch = pMbs->sbuf;

	*pch++ = pMbs->rbuf[0]; //pMbs->devID;
	*pch++ = pMbs->rbuf[1] | 0x80;
	*pch++ = pMbs->errCode;
	crcValue = CRC16(pMbs->sbuf, 3);
	*pch++ = crcValue >> 8;
	*pch++ = crcValue & 0xFF;
	pMbs->sndSize = 5;
}

/******************************************************************************
Function Name :	ReadHoldReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]pReqMsg: head pointer of request message
				[in]reqMsgLen: length of request message
				[out]pRspType: pointer of response type 
				[in]pRspMsg: head pointer of buffer to save response message
				[out]pErrCode: pointer of exception code if exceptional response
Return		  : Null. 
Description	  : Read hold registers.
******************************************************************************/
void ReadHoldReg(MB_RTU_SLV *pMbs)
{
	uint16	stAddr = (pMbs->rbuf[2]<<8) + pMbs->rbuf[3];
	uint16	dataLen = (pMbs->rbuf[4]<<8) + pMbs->rbuf[5];
	uint8	*pch = pMbs->sbuf; //pMbs->sbuf + pMbs->sndLen;
	uint16	stAddrBak;
	uint16	dataLenBak;

	if (pMbs->rcvSize != 8) //length of frame error
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
		pMbs->errCode = MBE_ILLEGAL_DATA_ADDR;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (stAddr + dataLen) > (pMbs->regBaseAddr + pMbs->regSize) )
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_ADDR;
		pMbs->rspType = ERR_RSP;
		return;
	}

	if ( (pMbs->pFuncHandler != NULL)
		&& (pMbs->pFuncHandler->PreRdRegHandler != NULL))
	{
		pMbs->errCode = pMbs->pFuncHandler->PreRdRegHandler(pMbs->sbuf + 3, stAddr, dataLen);
		if (pMbs->errCode != MBE_NO_ERR)
		{
			pMbs->rspType = ERR_RSP;
			return;
		}
	}

	stAddrBak = stAddr;
	dataLenBak = dataLen;
	
	pch = CopyDataWithDesPtrMove(pch, pMbs->rbuf, 2);
	*pch++ = dataLen << 1;
	while (dataLen--)
	{
		*pch++ = pMbs->pMbReg[stAddr - pMbs->regBaseAddr] >> 8;
		*pch++ = pMbs->pMbReg[stAddr - pMbs->regBaseAddr] & 0xFF;
		stAddr++;
	}
	pMbs->rspType = COR_RSP;
	pMbs->sndSize = pch - pMbs->sbuf;
	
	if ( (pMbs->pFuncHandler != NULL)
		&& (pMbs->pFuncHandler->PostRdRegHandler != NULL))
	{
		pMbs->pFuncHandler->PostRdRegHandler(stAddrBak, dataLenBak);
	}
}

/******************************************************************************
Function Name :	WriteSingleReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null. 
Description	  : Preset single register.
******************************************************************************/
void WriteSingleReg(MB_RTU_SLV *pMbs)
{
	uint16	dataAddr = (pMbs->rbuf[2]<<8) + pMbs->rbuf[3];
	uint16	presetData = (pMbs->rbuf[4]<<8) + pMbs->rbuf[5];

	if (pMbs->rcvSize != 8) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (dataAddr < pMbs->rwRegBaseAddr) 
		|| (dataAddr >= (pMbs->rwRegBaseAddr + pMbs->rwRegSize)) )//access slop over
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_ADDR;
		pMbs->rspType = ERR_RSP;
		return;
	}

	if ( (pMbs->pFuncHandler != NULL)
		&& (pMbs->pFuncHandler->PreWtRegHandler != NULL))
	{
		pMbs->errCode = pMbs->pFuncHandler->PreWtRegHandler(pMbs->rbuf + 4, dataAddr, 1);
		if (pMbs->errCode != MBE_NO_ERR)
		{
			pMbs->rspType = ERR_RSP;
			return;
		}
	}
	
	pMbs->pMbReg[dataAddr - pMbs->regBaseAddr] = presetData;
	memcpy(pMbs->sbuf, pMbs->rbuf, 6);
	pMbs->rspType = COR_RSP;
	pMbs->sndSize = 6;

	if ( (pMbs->pFuncHandler != NULL)
		&& (pMbs->pFuncHandler->PostWtRegHandler != NULL))
	{
		pMbs->pFuncHandler->PostWtRegHandler(dataAddr, 1);
	}
}

/******************************************************************************
Function Name :	WriteMultiReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null 
Description	  : Preset multiple register.
******************************************************************************/
void WriteMultiReg(MB_RTU_SLV *pMbs)
{
	uint16	stAddr = (pMbs->rbuf[2]<<8) + pMbs->rbuf[3];
	uint16	dataLen = (pMbs->rbuf[4]<<8) + pMbs->rbuf[5];
	uint8	*pch;
	uint16	stAddrBak;
	uint16	dataLenBak;

	if (pMbs->rcvSize != 9 + pMbs->rbuf[6]) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (dataLen == 0) 
		|| (dataLen > MAX_WRITE_MULTI_REG) ) //transnormal data quantity 
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ((dataLen<<1) != pMbs->rbuf[6]) //data value not match
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_VALUE;
		pMbs->rspType = ERR_RSP;
		return;
	}
	if ( (stAddr < pMbs->rwRegBaseAddr) 
		|| ((stAddr + dataLen) > (pMbs->rwRegBaseAddr + pMbs->rwRegSize)) )//access slop over
	{
		pMbs->errCode = MBE_ILLEGAL_DATA_ADDR;
		pMbs->rspType = ERR_RSP;
		return;
	}

	if ( (pMbs->pFuncHandler != NULL)
		&& (pMbs->pFuncHandler->PreWtRegHandler != NULL))
	{
		pMbs->errCode = pMbs->pFuncHandler->PreWtRegHandler(pMbs->rbuf + 7, stAddr, dataLen);
		if (pMbs->errCode != MBE_NO_ERR)
		{
			pMbs->rspType = ERR_RSP;
			return;
		}
	}
	
	stAddrBak = stAddr;
	dataLenBak = dataLen;
	
	memcpy(pMbs->sbuf, pMbs->rbuf, 6);
	pch = pMbs->rbuf + 7;
	while (dataLen--)
	{
		pMbs->pMbReg[stAddr - pMbs->regBaseAddr] = *pch++ << 8;
		pMbs->pMbReg[stAddr - pMbs->regBaseAddr] += *pch++;
		stAddr++;
	}
	pMbs->rspType = COR_RSP;
	pMbs->sndSize = 6;

	if ( (pMbs->pFuncHandler != NULL)
		&& (pMbs->pFuncHandler->PostWtRegHandler != NULL))
	{
		pMbs->pFuncHandler->PostWtRegHandler(stAddrBak, dataLenBak);
	}
}

/******************************************************************************
Function Name :	GetInnerVer
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : Get inner firmware version of product.
******************************************************************************/
void GetInnerVer(MB_RTU_SLV *pMbs)
{
#if 0
	uint8	*pch = pMbs->sbuf;

	if (pMbs->rcvSize != 4) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}

	pch = CopyDataWithDesPtrMove(pch, pMbs->rbuf, 2);
	*pch++ = INNER_VER >> 24;
	*pch++ = (INNER_VER >> 16) & 0xFF;
	*pch++ = (INNER_VER >> 8) & 0xFF;
	*pch++ = INNER_VER & 0xFF;
	*pch++ = INNER_VER_TIME >> 24;
	*pch++ = (INNER_VER_TIME >> 16) & 0xFF;
	*pch++ = (INNER_VER_TIME >> 8) & 0xFF;
	*pch++ = INNER_VER_TIME & 0xFF;

	pMbs->rspType = COR_RSP;
	pMbs->sndSize = 10;
#endif
}


/******************************************************************************
Function Name :	GetLibVer
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : Get version of Modbus library itself.
******************************************************************************/
void GetLibVer(MB_RTU_SLV *pMbs)
{
	uint8	*pch = pMbs->sbuf;

	if (pMbs->rcvSize != 4) //length of frame error
	{
		pMbs->errCode = MBE_UNKNOW_ERR;
		pMbs->rspType = ERR_RSP;
		return;
	}

	pch = CopyDataWithDesPtrMove(pch, pMbs->rbuf, 2);
	*pch++ = LIB_VER >> 24;
	*pch++ = (LIB_VER >> 16) & 0xFF;
	*pch++ = (LIB_VER >> 8) & 0xFF;
	*pch++ = LIB_VER & 0xFF;
	*pch++ = LIB_CMPL_TIME >> 24;
	*pch++ = (LIB_CMPL_TIME >> 16) & 0xFF;
	*pch++ = (LIB_CMPL_TIME >> 8) & 0xFF;
	*pch++ = LIB_CMPL_TIME & 0xFF;

	pMbs->rspType = COR_RSP;
	pMbs->sndSize = 10;
}


/******************************************************************************
Function Name :	ReadMbReg
Arguments 	  : [in]pMbs: pointer to the modbus object
				[in]index: data index in the modbus register array
Return		  : data to be read, if 'index' overrun, return 'FAIL'(0xFFFF) 
Description	  : read a single modbus register.
******************************************************************************/
uint16 ReadMbReg(MB_RTU_SLV *pMbs, uint8 index)
{
	if (index >= pMbs->regSize)
		return FAIL;
	
	return pMbs->pMbReg[index];
}

/******************************************************************************
Function Name :	WriteMbReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]index: data index in the modbus register array
				[in]data: data to be written in
Return		  : TRUE-success
				FALSE-failed, 'inedx' overrun 
Description	  : write a single modbus register.
******************************************************************************/
uint8 WriteMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 data)
{
	if (index >= pMbs->regSize)
		return FALSE;
	
	pMbs->pMbReg[index] = data;
	return TRUE;
}

/******************************************************************************
Function Name :	ReadMassMbReg
Arguments 	  : [in]pMbs: pointer to the modbus object
				[in]index: start data index in the modbus register array
				[out]pDes: desitination point to save data to be read
				[in]size: amount of words to be read
Return		  : If failed, return FALSE
				If successful, return 'TRUE'
Description	  : read a serial mass of modbus registers.
******************************************************************************/
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

/******************************************************************************
Function Name :	WriteMassMbReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]index: start data index in the modbus register array
				[in]pSrc: point to source data to be writen in
				[in]size: amount of words to be read
Return		  : If failed, return FALSE
				If successful, return 'TRUE'
Description	  : write a serial mass of modbus registers.
******************************************************************************/
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

/******************************************************************************
Function Name :	MbsRcvSrv
Arguments 	  : [in, out]pMbs: pointer to the modbus object
Return		  : Null
Description	  : modbus-RTU slave receiving service.
******************************************************************************/
void MbsRcvSrv(MB_RTU_SLV *pMbs)
{
	uint8 rcvFrm[256];

	if((pMbs->GetRcvCnt() > 0) && (GetInterval(pMbs->GetLastRcvTime()) >= pMbs->rtuSpace))
	{
		pMbs->rbuf = rcvFrm;
		pMbs->rcvSize = pMbs->RcvFrm(pMbs->rbuf);

		/* if size of frame not enough, ignore it. */
		if (pMbs->rcvSize < MB_SER_PDU_SIZE_MIN)
			return;

		/* if command code invalid, ignore it. */
		if (rcvFrm[1] >= 0x80)
			return;
		
		/* device address match or broad-casting address or universal address */
		if ( (rcvFrm[0] == pMbs->devID)
			|| (rcvFrm[0] == MB_ID_UNIVE)
			|| ((rcvFrm[0] == MB_ID_BROAD) && ((rcvFrm[1] == MBC_WRITE_SINGLE_REG)
											   || (rcvFrm[1] == MBC_WRITE_MULTI_REG))) )
		{
			/* CRC check */
			if ( CRC16(rcvFrm, pMbs->rcvSize - 2)
				== ((rcvFrm[pMbs->rcvSize - 2] << 8) + rcvFrm[pMbs->rcvSize - 1]) )
			{
				MbsRsp(pMbs);
			}
		}
	}
}

/******************************************************************************
Function Name :	MbsSndSrv
Arguments 	  : [in]pMbs: pointer to the modbus object
Return		  : Null
Description	  : modbus-RTU slave sending service.
******************************************************************************/
void MbsSndSrv(MB_RTU_SLV *pMbs)
{
	if (pMbs->sndSize)
	{
		if (pMbs->IsSndIdle())
		{
			if (pMbs->SndFrm(pMbs->sbuf, pMbs->sndSize))
			{
				pMbs->sndSize = 0;
			}			
		}
	}
}

/******************************************************************************
Function Name :	MbsSrv
Arguments 	  : [in]pMbs: pointer to the modbus object
Return		  : Null
Description	  : Main entrence of modbus-RTU slave service.
******************************************************************************/
void MbsSrv(MB_RTU_SLV *pMbs)
{
	MbsRcvSrv(pMbs); //analyze receiving
	MbsSndSrv(pMbs); //analyze sending
}

/************************************************************************************
Procedure     :	MbsRegisterFuncCB
Arguments 	  : [in, out]pMbs: pointer to the modbus slave object
			  :	[in]handler: pointer to structure of callbacks to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on Modbus function execution.
***********************************************************************************/
BOOL8	MbsRegisterFuncCB(MB_RTU_SLV *pMbs, psMbsFuncCB handler)
{
	if (handler == NULL)
		return FALSE;

	pMbs->pFuncHandler = handler;
	return TRUE;
}


