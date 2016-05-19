/*************************************************************************
**Copyright (c) 2013 Trojan Technologies Inc. 
**All rights reserved.
**
** File name:		mbs.h
** Descriptions:	head file of mbs.c
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
#ifndef __MBS_H
#define __MBS_H


#ifdef 	__MBS_C
	#define	EXTERN
#else
	#define EXTERN	extern
#endif


#include "datatype.h"
#include "mbproto.h"


/******************************macro definition*******************************/

  
/*******************************data definition*******************************/
/*! \name 	pxPreRdRegCB	
 *	\brief 	Callback before response on reading hold register 
 *	\param 	dest: destination to save data to be read 
 *	\param 	addr: satart address to read 
 *	\param 	size: length of data by wrod to be read
 *	\return 0 if execution all right, 
 *			user defined exception code(0x80-0xC0) if execution exception. 
 */
typedef	uint8 	(*pxPreRdRegCB) (uint8 *dest, uint16 addr, uint16 size);

/*! \name 	pxPreWtRegCB	
 *	\brief 	Callback before response on writing hold register 
 *	\param 	src: pointer to source data to be writen in  
 *	\param 	addr: satart address to write 
 *	\param 	size: length of data by wrod to be writen
 *	\return 0 if execution all right, 
 *			user defined exception code(0x80-0xC0) if execution exception. 
 */
typedef	uint8 	(*pxPreWtRegCB) (uint8 *src, uint16 addr, uint16 size);

/*! \name 	pxPostRdRegCB	
 *	\brief 	Callback after response on reading hold register 
 *	\param 	addr: satart address to read 
 *	\param 	size: length of data by wrod to be read
 *	\return TRUE-successful, 
 *			FALSE-failed, 
 */
typedef	BOOL8 	(*pxPostRdRegCB) (uint16 addr, uint16 size);

/*! \name 	pxPostWtRegCB	
 *	\brief 	Callback after response on writing hold register 
 *	\param 	addr: satart address to write 
 *	\param 	size: length of data by wrod to be writen
 *	\return TRUE-successful, 
 *			FALSE-failed, 
 */
typedef	BOOL8 	(*pxPostWtRegCB) (uint16 addr, uint16 size);

/*
typedef struct
{
	pxPreRdRegCB	PreRdRegHandler;
	pxPreWtRegCB	PreWtRegHandler;
}pxPreRspCB;

typedef struct
{
	pxPostRdRegCB	PostRdRegHandler;
	pxPostWtRegCB	PostWtRegHandler;
}pxPostRspCB;
*/

typedef struct
{
	//pxPreRspCB
	pxPreRdRegCB	PreRdRegHandler;
	pxPreWtRegCB	PreWtRegHandler;

	//pxPostRspCB
	pxPostRdRegCB	PostRdRegHandler;
	pxPostWtRegCB	PostWtRegHandler;
}sMbsFuncCB, *psMbsFuncCB;

/* modbus-RTU slave control structure */
typedef struct
{
	/*	for modbus register areas */
	uint16*		pMbReg;		   //pointer to modbus register area
	uint16		regSize;	   //size of whole register area
	uint16		regBaseAddr;   //start modbus address of whole register area
	uint16		rwRegBaseAddr; //start modbus address of RW register area
	uint16		rwRegSize;	   //size of RW register area

	/*	for modbus receiving and sending */
	uint16		rtuSpace;//RTU frame space setting, uint: ms
	uint8		devID;   //device address
	uint8		rcvSize; //length of received frame
	uint8*		rbuf;    //buffer to load received modbus frame
	uint8*		sbuf;    //destination to put response frame
	uint8		sbufSize;//size of send buffer
	uint8		sndSize;  //legth of response frame
	uint8		errCode; //exception code
	RSP_TYPE	rspType; //response type

	/*	port supporting operations */
	
	/*! \name  	PortInit	
	 *	\brief 	Initialize physical port
	 *	\param 	baud: baud rate setting, (2400~115200 bps) 
	 *	\param 	parity: parity check mode setting, ('N' 'E' or 'O')
	 *	\param 	frmSpace: min RTU frame space setting, unit: ms
	 *	\return TRUE-success
	 *			FALSE-failed (invalid arguments)	
	 */
	BOOL8 	(*PortInit) (uint32 baud, uint8 parity, uint16 frmSpace);
	
	/*! \name  	GetRcvCnt	
	 *	\brief 	Get count of received data
	 *	\return count of received data 
	 */
	uint16	(*GetRcvCnt) (void);

	/*! \name  	GetLastRcvTime	
	 *	\brief 	Get time of last data received
	 *	\return time of last data received (ms)	
	 */
	uint32 	(*GetLastRcvTime) (void); 
	
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

	/*	pointer to Modbus function execution callback structure, 
		including callbacks before response and after response. */
	psMbsFuncCB	pFuncHandler;	
}MB_RTU_SLV;


/****************************variable declaration*****************************/


/*******************************macro operation*******************************/


/****************************function declaration*****************************/
#ifdef __cplusplus
extern "C" 	{
#endif

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
EXTERN uint8	MbsInit(MB_RTU_SLV *pMbs, uint32 baud, uint8 parity, uint8 devAddr, 
			  			uint16 *pReg, uint16 regSize, uint16 regBase, uint16 rwBase, 
			  			uint16 rwSize, uint8 *sndBuf, uint8 sbufSize,
			  			BOOL8 	(*funcPortInit) (uint32, uint8, uint16), 
			  			uint16	(*funcGetRcvCnt) (void),
			  			uint32 	(*funcGetLastRcvTime) (void),
			  			uint16 	(*funcRcvFrm) (uint8*),
			  			BOOL8 	(*funcIsSndIdle) (void),
			  			BOOL8 	(*funcSndFrm) (uint8*, uint16));

/******************************************************************************
Function Name :	MbsSrv
Arguments 	  : [in]pMbs: pointer to the modbus object
Return		  : Null
Description	  : Main entrence of modbus-RTU slave service.
******************************************************************************/
EXTERN void		MbsSrv(MB_RTU_SLV *pMbs);

/******************************************************************************
Function Name :	ReadMbReg
Arguments 	  : [in]pMbs: pointer to the modbus object
				[in]index: data index in the modbus register array
Return		  : data to be read, if 'index' overrun, return 'FAIL'(0xFFFF) 
Description	  : read a single modbus register.
******************************************************************************/
EXTERN uint16 	ReadMbReg(MB_RTU_SLV *pMbs, uint8 index);

/******************************************************************************
Function Name :	WriteMbReg
Arguments 	  : [in, out]pMbs: pointer to the modbus object
				[in]index: data index in the modbus register array
				[in]data: data to be written in
Return		  : TRUE-success
				FALSE-failed, 'inedx' overrun 
Description	  : write a single modbus register.
******************************************************************************/
EXTERN uint8 	WriteMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 data);

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
EXTERN uint8 	ReadMassMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 *pDes, uint8 size);

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
EXTERN uint8 	WriteMassMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 *pSrc, uint8 size);

/************************************************************************************
Procedure     :	MbsRegisterFuncCB
Arguments 	  : [in, out]pMbs: pointer to the modbus slave object
			  :	[in]handler: pointer to structure of callbacks to be registered
Return		  : TRUE-success
				FALSE-failed (invalid arguments)
Description	  : Register callback function on Modbus function execution.
***********************************************************************************/
EXTERN BOOL8	MbsRegisterFuncCB(MB_RTU_SLV *pMbs, psMbsFuncCB handler);

#ifdef __cplusplus 
} 
#endif 


#undef EXTERN
#endif



