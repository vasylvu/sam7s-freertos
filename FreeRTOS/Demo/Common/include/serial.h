/*
	FreeRTOS.org V4.1.0 - Copyright (C) 2003-2006 Richard Barry.

	This file is part of the FreeRTOS.org distribution.

	FreeRTOS.org is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	FreeRTOS.org is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with FreeRTOS.org; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	A special exception to the GPL can be applied should you wish to distribute
	a combined work that includes FreeRTOS.org, without being obliged to provide
	the source code for any proprietary components.  See the licensing section 
	of http://www.FreeRTOS.org for full details of how and when the exception
	can be applied.

	***************************************************************************
	See http://www.FreeRTOS.org for documentation, latest information, license 
	and contact details.  Please ensure to read the configuration and relevant 
	port sections of the online documentation.
	***************************************************************************
*/

#ifndef SERIAL_COMMS_H
#define SERIAL_COMMS_H

/* Location of the COM0 and COM1 registers. */
#define serUSART0							( ( AT91PS_USART ) AT91C_BASE_US0 )
#define serUSART1							( ( AT91PS_USART ) AT91C_BASE_US1 )

/* Interrupt control macros. */
#define serINTERRUPT_LEVEL				( 5 )
#define vCOMInterruptOn( port )		AT91F_US_EnableIt( port, AT91C_US_TXRDY | AT91C_US_RXRDY )
#define vCOMInterruptOff( port )		AT91F_US_DisableIt( port, AT91C_US_TXRDY )

typedef void * xComPortHandle;

typedef enum
{ 
	serCOM1, 
	serCOM2, 
	serCOM3, 
	serCOM4, 
	serCOM5, 
	serCOM6, 
	serCOM7, 
	serCOM8 
} eCOMPort;

typedef enum 
{ 
	serNO_PARITY, 
	serODD_PARITY, 
	serEVEN_PARITY, 
	serMARK_PARITY, 
	serSPACE_PARITY 
} eParity;

typedef enum 
{ 
	serSTOP_1, 
	serSTOP_2 
} eStopBits;

typedef enum 
{ 
	serBITS_5, 
	serBITS_6, 
	serBITS_7, 
	serBITS_8 
} eDataBits;

typedef enum 
{ 
	ser50,		
	ser75,		
	ser110,		
	ser134,		
	ser150,    
	ser200,
	ser300,		
	ser600,		
	ser1200,	
	ser1800,	
	ser2400,   
	ser4800,
	ser9600,		
	ser19200,	
	ser38400,	
	ser57600,	
	ser115200,
	ser230400
} eBaud;

xComPortHandle xSerialPortInitMinimal( unsigned portLONG ulWantedBaud, unsigned portBASE_TYPE uxQueueLength );
xComPortHandle xSerialPortInit( eCOMPort ePort, eBaud eWantedBaud, eParity eWantedParity, eDataBits eWantedDataBits, eStopBits eWantedStopBits, unsigned portBASE_TYPE uxBufferLength );
void vSerialPutString( xComPortHandle pxPort, const signed portCHAR * const pcString, unsigned portSHORT usStringLength );
signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed portCHAR *pcRxedChar, portTickType xBlockTime );
signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed portCHAR cOutChar, portTickType xBlockTime );
portBASE_TYPE xSerialWaitForSemaphore( xComPortHandle xPort );
void vSerialClose( xComPortHandle xPort );

/* 
 * The queues are created in serialISR.c as they are used from the ISR.
 * Obtain references to the queues and THRE Empty flag. 
 */
void vSerialISRCreateQueues( unsigned portBASE_TYPE uxQueueLength, xQueueHandle *pxRxedChars, xQueueHandle *pxCharsForTx );

/* UART0 interrupt service routine.  This can cause a context switch so MUST
be declared "naked". */
void vSerialISR( void ) __attribute__((naked));

#endif

