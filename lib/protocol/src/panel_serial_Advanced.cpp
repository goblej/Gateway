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

/****************************************************************************
 * Globals
 ****************************************************************************/
/* General purpose string resource for console messages */
 char errorStr [100];

// Offset in the EEPROM where our config data is held
extern const int particleEepromDataOffset;

/****************************************************************************
 * Pointer to a function used to process incoming serial data
 * If applicable, the pointer is assigned to the appropriate protocol
 * handler during protocol initialisation
 * Can be null if no serial protocol is configured
 ****************************************************************************/
extern protocolRxHandler_t protocolRxHandler;

void advancedPacketReceptionError ( const char * str );
bool validateCRC ( uint8_t * buf, uint16_t len );
void advancedAddClashCodes( uint8_t *pbuf_in, uint8_t *pbuf_out, uint16_t *pbuf_len );
void advancedCrcUpdate ( uint8_t *buff, uint16_t count, uint8_t *crc_hi, uint8_t *crc_lo );
bool validateAdvancedBmsPacket ( void );
bool checkPacketFormat ( void );

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
void serialRxAdvanced ( uint8_t rxChar );

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/****************************************************************************
 * Shortest valid packet is 'Network Configuration Change' at 12 bytes
 ****************************************************************************/
#define ADVANCED_MIN_LENGTH 12

/****************************************************************************
 * From the specification we know that the longest valid packet is 108 bytes:
 * - Start of Message
 * - Packet Identity
 * - Destination Address
 * - Source Address
 * - Packet Sequence Number
 * - Payload (100 bytes max)
 * - CRC high
 * - CRC low
 * - End of Message
 *
 * So 5 header bytes + 100 bytes max. payload length + 3 following bytes
 *
 * Use this upper bound to make sure the state machine doesn't gather bytes
 * indefinitely, which would cause problems
 ****************************************************************************/
#define ADVANCED_MAX_LENGTH 108

/****************************************************************************
 * The Advanced BMS protocol handler needs to periodically poll the Advanced
 * panel in order to verify the comms path. It does this by sending the
 * 'Request Node Status' packet as recommended in the Advanced  BMS specification
 * The panel should then reply with a suitable valid response which must itself
 * be acknowledged in the usual way
 ****************************************************************************/
const uint8_t requestNodeStatus [] = { 	0xfe,   /* Start of Message (SOM)        */
                                     	0x80,   /* Packet Identity (always 0x80) */
                                        0x00,   /* Destination address           */
                                        0x00,   /* Source address                */
                                        0x01,   /* Packet sequence number        */
                                        /* Start of payload                      */
                                        0x2a,   /* Request Node Status           */
                                        0x03,   /* Length                        */
                                        0x01,   /* Network Node                  */
                                        /* End of payload                        */
                                        0xf0,   /* No more messages              */
                                        0x8c,   /* CRC high byte                 */
                                        0x67,   /* CRC low byte                  */
                                        0xff }; /* End of Message (EOM)          */

/****************************************************************************
 * CRC Tables provided by Advanced BMS specification
 ****************************************************************************/
const uint8_t crc_table_low [] =
{	0x00, 0xc0, 0xc1, 0x01, 0xc3, 0x03, 0x02, 0xc2, 0xc6, 0x06, 0x07, 0xc7, 0x05, 0xc5, 0xc4, 0x04,
	0xcc, 0x0c, 0x0d, 0xcd, 0x0f, 0xcf, 0xce, 0x0e, 0x0a, 0xca, 0xcb, 0x0b, 0xc9, 0x09, 0x08, 0xc8,
	0xd8, 0x18, 0x19, 0xd9, 0x1b, 0xdb, 0xda, 0x1a, 0x1e, 0xde, 0xdf, 0x1f, 0xdd, 0x1d, 0x1c, 0xdc,
	0x14, 0xd4, 0xd5, 0x15, 0xd7, 0x17, 0x16, 0xd6, 0xd2, 0x12, 0x13, 0xd3, 0x11, 0xd1, 0xd0, 0x10,
	0xf0, 0x30, 0x31, 0xf1, 0x33, 0xf3, 0xf2, 0x32, 0x36, 0xf6, 0xf7, 0x37, 0xf5, 0x35, 0x34, 0xf4,
	0x3c, 0xfc, 0xfd, 0x3d, 0xff, 0x3f, 0x3e, 0xfe, 0xfa, 0x3a, 0x3b, 0xfb, 0x39, 0xf9, 0xf8, 0x38,
	0x28, 0xe8, 0xe9, 0x29, 0xeb, 0x2b, 0x2a, 0xea, 0xee, 0x2e, 0x2f, 0xef, 0x2d, 0xed, 0xec, 0x2c,
	0xe4, 0x24, 0x25, 0xe5, 0x27, 0xe7, 0xe6, 0x26, 0x22, 0xe2, 0xe3, 0x23, 0xe1, 0x21, 0x20, 0xe0,
	0xa0, 0x60, 0x61, 0xa1, 0x63, 0xa3, 0xa2, 0x62, 0x66, 0xa6, 0xa7, 0x67, 0xa5, 0x65, 0x64, 0xa4,
	0x6c, 0xac, 0xad, 0x6d, 0xaf, 0x6f, 0x6e, 0xae, 0xaa, 0x6a, 0x6b, 0xab, 0x69, 0xa9, 0xa8, 0x68,
	0x78, 0xb8, 0xb9, 0x79, 0xbb, 0x7b, 0x7a, 0xba, 0xbe, 0x7e, 0x7f, 0xbf, 0x7d, 0xbd, 0xbc, 0x7c,
	0xb4, 0x74, 0x75, 0xb5, 0x77, 0xb7, 0xb6, 0x76, 0x72, 0xb2, 0xb3, 0x73, 0xb1, 0x71, 0x70, 0xb0,
	0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54,
	0x9c, 0x5c, 0x5d, 0x9d, 0x5f, 0x9f, 0x9e, 0x5e, 0x5a, 0x9a, 0x9b, 0x5b, 0x99, 0x59, 0x58, 0x98,
	0x88, 0x48, 0x49, 0x89, 0x4b, 0x8b, 0x8a, 0x4a, 0x4e, 0x8e, 0x8f, 0x4f, 0x8d, 0x4d, 0x4c, 0x8c,
	0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40 };

const uint8_t crc_table_high [] =
{	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40, 0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41,
	0x00, 0xc1, 0x81, 0x40, 0x01, 0xc0, 0x80, 0x41, 0x01, 0xc0, 0x80, 0x41, 0x00, 0xc1, 0x81, 0x40 };

/*****************************************************************************
 * Identifier Code table entries
 *****************************************************************************/
typedef struct idCodeTableEntry
{
	const uint8_t	code;	// The identifier code
	uint8_t			count;	// Number of instances found in a packet
	const char 		*label;	// Descriptive label
} idCodeTable_t;

/****************************************************************************
 * Lookup table for Identifier Code information relating to the
 * Advanced BMS protocol
 ****************************************************************************/
idCodeTable_t idCodeTable[] =
{  /* Code  Count   Label */
	{ 0x01,	0x00,	"Acknowledgement" },
	{ 0x0A,	0x00,	"Device Status" },
	{ 0x0B,	0x00,	"Node Status" },
	{ 0x0C,	0x00,	"Network Configuration Change" },
	{ 0x0D,	0x00,	"Zone Text" },
	{ 0x0E,	0x00,	"Analogue Value" },
	{ 0x0F,	0x00,	"Output Activated / Deactivated by BMS" }
};

#define NUM_ID_CODES	7

/****************************************************************************
 * serialStartAdvanced
 * Protocol-specific function to start this protocol
 * - Assign the protocol handler
 * - Assign and start any other resources such as timers
 ****************************************************************************/
void serialStartAdvanced ( void )
{
	/* Assign the protocol handler */
	protocolRxHandler = serialRxAdvanced;

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
void serialStopAdvanced ( void )
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
void serialRxAdvanced ( uint8_t rxChar )
{
	switch ( serialRxState )
	{
	case 0:
		/* Idle State */
		/* Wait here until we get the unique 0xfe character denoting packet start */
		/* Ignore everything else */
		if ( rxChar == 0xfe )
		{
			/* We have the unique start-of frame byte */
			/* Start building the frame */
			serialRxLength = 0;
			serialRxState = 1;
			serialRxBuffer [serialRxLength++] = rxChar;
		}
		break;

	case 1:
		if ( rxChar == 0xff )
		{
			/* We have the unique end-of-frame byte */
			/* Append it to the others */
			serialRxBuffer [serialRxLength++] = rxChar;

			/* Potential good packet ready for processing */
			if ( validateAdvancedBmsPacket () )
			{
				/* Packet confirmed OK */
				/* Forward relevant packets to Nimbus */
				/* Which for now means any packets containing one or more 'Device Status' messages */
				if ( ( particleEepromData.verbose ) || ( idCodeTable[1].count > 0 ) ) {
					serialTxEventToNimbus ( );
				}
			}
			else
			{
				/* Packet has errors */
				Serial.write (errorStr);
				if ( particleEepromData.verbose  ) {
					/* Forward the packet anyway */
					serialTxEventToNimbus ( );
				}
				else {
					totalDiscardedBytes += serialRxLength;
					Serial.printf ( "Discarding %u bytes\r\n", serialRxLength );
					hexDump ( serialRxBuffer, (uint16_t)serialRxLength);
				}
			}

			/* Ready for next packet */
			serialRxState = 0;
		}
		else if ( rxChar == 0xfa )
		{
			/* Clash Code prefix                                                  */
			/* It is inevitable that a value in the message body may occasionally */
			/* clash with a reserved value (data value equal to or exceeds 0xfa)  */
			/* In these instances the clash code is sent followed by              */
			/* value - clash code. For example:                                   */
			/* - 0xfa is transmitted as 0xfa 0x00                                 */
			/* - 0xfb is transmitted as 0xfa 0x01                                 */
			/* - 0xfc is transmitted as 0xfa 0x02                                 */
			/* - 0xfd is transmitted as 0xfa 0x03                                 */
			/* - 0xfe is transmitted as 0xfa 0x04                                 */
			/* - 0xff is transmitted as 0xfa 0x05                                 */
			/* :                                                                  */
			/* This byte marks the start of an escape sequence */

			/* Skip this byte, and note that the next */
			/* character has to be substituted */
			serialRxState = 2;
		}
		else if ( ( rxChar > 0xfa ) && ( rxChar <= 0xfe ) )
		{
			/* Error - Unexpected Clash Code        */
			/* These values should not occur inside */
			/* the message body                     */
			advancedPacketReceptionError ( "Unexpected Clash Code" );

			/* Dump everything and start again      */
			totalDiscardedBytes += serialRxLength;
			serialRxState = 0;
		}
		else if ( serialRxLength >= ADVANCED_MAX_LENGTH )
		{
			/* Error - Packet Too Long */
			advancedPacketReceptionError ( "Packet Too Long" );

			/* Dump everything and start again */
			totalDiscardedBytes += serialRxLength;
			serialRxState = 0;
		}
		else
		{
			/* Next byte, append to the rx buffer */
			serialRxBuffer [serialRxLength++] = rxChar;
		}
		break;

	case 2:
		/* Previous byte must have been the clash code 0xfa */
		/* So this following byte should be in the range 0x00-0x05 */
		if ( rxChar > 0x05 )
		{
			/* Error - Invalid Clash Code */
			advancedPacketReceptionError ( "Invalid Clash Code" );

			/* Dump everything and start again */
			totalDiscardedBytes += serialRxLength;
			serialRxState = 0;
		}
		else
		{
			/* Append restored byte to the rx buffer */
			serialRxBuffer [serialRxLength++] = rxChar + 0xfa;

			/* Back to collecting regular bytes */
			serialRxState = 1;
		}
		break;

	default:
		/* Just in case */
		serialRxState = 0;
		break;
	}
}

void advancedPacketReceptionError ( const char * str )
{
	Serial.printf ( "Error during packet reception - %s\r\nDiscarding %u bytes\r\n", str, serialRxLength );

	if ( particleEepromData.verbose  ) {
		/* Forward what we have anyway */
		serialTxEventToNimbus ( );
	}
}

/****************************************************************************
 * advancedCrcUpdate
 * CRC routine provided by Advanced
 ****************************************************************************/
void advancedCrcUpdate ( uint8_t *buff, uint16_t count, uint8_t *crc_hi, uint8_t *crc_lo )
{
	uint16_t i;
	uint8_t table_index;

	for ( i = 0; i < count; i++ )
	{
		table_index = *crc_hi ^ buff [ i ];
		*crc_hi = *crc_lo ^ crc_table_high [ table_index ];
		*crc_lo = crc_table_low [ table_index ];
	}
}

/****************************************************************************
 * advancedAddClashCodes
 * When sending packets to the Advanced panel, any characters with value 0xFA
 * and above in the packet body must be substituted according to the
 * 'clash code' mechanism
 * - 0xfa is transmitted as 0xfa 0x00
 * - 0xfb is transmitted as 0xfa 0x01
 * - 0xfc is transmitted as 0xfa 0x02
 * - 0xfd is transmitted as 0xfa 0x03
 * - 0xfe is transmitted as 0xfa 0x04
 * - 0xff is transmitted as 0xfa 0x05
 *
 ****************************************************************************/
void advancedAddClashCodes( uint8_t *pbuf_in, uint8_t *pbuf_out, uint16_t *pbuf_len )
{
    uint16_t i, length;

	/* Clash code substitution only applies within the body of the packet, ignore first and last bytes */
    length = *pbuf_len - 2;

	/* Copy first byte without substitution */
	*pbuf_out++ = *pbuf_in++;

	/* If the outgoing message contains any special chars then substitute them with 0xFA, (char - 0xFA) */
	/* Special chars are 0xFA and above */
    for ( i = 0; i < length; i++ ){
        if ( *pbuf_in >= 0xfa ){
            *pbuf_out++ = 0xfa;
            *pbuf_out++ = ( *pbuf_in++ - 0xfa );

            /* One more char has been added so increment pbuf_len */
            ( *pbuf_len )++;
        }
        else *pbuf_out++ = *pbuf_in++;
    }

	/* Copy last byte without substitution */
	*pbuf_out = *pbuf_in;
}

bool validateCRC ( uint8_t * buf, uint16_t len )
{
    uint8_t crcHigh, crcLow;

	crcLow  = 0xff;
	crcHigh = 0xff;
	advancedCrcUpdate ( buf + 1, len - 4, &crcHigh, &crcLow );
	if ( ( buf [ len - 3 ] == crcHigh ) && ( buf [ len - 2 ] == crcLow ) ) return true;
	else return false;
}

/****************************************************************************
 * checkPacketFormat
 * Protocol-specific function to validate the
 * packet format
 *
 * The payload within the packet will comprise one or more messages, up to a
 * maximum overall length of 100 bytes
 * Each message has an Identifier Code byte followed by a length byte
 * An identifier code of 0xF0 denotes no more messages
 *
 * This routine verifies the first Identifier Code byte and then uses the
 * following length byte to find the next Identifier Code, repeating until
 * either:
 * - we find an Identifier Code of 0xF0 (No More Messages), or
 * - it turns out that the overall length of the payload is going to exceed
 *   the maximum
 *
 * Individual counts of each Identifier Code type are maintained, so that we
 * can later determine whether or not the packet contains content that is of
 * interest to Nimbus
 *
 * Returns a true/false result
 *
 ****************************************************************************/
bool checkPacketFormat ( void )
{
	uint8_t code, i, offset, maxOffset;
    idCodeTable_t *pidCodeTable = idCodeTable;

	/* Determine the maximum expected offset based on overall packet length
     * Ignoring the 'No More Messages' byte, the two CRC bytes and the
     * 'End of Message' byte
     ************************************************************************/
	maxOffset = serialRxLength - 4;

    /* Clear all of the count values in the Identifier Code table */
	for (i = 0; i < NUM_ID_CODES; i++, pidCodeTable++)
		pidCodeTable->count = 0;

	/* Get the first Identifier Code in the message */
	/* Always the 6th byte */
	offset = 5;
	code = serialRxBuffer[ offset++ ];

	/* Scan all of the message(s) in the packet */
	do {
		/* Starting with the Identifier Code */
		/* Scan the table of valid ID Codes looking for a match */
		pidCodeTable = idCodeTable;
		for (i = 0; i < NUM_ID_CODES; i++, pidCodeTable++)
		{
			if ( code == pidCodeTable->code)
			{
				/* We have a match */
				pidCodeTable->count++;

				/* Break out of for loop */
				break;
			}
		}

		if ( i < NUM_ID_CODES )
		{
			/* Found a valid Identifier code */
			/* Use associated length value to find the next Identifier Code */
			offset += serialRxBuffer[ offset ] - 1;

			/* Care required */
			/* If the packet is corrupted the packet offset could be way out of bounds */
			if ( offset > maxOffset )
				return false;

			/* Read the next Identifier Code from the packet */
			code = serialRxBuffer[ offset++ ];
		}
		else
		{
			/* We didn't find a match. Something is wrong with the format */
			return false;
		}
	} 	while ( code != 0xf0 );

	/* At this point we've scanned, or tried to scan, */
	/* the whole packet for message(s) */
	if ( ( code == 0xf0 ) )
	{
		/* This is the correct final Identifier Code */
		/* No more messages */
		/* Loop exited normally, packet format is good */
		return true;

	}
	else
	{
		/* Loop has exited abnormally for whatever reason */
		/* Packet format is invalid */
		return false;
	}
}

/****************************************************************************
 * validateAdvancedBMSpacket
 * Protocol-specific function to validate any
 * incoming serial event data
 *
 * At this point we have a potential good packet
 * ready for processing, subject to the following
 * validation steps:
 * - Length, make sure its not too short
 * - CRC, in accordance with the specification
 * - Packet ID should always be 0x80
 * - Format, this should comprise a list
 *   of one or more messages, with each message
 *   having a valid type code and length
 *
 * We have a specification and the packet format
 * is well defined, so we could potentially do a lot
 * more to validate the format of each message
 * but the law of diminishing returns applies
 *
 * Returns a true/false result. If false an associated
 * error string holds details of the error
 *
 ****************************************************************************/
bool validateAdvancedBmsPacket ( void )
{
	uint8_t i;
    idCodeTable_t *pidCodeTable = idCodeTable;

	Serial.printf ( "\r\nPacket received, %u bytes\r\n", serialRxLength );
	
	if ( serialRxLength < ADVANCED_MIN_LENGTH )
	{
		/* Packet too short */
		sprintf (errorStr, "Error - Packet too short\r\n" );
		return false;
	}
	else if ( !validateCRC ( serialRxBuffer, serialRxLength ) )
	{
		/* Invalid CRC */
		sprintf (errorStr, "Error - Invalid CRC\r\n" );
		return false;
	}
	else if ( serialRxBuffer [ 1 ] != 0x80 )
	{
		/* Unexpected Packet Identity, should always be 0x80 */
		sprintf (errorStr, "Error - Invalid Packet Id. Expected 0x80, found 0x%x\r\n", serialRxBuffer [ 1 ] );
		return false;
	}
	else if ( !checkPacketFormat () )
	{
		/* Invalid Format, either: */
		/* - Invalid Identifier Code */
		/* - Combined message(s) length exceeds maximum */
		sprintf (errorStr, "Error - Invalid Packet Format\r\n" );
		return false;
	}
	else
	{
		/* Packet looks OK */
		Serial.write ( "OK\r\n" );

		/* Send summary of message types to the console */
		pidCodeTable = idCodeTable;
		for (i = 0; i < NUM_ID_CODES; i++, pidCodeTable++)
		{
			if ( pidCodeTable->count > 0 )
			{
				Serial.printf ( "%u x Message type %u (\"%s\")\r\n", pidCodeTable->count,
																			pidCodeTable->code,
																			pidCodeTable->label );
			}
		}
		return true;
	}
}
