/*******************************************************************************
** Copyright (c) 2016 Trojan Technologies Inc. 
** All rights reserved.
**
** File name:		mbm.c
** Descriptions:	process of modbus-RTU master communication
**------------------------------------------------------------------------------
** Version:		    1.0
** Created by:		Hengguang 
** Created date:	2016-03-08
** Descriptions:	The original version
**------------------------------------------------------------------------------
** Version:				
** Modified by:			
** Modified date:		
** Descriptions:		
*******************************************************************************/
#define __MBM_C

#include "common.h"
#include "mbm.h"




/*************************extern variable declaration**************************/


/*************************extern function declaration**************************/
extern  uint32 	GetInterval(uint32 prevTime); //unit: ms


/****************************variable declaration******************************/


/****************************function declaration******************************/
static	void 	MbmSndSrv(MB_RTU_MST *pMbm);
static	void 	MbmRcvSrv(MB_RTU_MST *pMbm);
static	void 	MbmRcvRspSrv(MB_RTU_MST *pMbm);
static	void 	OnRspRight(MB_RTU_MST *pMbm);
static	void 	OnRspErr(MB_RTU_MST *pMbm);
static	void 	OnRspTimeout(MB_RTU_MST *pMbm);

static	void 	ReadHoldRegRight(MB_RTU_MST *pMbm);

/*******************************************************************************
Procedure     :	MbmInit
Arguments 	  : [out]pMbm: pointer to the modbus control structure to be initialized
				[in]baud: baud rate(2400-115200 bps)
				[in]parity: parity setting, ('N' 'E' or 'O')
				[in]sndBuf: pointer of send buffer provided to modbus communication
				[in]sbufSize: size of send buffer
				[in]funcPortInit: pointer to function of 'Port Initialization'
				[in]funcGetRcvCnt: pointer to function of 'Get Receive data Count'
				[in]funcGetLastRcvTime: pointer to function of 'Get Time of Last Received data'
				[in]funcGetLastSndTime: pointer to function of 'Get Time of Last Sent data'
				[in]funcRcvFrm: pointer to function of 'get Received Frame out'
				[in]funcIsSndIdle: pointer to function of 'check if port Sending Is Idle'
				[in]funcSndFrm: pointer to function of 'Sending Frame'	
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Modbus-RTU master initialization.
*******************************************************************************/
uint8 MbmInit(MB_RTU_MST *pMbm, uint32 baud, uint8 parity, uint8 *sbuf, uint16 sbufSize,
			  BOOL8 	(*funcPortInit) (uint32, uint8, uint16), 
			  uint16	(*funcGetRcvCnt) (void),
			  uint32 	(*funcGetLastRcvTime) (void),
			  uint32 	(*funcGetLastSndTime) (void),
			  uint16 	(*funcRcvFrm) (uint8*),
			  BOOL8 	(*funcIsSndIdle) (void),
			  BOOL8 	(*funcSndFrm) (uint8*, uint16))
{
	if (sbufSize < MIN_SBUF_SIZE)
	{
		return FALSE; //size of send buffer is not enough
	}		

	//MbmAppInit(pMbm); 
	pMbm->rcvSize = 0;
	pMbm->sbuf = sbuf;
	pMbm->sbufSize = sbufSize;
	pMbm->sndSize= 0;
	pMbm->isNeedRsp = FALSE;
	pMbm->reSndCnt = 0;
	pMbm->rspType = COR_RSP;
	pMbm->state = MB_IDLE;
	pMbm->rspTimeouts = DFT_RSP_TO;
	pMbm->reSndSet = MAX_RESND_CNT;
	if (baud > 19200)
	{
		pMbm->rtuSpace = 3; //unit: ms, 1750(us) / 1000(us/ms) + 2 = 3
	}
	else
	{
		pMbm->rtuSpace = (3500 * 11) / baud + 2; //unit: ms, 3500=3.5*1000(ms/s)
	}

	/*	initialize port supporting operation interface */
	pMbm->PortInit = funcPortInit;
	pMbm->GetRcvCnt = funcGetRcvCnt;
	pMbm->GetLastRcvTime = funcGetLastRcvTime;
	pMbm->GetLastSndTime = funcGetLastSndTime;
	pMbm->RcvFrm = funcRcvFrm;
	pMbm->IsSndIdle = funcIsSndIdle;
	pMbm->SndFrm = funcSndFrm;

	//response callback is empty by default
	pMbm->RightRspHandler = NULL;
	pMbm->ErrRspHandler = NULL;
	pMbm->NoRspHandler = NULL;
	
	//if (!Uart0Init(baud, DFT_CHARSIZE, DFT_PARITY, DFT_STOPBIT, rtuFrmSpaceSet))
	if (!(pMbm->PortInit(baud, parity, pMbm->rtuSpace)))
	{
		return FALSE;
	}
	
	return TRUE;
}


/*******************************************************************************
Procedure     :	IsMbmBusy
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : TRUE- modbus master is busy
				FALSE- modbus master is idle
Description	  : check if modbus master be busy.
*******************************************************************************/
inline uint8 IsMbmBusy(MB_RTU_MST *pMbm)
{
	return (pMbm->state != MB_IDLE);
}

/*******************************************************************************
Procedure     :	GetRspType
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : response type of current request
Description	  : get response result type of current request.
*******************************************************************************/
inline RSP_TYPE GetRspType(MB_RTU_MST *pMbm)
{
	return (pMbm->rspType);
}

/*******************************************************************************
Procedure     :	GetErrCode
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : exceptional response code of current request 
Description	  : get exceptional response code of current request.
				applicable on a exceptional response
*******************************************************************************/
inline uint8 GetErrCode(MB_RTU_MST *pMbm)
{
	return (pMbm->errCode);
}

/*******************************************************************************
Procedure     :	ReadHoldRegRight
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master service on receiving correct response for reading 
				hold register.
*******************************************************************************/
void ReadHoldRegRight(MB_RTU_MST *pMbm)
{
	uint16	*pword = (uint16*)pMbm->rspCtrl.desAddr;
	uint8	*pchar = pMbm->rbuf + 3;
	uint8	words = pMbm->rbuf[2] / 2;

	if (pword == NULL)
		return;
	
	while (words--)
	{
		*pword = *pchar++ << 8;
		*pword += *pchar++;
		pword++;
	}
	//can add additional control here 
}

/*******************************************************************************
Procedure     :	OnRspRight
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master service on receiving correct response.
*******************************************************************************/
void OnRspRight(MB_RTU_MST *pMbm)
{
	switch (pMbm->rbuf[1]) //command code
	{
		case  MBC_READ_HOLD_REG:
			ReadHoldRegRight(pMbm);
			break;
		default:
			break;
	}

	if (pMbm->RightRspHandler != NULL)
	{
		pMbm->RightRspHandler(pMbm->rbuf + 1, pMbm->rcvSize - 3);
	}
}

/*******************************************************************************
Procedure     :	OnRspErr
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master service on receiving exceptional response.
*******************************************************************************/
void OnRspErr(MB_RTU_MST *pMbm)
{
	if (pMbm->ErrRspHandler != NULL)
	{
		pMbm->ErrRspHandler(pMbm->rbuf + 1, pMbm->rbuf[2]);
	}
}

/*******************************************************************************
Procedure     :	OnRspTimeout
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master service on receiving response timeout.
*******************************************************************************/
void OnRspTimeout(MB_RTU_MST *pMbm)
{
	if (pMbm->NoRspHandler != NULL)
	{
		pMbm->NoRspHandler();
	}
}

/*******************************************************************************
Procedure     :	MbmRcvRspSrv
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master service on receiving response.
*******************************************************************************/
void MbmRcvRspSrv(MB_RTU_MST *pMbm)
{
	if ((pMbm->rcvSize == pMbm->rspCtrl.expectRspSize)
		&& (!memcmp(pMbm->sbuf, pMbm->rbuf, pMbm->rspCtrl.matchSize)))
	{
		pMbm->isNeedRsp = FALSE;
		pMbm->rspType = COR_RSP;
		pMbm->state = MB_IDLE;
		OnRspRight(pMbm);
	}
	else if ((pMbm->rcvSize == 5) 
			&& (pMbm->rbuf[0] == pMbm->sbuf[0])
		    && (pMbm->rbuf[1] == pMbm->sbuf[1] + 0x80))
	{
		pMbm->isNeedRsp = FALSE;
		pMbm->rspType = ERR_RSP;
		pMbm->errCode = pMbm->rbuf[2];
		pMbm->state = MB_IDLE;
		OnRspErr(pMbm);
	}
}

/*******************************************************************************
Procedure     :	MbmSndReq
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
				[in]pfrm: head of frame to be sent out
				[in]size: size of frame without CRC check bytes
				[in]destAddr: destination address to save read data, available 
					only on reading.
Return		  : TRUE- sending request be accepted successfully
				FALSE- sending request failed
Description	  : modbus master sending service.
*******************************************************************************/
uint8 MbmSndReq(MB_RTU_MST *pMbm, uint8 *pfrm, uint16 size, uint32 destAddr)
{
	if (pMbm->state != MB_IDLE)
		return FALSE;

	uint8 	expectRspSize = 0;
	uint8 	matchSize = 0;
	uint8	*pch;
	uint16	crcValue = 0;
	
	if (*pfrm == MB_ID_BROAD)
	{
		pMbm->isNeedRsp = FALSE;
	}
	else
	{
		switch (*(pfrm+1)) //command code
		{
			case  MBC_READ_HOLD_REG:
				matchSize = 2;
				expectRspSize = 5 + ((*(pfrm+4)<<8) + *(pfrm+5))*2;			
				break;
			case  MBC_WRITE_SINGLE_REG:
				matchSize = 6;
				expectRspSize = 8;			
				break;
			case  MBC_WRITE_MULTI_REG:
				matchSize = 6;
				expectRspSize = 8;			
				break;
			default: //unsupported command code 
				return FALSE;
		}

		pMbm->rspCtrl.expectRspSize = expectRspSize;
		pMbm->rspCtrl.matchSize = matchSize;
		pMbm->rspCtrl.desAddr = destAddr;
		pMbm->reSndCnt = 0;
		
		pMbm->isNeedRsp = TRUE;
	}

	pch = pMbm->sbuf;
	pch = CopyDataWithDesPtrMove(pch, pfrm, size);
	crcValue = CRC16(pfrm, size);
	*pch++ = crcValue >> 8;
	*pch = crcValue & 0xff;

	//set sending control information
	pMbm->sndSize = size + 2;
	pMbm->state = MB_SND_WAIT;
	
	return TRUE;
}


/*******************************************************************************
Procedure     :	MbmRcvSrv
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master receiving service.
*******************************************************************************/
void MbmRcvSrv(MB_RTU_MST *pMbm)
{
	uint8	rcvFrm[256];

	/* if receiving end (judged by free line time) */
	if ( (pMbm->GetRcvCnt() > 0) && (GetInterval(pMbm->GetLastRcvTime()) >= pMbm->rtuSpace) ) 
	{
		pMbm->rbuf = rcvFrm;
		pMbm->rcvSize = pMbm->RcvFrm(rcvFrm);
		
		/* if size of frame not enough, ignore it. */
		if (pMbm->rcvSize < 4)
			return;

		/* CRC check */
		if ( CRC16(rcvFrm, pMbm->rcvSize - 2)
			== ((rcvFrm[pMbm->rcvSize - 2] << 8) + rcvFrm[pMbm->rcvSize - 1]) )
		{
			if (pMbm->isNeedRsp == TRUE)
			{
				MbmRcvRspSrv(pMbm);
			}
		}
	}
}

/*******************************************************************************
Procedure     :	MbmSndSrv
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : modbus master sending service.
*******************************************************************************/
void MbmSndSrv(MB_RTU_MST *pMbm)
{
	//sending check
	if (!pMbm->IsSndIdle())
		return;

	if (pMbm->state > MB_SND_WAIT)
	{
		if (!pMbm->isNeedRsp)
		{		
			pMbm->state = MB_IDLE;
		}
	}
	else if (pMbm->state == MB_SND_WAIT)
	{
		if (pMbm->SndFrm(pMbm->sbuf, pMbm->sndSize))
		{
			pMbm->state = MB_WAIT_RSP; //MB_SND_ING;
		}			
	}	
		
	if (pMbm->isNeedRsp)
	{
		if (GetInterval(pMbm->GetLastSndTime()) > pMbm->rspTimeouts)
		{
			if (pMbm->reSndCnt < pMbm->reSndSet)
			{
				pMbm->state = MB_SND_WAIT;
				pMbm->reSndCnt++;
			}
			else 
			{
				pMbm->isNeedRsp = FALSE;
				pMbm->state = MB_IDLE;
				pMbm->rspType = NO_RSP;
				OnRspTimeout(pMbm);
			}
		}
	}
	#if 0
	else if (pMbm->state > MB_SND_WAIT)
	{
		if (pMbm->IsSndIdle())
		{
			pMbm->state = MB_IDLE;
		}
	}
	#endif
}


/*******************************************************************************
Procedure     :	MbmSrv
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : Main entrence of modbus master service.
*******************************************************************************/
void MbmSrv(MB_RTU_MST *pMbm)
{
	if (pMbm->IsSndIdle())
	{
		MbmRcvSrv(pMbm); //analyze receiving
		MbmSndSrv(pMbm); //analyze sending
	}
}

/*******************************************************************************
Procedure     :	MbmRegisterRightRspCB
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]handler: callback to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on received correct response.
*******************************************************************************/
BOOL8	MbmRegisterRightRspCB(MB_RTU_MST *pMbm, pxRightRspCB handler)
{
	if (handler == NULL)
		return FALSE;

	pMbm->RightRspHandler = handler;
	return TRUE;
}

/*******************************************************************************
Procedure     :	MbmRegisterErrRspCB
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]handler: callback to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on received exceptional response.
*******************************************************************************/
BOOL8	MbmRegisterErrRspCB(MB_RTU_MST *pMbm, pxErrRspCB handler)
{
	if (handler == NULL)
		return FALSE;

	pMbm->ErrRspHandler = handler;
	return TRUE;
}

/*******************************************************************************
Procedure     :	MbmRegisterNoRspCB
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]handler: callback to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on response timeouts.
*******************************************************************************/
BOOL8	MbmRegisterNoRspCB(MB_RTU_MST *pMbm, pxNoRspCB handler)
{
	if (handler == NULL)
		return FALSE;

	pMbm->NoRspHandler = handler;
	return TRUE;
}

/*******************************************************************************
Procedure     :	MbmTimeoutsSet
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]timeouts: timeouts value to be set, unit: ms
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Set response timeouts for master.
*******************************************************************************/
BOOL8	MbmTimeoutsSet(MB_RTU_MST *pMbm, uint16 timeouts)
{
	if (timeouts < MIN_RSP_TO)
		return FALSE;

	pMbm->rspTimeouts = timeouts;
	return TRUE;
}

/*******************************************************************************
Procedure     :	MbmReSndSet
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]cnt: resending count to be set 
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Set resending count for master.
*******************************************************************************/
BOOL8	MbmReSndSet(MB_RTU_MST *pMbm, uint8 cnt)
{
	pMbm->reSndSet = cnt;
	return TRUE;
}

/*******************************************************************************
Procedure     :	MbReadHoldRegReq
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
				[in]devId: device ID of slave to be accessed
				[in]addr: start address of registers to be read
				[in]size: size of registers to be read
				[out]dest: destination to put data be read
Return		  : TRUE- request successfully
				FALSE- request failed
Description	  : Request reading modbus hold registers.
*******************************************************************************/
uint8 MbReadHoldRegReq(MB_RTU_MST *pMbm, uint8 devId, uint16 addr, uint16 size, uint16* dest)
{
	uint8	frm[8] = {0};

	if (devId == MB_ID_BROAD)
		return FALSE;
	
	if ((!size) || (size > MAX_READ_REG))
		return FALSE;
	
	frm[0] = devId;
	frm[1] = MBC_READ_HOLD_REG;
	frm[2] = addr >> 8;
	frm[3] = addr & 0xFF;
	frm[4] = size >> 8;
	frm[5] = size & 0xFF;
		
	return MbmSndReq(pMbm, frm, 6, (uint32)dest);
}

/*******************************************************************************
Procedure     :	MbWriteSingleRegReq
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
				[in]devId: device ID of slave to be accessed
				[in]addr: start address of registers to be read
				[in]data: source data to be written into Modbus slave
Return		  : TRUE- request successfully
				FALSE- request failed
Description	  : Request writing single modbus hold registers.
*******************************************************************************/
uint8 MbWriteSingleRegReq(MB_RTU_MST *pMbm, uint8 devId, uint16 addr, uint16 data)
{
	uint8	frm[8] = {0};
	uint8	*pch = frm;

	*pch++ = devId;
	*pch++ = MBC_WRITE_SINGLE_REG;	
	*pch++ = addr >> 8;
	*pch++ = addr & 0xFF;	
	*pch++ = data >> 8;
	*pch++ = data & 0xFF;
		
	return MbmSndReq(pMbm, frm, 6, NULL);
}

/*******************************************************************************
Procedure     :	MbWriteMultiRegReq
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
				[in]devId: device ID of slave to be accessed
				[in]addr: start address of registers to be read
				[in]size: size of registers to be read
				[in]src: source data to be written into Modbus slave
Return		  : TRUE- request successfully
				FALSE- request failed
Description	  : Request writing multiple modbus hold registers.
*******************************************************************************/
uint8 MbWriteMultiRegReq(MB_RTU_MST *pMbm, uint8 devId, uint16 addr, uint16 size, uint16* src)
{
	uint8	frm[256] = {0};
	uint8	*pch = frm;
	uint32	i = size << 1;

	if ((!size) || (size > MAX_WRITE_MULTI_REG))
		return FALSE;
	
	*pch++ = devId;
	*pch++ = MBC_WRITE_MULTI_REG;	
	*pch++ = addr >> 8;
	*pch++ = addr & 0xFF;	
	*pch++ = size >> 8;
	*pch++ = size & 0xFF;
	*pch++ = size << 1;
	
	while (size--)
	{
		*pch++ = (*src) >> 8;
		*pch++ = (*src++) & 0xFF;
	}
		
	return MbmSndReq(pMbm, frm, 7+i, NULL);
}

