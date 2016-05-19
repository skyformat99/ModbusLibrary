/*************************************************************************
**Copyright (c) 2016 Trojan Technologies Inc. 
**All rights reserved.
**
** File name:		mbproto.h
** Descriptions:	defines of modbus protocol
**------------------------------------------------------------------------
** Version:			1.0
** Created by:		Hengguang 
** Created date:	2016-03-12
** Descriptions:	The original version
**------------------------------------------------------------------------
** Version:				
** Modified by:			
** Modified date:		
** Descriptions:		
**************************************************************************/
#ifndef __MBPROTO_H
#define __MBPROTO_H


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

/******************************macro definition********************************/
#define MB_SER_PDU_SIZE_MIN     2       /*!< Minimum size of a Modbus RTU frame. */
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
#define MBE_NO_ERR				0
#define MBE_ILLEGAL_FUNCTION	1
#define MBE_ILLEGAL_DATA_ADDR	2
#define MBE_ILLEGAL_DATA_VALUE	3
#define MBE_SLAVE_DEVICE_FAIL	4
#define MBE_ACKNOWLEDGE			5
#define MBE_SLAVE_DEVICE_BUSY	6
#define MBE_MEMORY_PARITY_ERROR	8
#define MBE_GATEWAY_PATH_FAILED	10
#define MBE_GATEWAY_TGT_FAILED	11
#define MBE_UNKNOW_ERR			0xFF

//max data quantity about Modbus access at one time
#define MAX_READ_COIL			2000
#define MAX_READ_REG			125
#define MAX_WRITE_MULTI_COIL	1968
#define MAX_WRITE_MULTI_REG		123

//Modbus standard function code define
#define	MBC_READ_COIL			0x01
#define MBC_READ_INPUT_DIS		0x02
#define MBC_READ_HOLD_REG		0x03
#define MBC_READ_INPUT_REG		0x04
#define MBC_WRITE_SINGLE_COIL	0x05
#define MBC_WRITE_SINGLE_REG	0x06
#define MBC_WRITE_MULTI_COIL	0x0F
#define MBC_WRITE_MULTI_REG		0x10
//user defined function code (65-72)
#define MBC_GET_INNER_VER		0x41
#define MBC_GET_DEV_INFO		0x42
#define MBC_GET_STAT_INFO		0x43
#define MBC_SYS_DIAGN			0x44
#define MBC_GET_ERR				0x45
#define MBC_CLR_ERR				0x46
//user defined function code (100-110)
#define MBC_GET_LIB_VER			0x64

//#define MBC_ADC_CAL			0x61
//#define MBC_RST_PARAM			0x62


#define MIN_SBUF_SIZE	16	//restriction of size of send buffer 

  
/*******************************data definition********************************/
/* response types define */
typedef enum
{
	NO_RSP,	  //no response
	COR_RSP, //correct response
	ERR_RSP, //exceptional response
}RSP_TYPE;

/* function code define*/
/*typedef enum
{
	//Modbus standard function code define
	MBC_READ_COIL			= 0x01,
	MBC_READ_INPUT_DIS		= 0x02,
	MBC_READ_HOLD_REG		= 0x03,
	MBC_READ_INPUT_REG		= 0x04,
	MBC_WRITE_SINGLE_COIL	= 0x05,
	MBC_WRITE_SINGLE_REG	= 0x06,
	MBC_WRITE_MULTI_COIL	= 0x0E,
	MBC_WRITE_MULTI_REG	= 0x10,

	//user defined function code
	MBC_GET_INNER_VER		= 0x41,
	MBC_GET_DEV_INFO		= 0x42,
	MBC_GET_STAT_INFO		= 0x43,
	MBC_SYS_DIAGN			= 0x44,
	MBC_GET_ERR				= 0x45,
	MBC_CLR_ERR				= 0x46
}MB_FUNC_ENUM;*/



/****************************variable declaration******************************/


/*******************************macro operation********************************/


/****************************function declaration******************************/
#ifdef __cplusplus
extern "C" 	{
#endif


#ifdef __cplusplus 
} 
#endif 

#endif



