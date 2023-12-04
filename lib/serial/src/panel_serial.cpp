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
 * Module :     panel_serial
 * Description: Supporting funtions for serial fire panels, including:
 *              - table of supported protocols and associated serialhandlers
 *              - table of default baud rates and character framings
 *              - routine to start/stop a sepcified serial protocol
 *
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "Particle.h"
#include "build.h"
#include "uart.h"
#include "panel_serial.h"
#include "hexDump.h"
#include <cstdint>
#include "Base64RK.h"

/****************************************************************************
 * Globals
 ****************************************************************************/

/****************************************************************************
 * Lookup table to get ASCII label from uart framing Id
 ****************************************************************************/
extern const serialFramingTable_t serialFramingTable[];
extern eepromData_t eepromData;

/****************************************************************************
 * Pointer to a function used to process incoming serial data
 * If applicable, the pointer is assigned to the appropriate protocol 
 * handler during protocol initialisation
 * Can be null if no serial protocol is configured
 ****************************************************************************/
serialRxHandler_t serialRxHandler;

/* Resources for building and tracking incoming serial events */
uint8_t rxChar;
uint8_t serialRxState;
uint8_t serialRxBuffer [ 512 ];
uint16_t serialRxLength;
uint32_t totalDiscardedBytes = 0;
uint32_t totalRxEvents = 0;

/* Resources for building outgoing responses and commands */
uint8_t	serialTxBuffer [ 20 ];

/* Unique transfer ID fudge */
uint8_t	uniqueTransferId = 1;

/****************************************************************************
 * Pointers for all of the serial start and stop handlers are held in a 
 * table along with the corresponding default UART settings
 * When a protocol is started the default UART settings are applied
 * but these can be changed later as required
 *
 * Note:
 * The at+pdsp command (list available serial protocols) has a lot to do,
 * approximately 700 chars to write to console
 *
 ****************************************************************************/
extern const serialHandlerTable_t serialHandlerTable[] =
{
	{ SERIAL_NONE_CONFIGURED,	"None configured",			nullptr,           			nullptr,			        19200UL,	SERIAL_FRAMING_8N1 },
	{ SERIAL_GENT,				"Gent Vigilon Universal",	serialStartGent,			serialStopGent,				9600UL,		SERIAL_FRAMING_8E1 },
	{ SERIAL_KENTEC,			"Kentec Syncro AS",			serialStartKentec,			serialStopKentec,			19200UL,    SERIAL_FRAMING_8N1 },
	{ SERIAL_SIEMENS_ASCII,		"Siemens Cerberus CS1140",	serialStartSiemensASCII,	serialStopSiemensASCII,		9600UL,		SERIAL_FRAMING_8N1 },
	{ SERIAL_MINERVA_ASCII,		"Tyco Minerva 8/16E/80",	serialStartMinervaASCII,	serialStopMinervaASCII,		4800UL,     SERIAL_FRAMING_8N1 },
	{ SERIAL_ADVANCED,			"Advanced MXPro BMS I/F",	serialStartAdvanced,		serialStopAdvanced,			38400UL,	SERIAL_FRAMING_8N1 },
	{ SERIAL_NOTIFIER,			"Notifier ID3000",			serialStartNotifier,		serialStopNotifier,			9600UL,		SERIAL_FRAMING_8N1 },
	{ SERIAL_GENT_ASCII,		"Gent Vigilon ASCII",		serialStartGentASCII,		serialStopGentASCII,		9600UL,		SERIAL_FRAMING_8E1 },
	{ SERIAL_ZITON,				"Ziton ZP3",				serialStartZiton,			serialStopZiton,			19200UL,	SERIAL_FRAMING_8N1 },
	{ SERIAL_RESERVED,			"Reserved",					nullptr,           			nullptr,					19200UL,	SERIAL_FRAMING_8N1 },
	{ SERIAL_ADVANCED_ASCII,	"Advanced MXPro ASCII",		serialStartAdvancedASCII,	serialStopAdvancedASCII,	9600UL,		SERIAL_FRAMING_8N1 }
};

/****************************************************************************
 * setSerialProtocol
 *  When starting a protocol it is important to stop any protocol that is 
 *  already running by calling the appropriate 'stop' handler. This will 
 *  stop and release any resources used by that handler
 ****************************************************************************/
void setSerialProtocol ( uint8_t Id )
{
	static uint8_t currentId = 0;

Logger log("app.serial");
	if ( serialHandlerTable[currentId].serialStopHandler != nullptr )
	{
		/* Looks like a protocol is already running */
		/* Stop it, even if it's the same one */
		/* There might be occasions when we want to stop and restart a protocol */
		serialHandlerTable[currentId].serialStopHandler();

		/* Report to Console */
		Log.info("Stopping serial protocol Id: %u, \"%s\"", 
										currentId, 
										serialHandlerTable[currentId].label );

		/* And stop */
		/* This disables the UART and releases the Tx and Rx pins for use as GPIO */
		Serial1.end();
	}

	/* Start the corresponding protocol */
	if ( serialHandlerTable[Id].serialStartHandler != nullptr )
	{
		/* Report to Console */
		log.info("Starting serial protocol Id: %u, \"%s\"", 
											Id, 
											serialHandlerTable[Id].label );
		log.info( "Baud rate: %lu, Framing %s", 
						serialHandlerTable [ Id ].baudRate, 
						serialFramingTable [ serialHandlerTable [ Id ].framingId ].label );

		/* Start the UART, using default settings for this protocol */
		Serial1.begin (serialHandlerTable [ Id ].baudRate, SERIAL_8N1);

		/* Update the eeprom config These settings might be different from those held in the eeprom config */
	
		/* And start the protocol */
		serialHandlerTable[Id].serialStartHandler();
	}
	else if ( Id == 0 )
	{
		/* No serial protocol configured */

		/* Report to Console */
		Log.info( "No serial protocol configured");

		/* Disable the UART and releases the Tx and Rx pins for use as GPIO */
		Serial1.end();
	}
	else
	{
		/* Report to Console */
		Log.info( "Error starting serial protocol Id: %u, \"%s\"", 
											Id, 
											serialHandlerTable[Id].label );
		Log.info( "No serial protocol configured");

		/* Disable the UART and releases the Tx and Rx pins for use as GPIO */
		Serial1.end();
	}

	/* Update the current ID */
	currentId = Id;
}

/* Process any incoming characters from the configured serial protocol */
void serialScan ( void )
{
  if ( Serial1.available() != 0 )
  {
      rxChar = Serial1.read ( );
//	  Serial.write(rxChar);
      if ( serialRxHandler != nullptr )
      {
          serialRxHandler ( rxChar );
      }
  }
}



/*****************************************************************************
 * Prototypes
 *****************************************************************************/



void serialStartKentec ( void )
{
}

void serialStopKentec ( void )
{
}

void serialStartSiemensASCII ( void )
{
}

void serialStopSiemensASCII ( void )
{
}

void serialStartMinervaASCII ( void )
{
}

void serialStopMinervaASCII ( void )
{
}

void serialStartNotifier ( void )
{
}

void serialStopNotifier ( void )
{
}

void serialStartGentASCII ( void )
{
}

void serialStopGentASCII ( void )
{
}

void serialStartZiton ( void )
{
}

void serialStopZiton ( void )
{
}

/* Structure populated by incoming events */
eventMessage_t eventMessage;

/* Structure used to transfer event(s) to Nimbus */
nimbusTransferMessage_t nimbusTransferMessage;


/****************************************************************************
 * serialTxEventToNimbus
 *  
 *  
 *  
 ****************************************************************************/
char encodedBuf[1024];
size_t encodedBufLen = sizeof(encodedBuf);
uint8_t serialLen, transferLen;

void serialTxEventToNimbus ( void )
{
	totalRxEvents += 1;
	Log.info( "Forwarding %u bytes to Nimbus (Event %lu):", serialRxLength, totalRxEvents );
//	hexDump ( serialRxBuffer, (uint16_t)serialRxLength);

    /* With each new event we have to add timestamp and length values */
    /* at the time of receipt. This preserves timing information and allows */
    /* us to buffer multiple events while we wait for a connection to Nimbus */

    /* This is done with a simple structure that includes type, length and */
    /* timestamp values along with the event itself */

    /* Populate the structure starting with the protocol type ... */
    /* All of the test data is Advanced HLI  */
    /* Type = 0x05 */
    eventMessage.Type = eepromData.serialProtocolId;

    /* Length */
    /* Includes +8 bytes for timestamp */
    /* All of our Advanced test messages are ~50 bytes */
    /* So only need the first byte of the length field */
    /* Set the other bytes to 0 */
    serialLen =  serialRxLength + 8;
    eventMessage.Length [ 0 ] = serialLen;
    eventMessage.Length [ 1 ] = 0;
    eventMessage.Length [ 2 ] = 0;

    /* Next 8 bytes are the timestamp showing when we received the message */
    /* I'm going to ignore fractional part, set to 0 */
    eventMessage.TimeStamp = Time.now();
    eventMessage.SecondFracPart = 0;

    /* Finally copy the serial event itself into the structure */
    memcpy (eventMessage.Data, serialRxBuffer, serialRxLength);
    
	/* Now convert the event into the nimbusTransferMessage format */
    /* Transfer length has to include 'Type' and 'Length' fields (1 and 3 bytes respectively) */
    serialLen += 4;

    /* Fill in the nimbusTransferMessage structure */
    nimbusTransferMessage.Type = 0x83;
    transferLen = serialLen + 12;
    nimbusTransferMessage.Length [ 0 ] = ( uint8_t ) ( transferLen );
    nimbusTransferMessage.Length [ 1 ] = 0;
    nimbusTransferMessage.Length [ 2 ] = 0;

    nimbusTransferMessage.uniqueTransferId = uniqueTransferId++;
    nimbusTransferMessage.TimeStamp = Time.now();
    nimbusTransferMessage.SecondFracPart = 0;

      /* Finally the content of the serial event itself */
    memcpy (nimbusTransferMessage.Data, eventMessage.Event, serialLen);
    
    /* Publish length has to include 'Type' and 'Length' fields (1 and 3 bytes respectively) */
    transferLen += 4;

	// Apply Base64 encoding and send to cloud
    /* Apply Base 64 encoding */
    encodedBufLen = sizeof(encodedBuf);
    bool success = Base64::encode(nimbusTransferMessage.Message, transferLen, encodedBuf, encodedBufLen, true);
    if (success) {
		  Particle.publish("nimbus/dev/event", encodedBuf, PRIVATE);

      /* Optional hex dump to verify the encoded data */
//			Serial.printlnf("Nimbus Event: %s", encodedBuf);
   }
}
