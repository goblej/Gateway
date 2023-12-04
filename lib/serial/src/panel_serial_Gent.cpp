/*****************************************************************************
 *
 * Copyright (C) 2021 LAN Control Systems Ltd
 *
 * All rights thereto are reserved. Use or copying of all or any portion
 * of this program is prohibited except with express written authorisation
 * from LAN Control Systems Ltd
 *
 ******************************************************************************
 *
 * Module :     panel_serial_Gent
 * Description: Supporting serial funtions for the Advanced fire panel
 *              printer port (ASCII)
 *              - handle all incoming serial data and extract any events
 *              - Handle incoming serial events
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "Particle.h"
#include "panel_serial.h"

/****************************************************************************
 * Globals
 ****************************************************************************/

/****************************************************************************
 * Pointer to a function used to process incoming serial data
 * If applicable, the pointer is assigned to the appropriate protocol 
 * handler during protocol initialisation
 * Can be null if no serial protocol is configured
 ****************************************************************************/
extern serialRxHandler_t serialRxHandler;

/* Resources for building incoming events */
extern uint8_t serialRxState;
extern uint8_t serialRxBuffer [];
extern uint16_t serialRxLength;

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/* Allow generous 5s for complete frame */
#define GENT_MESSAGE_FRAME_TIME	5

/* A Gent message (either null or event) is expected at least once every minute */
#define GENT_MESSAGE_INTERVAL 60

#define ASCII_ACK 0x06
#define ASCII_NAK 0x15

/*****************************************************************************
 * Data
 *****************************************************************************/

/* Every Gent message requires one of two fixed responses, corresponding to ACK or NAK */
/* Each comprises two data bytes plus 16-bit checksum */
const uint8_t gentAckResponse [] = { 0x00, ASCII_ACK, 0x00, ASCII_ACK };
const uint8_t gentNakResponse [] = { 0x00, ASCII_NAK, 0x00, ASCII_NAK };

/*****************************************************************************
 * Prototypes
 *****************************************************************************/

/* Protocol-specific handler for incoming serial data */
void serialRxGent ( uint8_t rxChar );

/* Protocol-specific handler for verifying first two bytes of a Gent packet */
bool rxGentValidateEventPair ( void );

/****************************************************************************
 * serialStartGent
 * Protocol-specific function to start this protocol
 * - Assign the protocol handler
 * - Assign and start any other resources such as timers
 ****************************************************************************/
void serialStartGent ( void )
{
	/* Assign the UART handler */
	serialRxHandler = serialRxGent;
}

/****************************************************************************
 * serialStopGent
 * Protocol-specific function to stop this protocol
 * - Release the protocol handler
 * - Stop and release any other resources such as timers
 ****************************************************************************/
void serialStopGent ( void )
{
	/* Release the UART handler */
	serialRxHandler = nullptr;
}

/****************************************************************************
 * serialProcessGent
 * Protocol-specific function to process any incoming
 * serial event data
 ****************************************************************************/
void serialProcessGent ( void )
{
	/* Nothing to process */
	/* Just forward whole thing to Nimbus */
	serialTxEventToNimbus ( );
}

/****************************************************************************
 * serialRxGent
 * Protocol-specific function for handling incoming
 * serial data
 ****************************************************************************/
void serialRxGent ( uint8_t rxChar )
{
	static uint8_t i, rxExpectedLen;
	static uint16_t rxChecksum;

	switch ( serialRxState )
	{
	case 0:
		/* Start of Gent packet                       */
		/* We're only interested in Event and ACK/NAK */
		/* packets                                    */
		/* First two bytes are the Event Code:        */
		/* MSB   LSB                                  */
		/* 0     1     Fire Reset                     */
		/* 0     2     All faults cleared             */
		/* 0     3     All disablements cleared       */
		/* 0     4     Alarms silenced                */
		/* 0     5     Alarms sounded                 */
		/* 0     6     Ack                            */
		/* 0    21     Nak                            */
		/* 2     1     Supervisory on                 */
		/* 2     2     Supervisory off                */
		/* 4     x     Fault- System                  */
		/* 5     x     Fault- Outstation/Loop         */
		/* 7     x     Disablement                    */
		/* 9     x     Fire                           */
		/* 10    x     Super Fire                     */
		/* 18    x     Cancel buzzer (not documented) */
		if ( rxChar > 0x12 )
		{
			/* Event MSB out of range */
			/* Definitely not a valid start byte */
			/* Skip */
			return;
		}
		else
		{
			/* Event MSB is within range, but is it valid ? */
			/* No way to be certain yet, but assume valid for now and start collecting bytes */
			/* Messy, but Gent does not specify a unique start-of-packet byte value */
			i = 0;
			serialRxState = 1;
			serialRxBuffer [i++] = rxChar;
			rxChecksum = ( uint16_t ) rxChar;
		}
		break;

	case 1:
		/* Event LSB of Gent packet */
		/* Keep building the Gent packet, updating the checksum as we go */
		serialRxBuffer [i++] = rxChar;
		rxChecksum += ( uint16_t ) rxChar;

		/* We have the first two bytes, the Gent Event bytes */
		/* Validate against the known Gent Event types */
		if ( rxGentValidateEventPair () )
		{
			/* Event pair was in range, so assume this is either  */
			/* a Gent event packet or ACK/NAK packet              */
			/* At this point we can determine the expected length */
			/* Length depends on packet type                      */
			/* Gent ACK and NAK packets are 4 bytes               */
			/* Gent Event packets are always 59 bytes             */
			if ( ( rxChar == ASCII_ACK ) || ( rxChar == ASCII_NAK ) )
			{
				/* Gent ACK/NAK packet */
				/* We already have the main message body */
				/* So we're finished with our checksum */
				/* Next byte will be the first checksum byte */
				serialRxState = 3;
			}
			else
			{
				/* Gent Event packet */
				/* These are fixed length at 59 bytes, so 57 to go */
				/* Or 56 since our index starts at zero */
				rxExpectedLen = 56;

				/* Keep building the Gent packet */
				serialRxState = 2;
			}
		}
		else
		{
			/* First byte pair out of range */
			/* Either we're out of sync or this is not a Gent packet of interest */
			/* Restart */
			serialRxState = 0;
		}
		break;

	case 2:
		/* Come here if we're building the 59-byte Gent events */
		if ( i < rxExpectedLen )
		{
			/* Keep building the Gent packet */
			serialRxBuffer [i++] = rxChar;
		}
		else
		{
			/* We now have the main message body */
			/* So we're finished with our checksum */
			/* Next byte will be the first checksum byte */
			serialRxState = 3;

			/* Keep building the Gent packet */
			serialRxBuffer [i++] = rxChar;
		}

		/* And updating the checksum */
		rxChecksum += ( uint16_t ) rxChar;
		break;

	case 3:
		/* Come here for the first checksum byte (MSB) */
		if ( rxChar == ( uint8_t ) ( rxChecksum >> 8 ) )
		{
			/* OK so far, check next byte */				
			serialRxState = 4;

			/* Keep building the Gent packet */
			serialRxBuffer [i++] = rxChar;
		}
		else
		{
			/* Checksum error */
			/* Ready for next packet */				
			serialRxState = 0;
		}
		break;

	case 4:
		/* Second of two checksum bytes (LSB) */
		if ( rxChar == ( uint8_t ) ( rxChecksum & 0x00ff ) )
		{
			/* Not really necessary, but for completeness */
			/* Put the last byte into the Gent packet */
			serialRxBuffer [i++] = rxChar;

			/* Valid Gent packet received */
			/* Ignore the short ACK/NAK packets (4 bytes) */
			/* But process the event packets (59 bytes)   */
			if ( i == 59 )
			{
				serialRxLength = i;
				serialProcessGent ();
			}
		}

		/* Success or fail, ready for next packet */
		serialRxState = 0;
		break;
	}
}

/****************************************************************************
 * rxGentValidateEventPair
 * Protocol-specific handler for verifying first two bytes of a Gent packet
 ****************************************************************************/
bool rxGentValidateEventPair ( void )
{
	/* Switch on MSB */
	switch ( serialRxBuffer [ 0 ] )
	{
	case 0:
		/* Event LSB must be 1-5, 6 (ACK) or 21 (NAK) */
		if ( ( ( serialRxBuffer [ 1 ] >= 1 ) && ( serialRxBuffer [ 1 ] <= 6 ) )
			|| ( serialRxBuffer [ 1 ] == ASCII_NAK ) )
			return true;
		break;

	case 2:
		/* Event LSB must be 1 or 2 */
		if ( ( serialRxBuffer [ 1 ] == 1 ) || ( serialRxBuffer [ 1 ] == 2 ) )
			return true;
		break;

	case 4:
	case 5:
	case 7:
	case 9:
	case 10:
	case 18:
		/* Event LSB can take any value */
		return true;
		break;
	}

	/* Event must be invalid */
	return false;
}
