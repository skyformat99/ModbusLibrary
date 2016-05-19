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
#ifndef MBS_H
#define MBS_H


#ifdef __cplusplus
extern "C" 	{
#endif



/******************************macro definition********************************/
#define MIN_SBUF_SIZE	16	//restriction of size of send buffer 

  
/*******************************data definition********************************/
/* response types define */
typedef enum
{
	NO_RSP,	  //no response
	COR_RSP, //correct response 
	ERR_RSP //exceptional response
}RSP_TYPE;

/* Modbus-RTU slave control structure */
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
	uint8		rcvLen;  //length of frame
	uint8*		rbuf;    //buffer to load received modbus frame
	uint8*		sbuf;    //destination to put response frame
	uint8		sbufSize;//size of send buffer
	uint8		sndLen;  //legth of response frame
	uint8		errCode; //exception code
	RSP_TYPE	rspType; //response type

	/*	port supporting operations */
	
	/*! \name pfPortInit	
	 *	\brief Initialize physical port
	 *	\param baud: baud rate setting, (2400~115200 bps) 
	 *	\param parity: parity check mode setting, ('N' 'E' or 'O')
	 *	\param frmSpace: min RTU frame space setting, unit: 0.1ms
	 *	\return TRUE-success
	 *			FALSE-failed (invalid arguments)	
	 */
	BOOL8 	(*pfPortInit) (uint32 baud, uint8 parity, uint16 frmSpace);
	
	/*! \name pfGetRcvCnt	
	 *	\brief Get count of received data
	 *	\return count of received data 
	 */
	uint16	(*pfGetRcvCnt) (void);

	/*! \name pfGetLastRcvTime	
	 *	\brief Get time of last data received
	 *	\return time of last data received (ms)	
	 */
	uint32 	(*pfGetLastRcvTime) (void); 
	
	/*! \name pfRcvFrm	
	 *	\brief Get received modbus frame out from port
	 *	\param pDes: destination to save received frame 
	 *	\return length of received frame	
	 */
	uint16 	(*pfRcvFrm) (uint8 *pDes);
	
	/*! \name pfIsSndIdle	
	 *	\brief Check whether port is idle for sending of modbus.
	 *	\return TRUE- port is idle for sending 
	 *			FALSE- port is busy for sending 
	 *	\comment: For half-duplex mode, sending is always idle while received 
	 *			  a frame, so can ignore check and always return TRUE here. 
	 */
	BOOL8 	(*pfIsSndIdle) (void);
	
	/*! \name pfSndFrm	
	 *	\brief Send modbus frame out
	 *	\param pFrm: pointer to the frame to be send out
	 *	\param size: length of frame
	 *	\return TRUE-sending successful, 
				FALSE-sending failed, buffer is full
	 */
	BOOL8 	(*pfSndFrm) (uint8 *pFrm, uint16 size);
}MB_RTU_SLV;


/****************************variable declaration******************************/


/*******************************macro operation********************************/


/****************************function declaration******************************/
extern uint8	MbsInit(MB_RTU_SLV *pMbs, uint32 baud, uint8 parity, uint8 devAddr, 
			  			uint16 *pReg, uint16 regSize, uint16 regBase, uint16 rwBase, 
			  			uint16 rwSize, uint8 *sndBuf, uint8 sbufSize,
			  			BOOL8 	(*funcPortInit) (uint32, uint8, uint16), 
			  			uint16	(*funcGetRcvCnt) (void),
			  			uint32 	(*funcGetLastRcvTime) (void),
			  			uint16 	(*funcRcvFrm) (uint8*),
			  			BOOL8 	(*funcIsSndIdle) (void),
			  			BOOL8 	(*funcSndFrm) (uint8*, uint16));
extern void		MbsSrv(MB_RTU_SLV *pMbs);
extern uint16 	ReadMbReg(MB_RTU_SLV *pMbs, uint8 index);
extern uint8 	WriteMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 data);
extern uint8 	ReadMassMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 *pDes, uint8 size);
extern uint8 	WriteMassMbReg(MB_RTU_SLV *pMbs, uint8 index, uint16 *pSrc, uint8 size);


#ifdef __cplusplus 
} 
#endif 

#undef EXTERN
#endif



