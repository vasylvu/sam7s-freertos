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

/* 
	BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR UART0. 
*/

/* Standard includes. */ 
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"

/* Demo application includes. */
#include "serial.h"

/*-----------------------------------------------------------*/

/* Misc constants. */
#define serINVALID_QUEUE				( ( xQueueHandle ) 0 )
#define serHANDLE						( ( xComPortHandle ) 1 )
#define serNO_BLOCK						( ( portTickType ) 0 )
#define serNO_TIMEGUARD					( ( unsigned portLONG ) 0 )
#define serNO_PERIPHERAL_B_SETUP		( ( unsigned portLONG ) 0 )


/* Queues used to hold received characters, and characters waiting to be
transmitted. */
static xQueueHandle xRxedChars; 
static xQueueHandle xCharsForTx; 

/*-----------------------------------------------------------*/

/*
 * See the serial2.h header file.
 */
xComPortHandle xSerialPortInitMinimal( unsigned portLONG ulWantedBaud, unsigned portBASE_TYPE uxQueueLength )
{
xComPortHandle xReturn = serHANDLE;

	/* The queues are used in the serial ISR routine, so are created from
	serialISR.c (which is always compiled to ARM mode. */
	vSerialISRCreateQueues( uxQueueLength, &xRxedChars, &xCharsForTx );

	/* If the queues were created correctly then setup the serial port 
	hardware. */
	if( ( xRxedChars != serINVALID_QUEUE ) && ( xCharsForTx != serINVALID_QUEUE ) )
	{
		portENTER_CRITICAL();
		{
			/* Enable the USART clock. */
			AT91F_PMC_EnablePeriphClock( AT91C_BASE_PMC, 1 << AT91C_ID_US0 );

			AT91F_PIO_CfgPeriph( AT91C_BASE_PIOA, ( ( unsigned portLONG ) AT91C_PA5_RXD0 ) | ( ( unsigned portLONG ) AT91C_PA6_TXD0 ), serNO_PERIPHERAL_B_SETUP );

			/* Set the required protocol. */
			AT91F_US_Configure( serUSART0, configCPU_CLOCK_HZ, AT91C_US_ASYNC_MODE, ulWantedBaud, serNO_TIMEGUARD );

			/* Enable Rx and Tx. */
			serUSART0->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;

			/* Enable the Rx interrupts.  The Tx interrupts are not enabled
			until there are characters to be transmitted. */
    		AT91F_US_EnableIt( serUSART0, AT91C_US_RXRDY );

			/* Enable the interrupts in the AIC. */
			AT91F_AIC_ConfigureIt( AT91C_BASE_AIC, AT91C_ID_US0, serINTERRUPT_LEVEL, AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE, ( void (*)( void ) ) vSerialISR );
			AT91F_AIC_EnableIt( AT91C_BASE_AIC, AT91C_ID_US0 );
		}
		portEXIT_CRITICAL();
	}
	else
	{
		xReturn = ( xComPortHandle ) 0;
	}

	/* This demo file only supports a single port but we have to return 
	something to comply with the standard demo header file. */
	return xReturn;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed portCHAR *pcRxedChar, portTickType xBlockTime )
{
	/* The port handle is not required as this driver only supports one port. */
	( void ) pxPort;

	/* Get the next character from the buffer.  Return false if no characters
	are available, or arrive before xBlockTime expires. */
	if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
	{
		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}
/*-----------------------------------------------------------*/

void vSerialPutString( xComPortHandle pxPort, const signed portCHAR * const pcString, unsigned portSHORT usStringLength )
{
signed portCHAR *pxNext;

	/* A couple of parameters that this port does not use. */
	( void ) usStringLength;
	( void ) pxPort;

	/* NOTE: This implementation does not handle the queue being full as no
	block time is used! */

	/* The port handle is not required as this driver only supports UART0. */
	( void ) pxPort;

	/* Send each character in the string, one at a time. */
	pxNext = ( signed portCHAR * ) pcString;
	while( *pxNext )
	{
		xSerialPutChar( pxPort, *pxNext, serNO_BLOCK );
		pxNext++;
	}
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed portCHAR cOutChar, portTickType xBlockTime )
{
	/* Place the character in the queue of characters to be transmitted. */
	if( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) != pdPASS )
	{
		return pdFAIL;
	}

	/* Turn on the Tx interrupt so the ISR will remove the character from the
	queue and send it.   This does not need to be in a critical section as
	if the interrupt has already removed the character the next interrupt
	will simply turn off the Tx interrupt again. */
	vCOMInterruptOn( serUSART0 );

	return pdPASS;
}
/*-----------------------------------------------------------*/

void vSerialClose( xComPortHandle xPort )
{
	/* Not supported as not required by the demo application. */
}
/*-----------------------------------------------------------*/

