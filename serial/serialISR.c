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
  BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR USART0. 

  This file contains all the serial port components that must be compiled
  to ARM mode.  The components that can be compiled to either ARM or THUMB
  mode are contained in serial.c.

*/

/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* Demo application includes. */
#include "serial.h"

/*-----------------------------------------------------------*/

/* Constant to access the AIC. */
#define serCLEAR_AIC_INTERRUPT      ( ( unsigned portLONG ) 0 )

/* Constants to determine the ISR source. */
#define serSOURCE_THRE				( ( unsigned portCHAR ) 0x02 )
#define serSOURCE_RX_TIMEOUT		( ( unsigned portCHAR ) 0x0c )
#define serSOURCE_ERROR				( ( unsigned portCHAR ) 0x06 )
#define serSOURCE_RX				( ( unsigned portCHAR ) 0x04 )
#define serINTERRUPT_SOURCE_MASK    ( ( unsigned portLONG ) (US_RXRDY | US_TXRDY | US_RXBRK | US_OVRE | US_FRAME | US_PARE) )

/* Queues used to hold received characters, and characters waiting to be
transmitted. */
static xQueueHandle xRxedChars; 
static xQueueHandle xCharsForTx; 

/*-----------------------------------------------------------*/
void vSerialISRCreateQueues( unsigned portBASE_TYPE uxQueueLength, xQueueHandle *pxRxedChars, xQueueHandle *pxCharsForTx )
{
	/* Create the queues used to hold Rx and Tx characters. */
	xRxedChars = xQueueCreate( uxQueueLength, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );
	xCharsForTx = xQueueCreate( uxQueueLength + 1, ( unsigned portBASE_TYPE ) sizeof( signed portCHAR ) );

	/* Pass back a reference to the queues so the serial API file can 
	post/receive characters. */
	*pxRxedChars = xRxedChars;
	*pxCharsForTx = xCharsForTx;
}
/*-----------------------------------------------------------*/

/* Serial port ISR.  This can cause a context switch so is not defined as a
standard ISR using the __irq keyword.  See the port documentation on the
FreeRTOS.org website for more information. */
void vSerialISR( void )
{
	/* This ISR can cause a context switch, so the first statement must be a
	call to the portENTER_SWITCHING_ISR() macro.  This must be BEFORE any
	variable declarations. */
	portENTER_SWITCHING_ISR();

unsigned portLONG ulStatus;
signed portCHAR cChar;
portBASE_TYPE xTaskWokenByTx = pdFALSE, xTaskWokenByPost = pdFALSE;

	/* What caused the interrupt? */
	ulStatus = serUSART0->US_CSR &= serUSART0->US_IMR;

	if( ulStatus & AT91C_US_TXRDY )
	{
		/* The interrupt was caused by the THR becoming empty.  Are there any
		more characters to transmit? */
		if( xQueueReceiveFromISR( xCharsForTx, &cChar, &xTaskWokenByTx ) == pdTRUE )
		{
			/* A character was retrieved from the queue so can be sent to the
			THR now. */
			serUSART0->US_THR = cChar;
		}
		else
		{
			/* Queue empty, nothing to send so turn off the Tx interrupt. */
			vCOMInterruptOff( serUSART0 );
		}		
	}

	if( ulStatus & AT91C_US_RXRDY )
	{
		/* The interrupt was caused by a character being received.  Grab the
		character from the RHR and place it in the queue or received 
		characters. */
		cChar = serUSART0->US_RHR;
		xTaskWokenByPost = xQueueSendFromISR( xRxedChars, &cChar, xTaskWokenByPost );
	}

	/* End the interrupt in the AIC. */
	AT91C_BASE_AIC->AIC_EOICR = 0;

	/* If a task was woken by either a character being received or a character 
	being transmitted then we may need to switch to another task. */
	portEXIT_SWITCHING_ISR( ( xTaskWokenByPost || xTaskWokenByTx ) );
}	
/*-----------------------------------------------------------*/

