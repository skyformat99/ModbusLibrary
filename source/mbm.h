/*******************************************************************************
** Copyright (c) 2016 Trojan Technologies Inc. 
** All rights reserved.
**
** File name:		mbm.h
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
#ifndef __MBM_H
#define __MBM_H

#include "mbproto.h"


/******************************macro definition********************************/
//communication timeouts related macro
#define	DFT_RSP_TO		1000	//default response timeouts, unit: ms
#define MIN_RSP_TO		10	//minimal response timeouts, unit: ms
#define	MAX_RESND_CNT	3 	//max re-send count setting

#define	MB_BUF_SIZE		64


/*******************************data definition********************************/
/* modbus frame define */
typedef	union
{
	uint8	msgBuf[MB_BUF_SIZE];//buffer to load modbus frame
	struct	
	{
		uint8	devAddr; //address of device to be accessed
		uint8	cmdCode; //command code
		uint8	stAddrH; //high byte of start access address
		uint8	stAddrL; //low byte of start access address
		uint8	regCntH; //high byte of count of register to be accessed 
		uint8	regCntL; //low  byte of count of register to be accessed 
	}mbPilot;
}MB_FRM;

/* modbus communication state define, be suit for half-duplex mode */
typedef enum
{
	MB_INIT,		 //state to be initialize 
	MB_IDLE,        //idle state
	MB_RCV_ING,     //receiving
	MB_RCV_END,     //finished reveive frame
	MB_SND_WAIT,    //frame wait to be send out 
	MB_SND_ING,     //sending
	MB_SND_END,     //sending finished
	MB_SND_SUSPEND, //sending suspend
	MB_WAIT_RSP, 	 //wait for response, only used for MASTER
	MB_RSP_OT,		 //response overtime, only used for MASTER
}MB_STAT;

typedef struct sndinfo
{
	uint16  expectRspSize;
	uint16 	matchSize; //count of char that rsp frame need to match with req frame
	uint32  desAddr; //destination address to save read data
}RSP_CTRL;
#define 	RSP_CTRL_SIZE	8


/*! \name 	pxRightRspCB	
 *	\brief 	Callback on recieved correct response 
 *	\param 	pdu: pointer to the respond Modbus PDU frame 
 *	\param 	size: length of PDU
 *	\return TRUE-successful, 
 *			FALSE-failed, 
 */
typedef	BOOL8 	(*pxRightRspCB) (uint8 *pdu, uint16 size);

/*! \name 	pxErrRspCB	
 *	\brief 	Callback on recieved except response 
 *	\param 	pdu: pointer to the respond Modbus PDU frame 
 *	\param 	errCode: exceptional response code
 *	\return TRUE-successful, 
 *			FALSE-failed, 
 */
typedef	BOOL8 	(*pxErrRspCB) (uint8 *pdu, uint8 errCode);

/*! \name 	pxNoRspCB	
 *	\brief 	Callback on response timeout
 *	\return TRUE-successful, 
 *			FALSE-failed, 
 */
typedef	BOOL8 	(*pxNoRspCB) (void);


/* modbus master control structure, be suit for half-duplex mode */
typedef struct
{
	MB_STAT		state;   //modbus communication state
	uint8*		rbuf;    //load received modbus frame
	uint8*		sbuf;    //destination to put sending frame
	uint16		sbufSize;//size of send buffer
	uint16		rtuSpace; //min space time between RTU frame, unit: ms
	uint8		rcvSize; //length of received frame
	uint8		sndSize; //length of sending frame
	RSP_CTRL	rspCtrl; //response control information
	RSP_TYPE	rspType; //response type, available when need response
	uint16		rspTimeouts; //response timeouts, unit: ms
	BOOL8		isNeedRsp; //0-need, 1-not need;
	uint8		reSndSet; //max re-send count setting
	uint8		reSndCnt; //re-send count
	uint8		errCode; //exceptional response code

	/*	port supporting operations */
	
	/*! \name 	PortInit	
	 *	\brief 	Initialize physical port
	 *	\param 	baud: baud rate setting, (2400~115200 bps) 
	 *	\param 	parity: parity check mode setting, ('N' 'E' or 'O')
	 *	\param 	frmSpace: min RTU frame space setting, unit: ms
	 *	\return TRUE-success
	 *			FALSE-failed (invalid arguments)	
	 */
	BOOL8 	(*PortInit) (uint32 baud, uint8 parity, uint16 frmSpace);
	
	/*! \name 	GetRcvCnt	
	 *	\brief 	Get count of received data
	 *	\return count of received data 
	 */
	uint16	(*GetRcvCnt) (void);

	/*! \name 	GetLastRcvTime	
	 *	\brief 	Get time of last data received
	 *	\return time of last data received (ms)	
	 */
	uint32 	(*GetLastRcvTime) (void); 
	
	/*! \name 	GetLastSndTime	
	 *	\brief 	Get time of last data sent
	 *	\return time of last data set (ms)	
	 */
	uint32 	(*GetLastSndTime) (void); 
	
	/*! \name  	RcvFrm	
	 *	\brief 	Get received modbus frame out from port
	 *	\param 	pDes: destination to save received frame 
	 *	\return length of received frame	
	 */
	uint16 	(*RcvFrm) (uint8 *pDes);
	
	/*! \name  	IsSndIdle	
	 *	\brief 	Check whether port is idle for sending of modbus.
	 *	\return TRUE- port is idle for sending 
	 *			FALSE- port is busy for sending 
	 *	\comment: For half-duplex mode, sending is always idle while received 
	 *			  a frame, so can ignore check and always return TRUE here. 
	 */
	BOOL8 	(*IsSndIdle) (void);
	
	/*! \name  	SndFrm	
	 *	\brief 	Send modbus frame out
	 *	\param 	pFrm: pointer to the frame to be send out
	 *	\param 	size: length of frame
	 *	\return TRUE-sending successful, 
				FALSE-sending failed, buffer is full
	 */
	BOOL8 	(*SndFrm) (uint8 *pFrm, uint16 size);

	/*! \name  	RightRspHandler	
	 *	\brief 	Callback on recieved correct response 
	 *	\param 	pdu: pointer to the respond Modbus PDU frame 
	 *	\param 	size: length of PDU
	 *	\return TRUE-successful, 
	 *			FALSE-failed, 
	 *	\typedef BOOL8 (*pxRightRspCB) (uint8 *pdu, uint16 size);
	 */
	pxRightRspCB	RightRspHandler;

	/*! \name  	ErrRspHandler	
	 *	\brief 	Callback on recieved except response 
	 *	\param 	pdu: pointer to the respond Modbus PDU frame 
	 *	\param 	errCode: exceptional response code
	 *	\return TRUE-successful, 
	 *			FALSE-failed, 
	 *	\typedef BOOL8 (*pxErrRspCB) (uint8 *pdu, uint8 errCode);
	 */
	pxErrRspCB		ErrRspHandler;

	/*! \name  	NoRspHandler	
	 *	\brief 	Callback on response timeout
	 *	\return TRUE-successful, 
	 *			FALSE-failed, 
	 *	\typedef BOOL8 (*pxNoRspCB) (void);
	 */
	pxNoRspCB		NoRspHandler;
}MB_RTU_MST;


/****************************variable declaration******************************/


/*******************************macro operation********************************/


/****************************function declaration******************************/
#ifdef __cplusplus
extern "C" 	{
#endif

/*******************************************************************************
Procedure     :	MbmInit
Arguments 	  : [out]pMbm: pointer to the modbus control structure to be initialized
				[in]baud: baud rate(2400-115200 bps)
				[in]parity: parity setting, ('N' 'E' or 'O')
				[in]sndBuf: pointer of send buffer provided to modbus communication
				[in]sbufSize: size of send buffer
				[in]funcPortInit: pointer to function of 'Port Initialization'
				[in]funcGetRcvCnt: pointer to function of 'Get Receive data Count'
				[in]funcGetLastSndTime: pointer to function of 'Get Time of Last Sent data'
				[in]funcGetLastRcvTime: pointer to function of 'Get Time of Last Received data'
				[in]funcRcvFrm: pointer to function of 'get Received Frame out'
				[in]funcIsSndIdle: pointer to function of 'check if port Sending Is Idle'
				[in]funcSndFrm: pointer to function of 'Sending Frame'	
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Modbus-RTU master initialization.
*******************************************************************************/
extern	uint8 	MbmInit(MB_RTU_MST *pMbm, uint32 baud, uint8 parity, uint8 *sbuf, uint16 sbufSize,
			  			BOOL8 	(*funcPortInit) (uint32, uint8, uint16), 
			  			uint16	(*funcGetRcvCnt) (void),
			  			uint32 	(*funcGetLastRcvTime) (void),
			  			uint32 	(*funcGetLastSndTime) (void),
			  			uint16 	(*funcRcvFrm) (uint8*),
			  			BOOL8 	(*funcIsSndIdle) (void),
			  			BOOL8 	(*funcSndFrm) (uint8*, uint16));

/*******************************************************************************
Procedure     :	MbmSrv
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : Null
Description	  : Main entrence of modbus master service.
*******************************************************************************/
extern	void MbmSrv(MB_RTU_MST *pMbm);


/*******************************************************************************
Procedure     :	IsMbmBusy
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : TRUE- modbus master is busy
				FALSE- modbus master is idle
Description	  : check if modbus master be busy.
*******************************************************************************/
extern	inline uint8 IsMbmBusy(MB_RTU_MST *pMbm);

/*******************************************************************************
Procedure     :	GetRspType
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : response type of current request
Description	  : get response result type of current request.
*******************************************************************************/
extern	inline RSP_TYPE GetRspType(MB_RTU_MST *pMbm);

/*******************************************************************************
Procedure     :	GetErrCode
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
Return		  : exceptional response code of current request 
Description	  : get exceptional response code of current request.
				applicable on a exceptional response
*******************************************************************************/
extern	inline uint8 GetErrCode(MB_RTU_MST *pMbm);

/*******************************************************************************
Procedure     :	MbmRegisterRightRspCB
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]handler: callback to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on received correct response.
*******************************************************************************/
extern	BOOL8	MbmRegisterRightRspCB(MB_RTU_MST *pMbm, pxRightRspCB handler);

/*******************************************************************************
Procedure     :	MbmRegisterErrRspCB
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]handler: callback to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on received exceptional response.
*******************************************************************************/
extern	BOOL8	MbmRegisterErrRspCB(MB_RTU_MST *pMbm, pxErrRspCB handler);

/*******************************************************************************
Procedure     :	MbmRegisterNoRspCB
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]handler: callback to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on response timeouts.
*******************************************************************************/
extern	BOOL8	MbmRegisterNoRspCB(MB_RTU_MST *pMbm, pxNoRspCB handler);

/*******************************************************************************
Procedure     :	MbmTimeoutsSet
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]timeouts: timeouts value to be set, unit: ms
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Set response timeouts for master.
*******************************************************************************/
extern	BOOL8	MbmTimeoutsSet(MB_RTU_MST *pMbm, uint16 timeouts);

/*******************************************************************************
Procedure     :	MbmReSndSet
Arguments 	  : [in, out]pMbm: pointer to the modbus master object
			  :	[in]cnt: resending count to be set 
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Set resending count for master.
*******************************************************************************/
extern	BOOL8	MbmReSndSet(MB_RTU_MST *pMbm, uint8 cnt);

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
extern	uint8 MbmSndReq(MB_RTU_MST *pMbm, uint8 *pfrm, uint16 size, uint32 destAddr);

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
extern	uint8 MbReadHoldRegReq(MB_RTU_MST *pMbm, uint8 devId, uint16 addr, uint16 size, uint16* dest);

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
extern	uint8 	MbWriteSingleRegReq(MB_RTU_MST *pMbm, uint8 devId, uint16 addr, uint16 data);

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
extern	uint8 	MbWriteMultiRegReq(MB_RTU_MST *pMbm, uint8 devId, uint16 addr, uint16 size, uint16* src);

#ifdef __cplusplus 
} 
#endif 


#endif




