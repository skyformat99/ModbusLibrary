/*******************************************************************************
** Copyright (c) 2012 Trojan Technologies Inc. 
** All rights reserved.
**
** File name:		common.h
** Descriptions:	definition of general-purpose operation or common symbol
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
#ifndef COMMON_H
#define COMMON_H

#include "datatype.h"


#ifdef __cplusplus
extern "C" 	{
#endif


/***********************definition of common used macro************************/
#ifdef	FALSE
#undef	FALSE
#endif
#define FALSE	(0)

#ifdef	TRUE
#undef	TRUE
#endif
#define	TRUE	(1)

#ifdef	NULL
#undef	NULL
#endif
#define NULL	(0)

#ifdef  ON
#undef  ON
#endif
#define ON      (1)

#ifdef  OFF
#undef  OFF
#endif
#define OFF     (0)

#ifdef  FAIL
#undef  FAIL
#endif
#define FAIL    (-1)

#ifdef  PASS
#undef  PASS
#endif
#define PASS    (0)
  
#ifdef  SCCS
#undef  SCCS
#endif
#define SCCS    (1)

  
/* Logical Operators */
#define AND       &&
#define OR        ||
#define NOT       !
#define ISNOT     !=
#define IS        ==
#define MORE_THAN >
#define LESS_THAN <

/* Comparision operators */
#define EQ  ==
#define GE  >=
#define GT  >
#define LE  <=
#define LT  <
#define NE  !=

/* Bitwise operators */
#define BAND  &
#define BITOR |
#define BXOR  ^
#define BNOT  ~
#define LSHF  <<
#define RSHF  >>


#define Min(v0, v1) 	((v0>v1) ? v1 : v0)
#define Max(v0, v1) 	((v0>v1) ? v0 : v1)

/* get a byte from a appointed address */
#define MEM_B(x)	    (*((uint8*)(x)))		
/* get a word from a appointed address */
#define MEM_W(x)	    (*((uint16*)(x)))	

/* convert two bytes to a word according to LSB format */
#define FLIPW(ray)	    ((((uint16)(ray)[0]) * 256) + (ray)[1])
/* convert a word to two bytes according to LSB format */
#define FLOPW(ray, val)	(ray)[0] = ((val) / 256); (ray)[1] = ((val) & 0xFF)

/* get low byte from a word */
#define WORD_LOW(x)	    ((uint8)((uint16)(x) & 255))	
/* get high byte from a word */
#define WORD_HIGH(x)	((uint8)((uint16)(x) >> 8))	

/* convert a letter to capital format */
#define UPCASE(c)	    ( ((c) >= 'a' && (c) <= 'z') ? ((c) - 0x20) : (c) )

/* convert a ascii format digit to binary format */
#define ASC_TO_BIN(a)	    ( (a) - 0x30)

/* convert a binary format digit to ascii format */
#define BIN_TO_ASC(b)	    ( (b) + 0x30)

/* check a char whether it's a number */
#define IS_NUM(c)	    ( ((c) >= '0' && (c) <= '9') ? (1) : (0) )

/* increasing with preventing overflow */
#define INC_SAT(val) 	(val = ((val) + 1 > (val)) ? (val) + 1 : (val))

/* Macro to TEST the specified bit_operand(mask form) is Set or Clear status */
#define TESTBIT(operand_value, bit_operand) 	(((operand_value) & (bit_operand)) != ((bit_operand) - (bit_operand)))
/* Macro to SET the specified bit_operand */
#define SETBIT(operand_value, bit_operand) 	    ((operand_value) |= (bit_operand))
/* Macro to CLEAR the specified bit_operand */
#define CLEARBIT(operand_value, bit_operand)	((operand_value) &= ((uint32)~(bit_operand)))//? or ~((uint32)(bit_operand))

/* bit operation use MARCOs */ //note: default data type for complier
#define	BitTest(p, nBitOfs)	    ((p) & (0x1 << (nBitOfs)))
#define	BitGet(p, nBitOfs)	    (((p) & (0x1 << (nBitOfs))) >> (nBitOfs))
#define	BitSet(p, nBitOfs) 	    ((p) |= (0x1 << (nBitOfs)))
#define	BitClr(p, nBitOfs)  	((p) &= ~(0x1 << (nBitOfs)))
/* 32 Bits data series bit operation use MARCOs */ //note: integral promotion
#define	BitTest32(p, nBitOfs)	((p) & ((uint32)0x1 << (nBitOfs)))
#define	BitGet32(p, nBitOfs)	(((p) & ((uint32)0x1 << (nBitOfs))) >> (nBitOfs))
#define	BitSet32(p, nBitOfs) 	((p) |= ((uint32)0x1 << (nBitOfs)))
#define	BitClr32(p, nBitOfs)  	((p) &= ~((uint32)0x1 << (nBitOfs)))


  
/*************************extern function declaration**************************/
extern 	uint16	CRC16(uint8 *puchMsg, uint8 usDataLen);


#ifdef __cplusplus 
} 
#endif 

#endif
