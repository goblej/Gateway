/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "Particle.h"
#include "hexDump.h"
#include "build.h"
#include "eeprom.h"
#include "uart.h"
#include "panel_protocol.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "logging.h"
LOG_SOURCE_CATEGORY("fm.sys")



/****************************************************************************
 * Globals
 ****************************************************************************/
/* General purpose string resource for console messages */
//char errorStr [100];

// Offset in the EEPROM where our config data is held
extern const int particleEepromDataOffset;

/****************************************************************************
 * Pointer to a function used to process incoming serial data
 * If applicable, the pointer is assigned to the appropriate protocol
 * handler during protocol initialisation
 * Can be null if no serial protocol is configured
 ****************************************************************************/
extern protocolRxHandler_t protocolRxHandler;

/* Resources for building incoming events */
extern uint8_t serialRxState;
extern uint8_t serialRxBuffer [];
extern uint16_t serialRxLength;
extern uint32_t totalDiscardedBytes;

/* Configuration parameters all held in one structure */
/* Retrieved from and written back to EEPROM as required */
extern particleEepromData_t particleEepromData;

// Used to power up the isolated serial interface
extern int pwrEnablePin;

/****************************************************************************
 * Lookup table to get ASCII descriptor from uart framing Id
 ****************************************************************************/
extern const serialFramingTable_t serialFramingTable[];

/****************************************************************************
 * Prototypes
 ****************************************************************************/

/* Protocol-specific handler for incoming serial data */
void serialRxAdvancedAscii ( uint8_t rxChar );

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/**********************************************************************************
 * Expected message length 
 * The first line has a fixed length and is always 16 characters (14 plus <cr><lf>) 
 * The next three lines are always 42 characters each (40 plus <cr><lf>).
 * The next two lines are optional and each have a variable length up to a 
 * maximum of 42 characters 
 * We have also allowed a contingency of a further two lines of 42 characters each 
 * Finally the end of the message is always marked by a blank line, ie just the 
 * two-character <cr><lf> pair 
 * This gives us a minimum/maximum event message length of 144/312 characters 
 * respectively.
 * For estimating purposes a more typical range would be between 144 and 200 characters 
**********************************************************************************/
#define ADVANCED_ASCII_MAX_LINES 8
#define ADVANCED_ASCII_LINE_LENGTH 42
#define ADVANCED_ASCII_MAX_LENGTH 312

/****************************************************************************
 * serialStartAdvanced
 * Protocol-specific function to start this protocol
 * - Assign the protocol handler
 * - Assign and start any other resources such as timers
 ****************************************************************************/
void serialStartAdvancedASCII ( void )
{
	/* Assign the protocol handler */
	protocolRxHandler = serialRxAdvancedAscii;

	/* This handler uses the serial port */
	Log.info( "Baud rate: %lu, framing %s", 
				particleEepromData.panelSerialBaud, 
				serialFramingTable [ particleEepromData.serialFramingId ].label );

	/* Start the isolated power to the UART */
	digitalWrite(pwrEnablePin, HIGH);

	/* Start the UART */
	Serial1.begin (particleEepromData.panelSerialBaud, serialFramingTable [ particleEepromData.serialFramingId ].regValue);
}

/****************************************************************************
 * serialStopAdvanced
 * Protocol-specific function to stop this protocol
 * - Release the protocol handler
 * - Stop and release any other resources such as timers
 ****************************************************************************/
void serialStopAdvancedASCII ( void )
{
	/* Release the protocol handler */
	protocolRxHandler = nullptr;

	/* And stop */
	/* This disables the UART and releases the Tx and Rx pins for use as GPIO */
	Serial1.end();

	/* Shutdown the isolated power to the UART */
	digitalWrite(pwrEnablePin, LOW);
}

/****************************************************************************
 * serialRxAdvanced
 * Protocol-specific function for handling incoming
 * serial data
 ****************************************************************************/
void serialRxAdvancedAscii ( uint8_t rxChar )
{
    static uint8_t lineCount;
    static uint8_t lineCharCount = 0;
    static uint8_t previousChar;

	switch ( serialRxState )
	{
	case 0:
		/* Idle State */
		/* There might be one or more blank lines before we see the first */
		/* non-blank line that denotes a possible event of interest */

		/* Always collect the characters */
		lineCharCount++;
		serialRxBuffer [serialRxLength++] = rxChar;
		
		/* Check for end of line */
		if ( rxChar == 0x0a ){
			if (lineCharCount > 2 ){
				/* We have our first line with at least one character */
                /* preceding the <cr><lf> pair */
				lineCount = 1;

				/* Get ready to collect next and any subsequent lines */
				lineCharCount = 0;
				serialRxState = 1;
			}
			else{
                /* lineCharCount <= 2 */
				/* Almost certainly <cr><lf>, or something weird has happened */
				/* Either way we can regard this as a blank/empty line */
				/* Discard everything and keep waiting */
				serialRxLength = 0;
				lineCharCount = 0;
			}
		}
		else if ( lineCharCount > ADVANCED_ASCII_LINE_LENGTH ){
			/* This line is too long */
			/* Discard everything and keep waiting */
			Serial.printf ( "Line too long - Discarding %u bytes\r\n", serialRxLength );

			lineCharCount = 0;
			serialRxLength = 0;
		}
		
		/* Remember this so that we find consecutive <cr><lf> pairs */
		previousChar = rxChar;
 		break;

	case 1:
		/* Keep collecting subsequent characters/lines */
		/* until we get a blank/empty line */
		lineCharCount++;
		serialRxBuffer [serialRxLength++] = rxChar;
		
		/* Check for end of line */
		if ( rxChar == 0x0a ){
			if (lineCharCount > 2 ){
				/* We have another consecutive line with at least one character */
				lineCount++;
				if ( lineCount > ADVANCED_ASCII_MAX_LINES ){
					/* Overall message has too many lines */
					/* Regard the whole message as corrupt */
					Serial.printf ( "Too many lines - Discarding %u bytes\r\n", serialRxLength );

					/* Wait for the blank line that denotes the end of this message */
					lineCharCount = 0;
					serialRxState = 2;
					break;
				}
				else{
					/* Keep collecting on potential new line */
					lineCharCount = 0;
				}
			}
			else if ((lineCharCount == 2 ) && (previousChar == 0x0d)){
				/* Exactly two characters <cr><lf> denoting blank/empty line */
				/* Which means the end of this message */

				/* Validate and filter the content */
				/* There's a lot we could do here, such as checking line */
				/* lengths and the format of fixed-format lines          */
				/* But I don't think its necessary */
				/* I certainly don't want to replicate the original handler */
				/* which is just bizzarre */
				/* And no filtering is required - just send everything */
//				Serial.printf ( "New message, %u lines\r\n", lineCount );
				serialTxEventToNimbus ( );
				
				/* Get ready for next message */
				lineCharCount = 0;
				serialRxLength = 0;
				serialRxState = 0;
			}
			else{
				/* Line is corrupt */
				Serial.printf ( "Format error - Discarding %u bytes\r\n", serialRxLength );

				/* Wait for the blank line that denotes the end of this message */
				lineCharCount = 0;
				serialRxState = 2;
			}
		}
		else if ( lineCharCount > ADVANCED_ASCII_LINE_LENGTH ){
			/* This line is too long */
			/* Regard the whole message as corrupt */
			Serial.printf ( "Line too long - Discarding %u bytes\r\n", serialRxLength );

			/* Wait for the blank line that denotes the end of this message */
			lineCharCount = 0;
			serialRxState = 2;
		}
		else if ( serialRxLength >= ADVANCED_ASCII_MAX_LENGTH ){
			/* Overall message is too long */
			/* Regard the whole message as corrupt */
			Serial.printf ( "Message too long - Discarding %u bytes\r\n", serialRxLength );

			/* Wait for the blank line that denotes the end of this message */
			lineCharCount = 0;
			serialRxState = 2;
		}
		
		/* Remember this so that we can easily find <cr><lf> pairs */
		previousChar = rxChar;
 		break;

	case 2:
		/* An error has occurred */
		/* The current message has format errors */

		/* Wait here for first blank line, which we can use as a Reset */
		/* Don't collect the characters (we don't need them) */
		/* But keep count of characters per line */
		lineCharCount++;

		if ( rxChar == 0x0a ){
			if (lineCharCount > 2 ){
				/* This wasn't a blank line */
				lineCharCount = 0;
			}
			else if ((lineCharCount == 2 ) && (previousChar == 0x0d)){
				/* Exactly two characters <cr><lf> denoting blank/empty line */
				/* Which means the end of this message */

				/* Get ready for next message */
				lineCharCount = 0;
				serialRxLength = 0;
				serialRxState = 0;
			}
		}

		/* Remember this so that we can easily find <cr><lf> pairs */
		previousChar = rxChar;
 		break;

	default:
		/* Just in case */
		lineCharCount = 0;
		serialRxLength = 0;
		serialRxState = 0;
		break;
	}
}
