/*******************************************************************************
** Copyright (c) 2012 Trojan Technologies Inc. 
** All rights reserved.
**
** File name:		datatype.h
** Descriptions:	redefinition of basic data type
**------------------------------------------------------------------------------
** Version:		    1.0
** Created by:		Hengguang 
** Created date:	2012-12-12
** Descriptions:	The original version
**------------------------------------------------------------------------------
** Version:				
** Modified by:			
** Modified date:		
** Descriptions:		
*******************************************************************************/
#ifndef DATATYPE_H
#define DATATYPE_H


#ifdef __cplusplus
extern "C" 	{
#endif


/* Generic Base Data Types */
typedef unsigned char 	BOOL8;
typedef	unsigned char  	uint8; 	/* defined for unsigned 8-bits integer variable */                  
typedef signed char  	int8;  	/* defined for signed 8-bits integer variable */             
typedef unsigned short  uint16;	/* defined for unsigned 16-bits integer variable */                 
typedef short   		int16; 	/* defined for signed 16-bits integer variable */                   
typedef unsigned long   uint32;	/* defined for unsigned 32-bits integer variable */                 
typedef long   			int32; 	/* defined for signed 32-bits integer variable */                   
typedef float          	float32;/* single precision floating point variable (32bits) */
typedef double         	float64;/* double precision floating point variable (64bits) */

typedef volatile int8	vint8;  /*  8 bits */
typedef volatile int16	vint16; /* 16 bits */
typedef volatile int32	vint32; /* 32 bits */
typedef volatile uint8	vuint8;  /*  8 bits */
typedef volatile uint16	vuint16; /* 16 bits */
typedef volatile uint32	vuint32; /* 32 bits */

#undef	TEXT
#define	TEXT(x)			(void *) (x)

/* defines of function pointer */
typedef void  	(*pfvoid)(void); 
typedef BOOL8 	(*pfB8)(void); 
typedef uint8	(*pfu8)(void); 
typedef uint16	(*pfu16)(void); 
typedef uint32	(*pfu32)(void); 
typedef void  	(*pfv_u8_u8)(uint8, uint8); 
typedef uint16  (*pfu16_pu8)(uint8*); 
typedef void  	(*pfv_pu8_u16)(uint8*, uint16); 
typedef void  	(*pfv_u32_u8_u16)(uint32, uint8, uint16); 


#ifdef __cplusplus 
} 
#endif 

#endif
