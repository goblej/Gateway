/*****************************************************************************
 *
 * Copyright (C) 2023 Nimbus Digital Ltd
 *
 * All rights thereto are reserved. Use or copying of all or any portion
 * of this program is prohibited except with express written authorisation
 * from Nimbus Digital Ltd
 *
 ******************************************************************************
 *
 * Module :     panel_protocol
 * Description: Supporting funtions for fire panel protocols, including:
 *              - table of supported protocols and associated handlers
 *              - routines to cleanly start/stop a specified protocol
 * 
 * With support for serial (Uart), USB and Ethernet physical interfaces
 *
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "Particle.h"
#include "build.h"
#include "eeprom.h"
#include "uart.h"
#include "panel_protocol.h"
#include "hexDump.h"
#include <cstdint>
#include "Base64RK.h"

/****************************************************************************
 * Globals
 ****************************************************************************/

/****************************************************************************
 * Lookup table to get ASCII descriptor from uart framing Id
 ****************************************************************************/
extern const serialFramingTable_t serialFramingTable[];

/****************************************************************************
 * Configuration parameters are held in non-volatile EEPROM
 ****************************************************************************/
extern particleEepromData_t particleEepromData;

/****************************************************************************
 * Pointer to a function used to process incoming panel data
 * If applicable, the pointer is assigned to the appropriate protocol 
 * handler during protocol initialisation
 * Can be null if no protocol is configured
 ****************************************************************************/
protocolRxHandler_t protocolRxHandler;

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
 * Pointers for all of the protocol start and stop handlers are held in a 
 * table. The table includes a long ASCII descriptor for each protocol.
 * 
 * The start handlers perform any necessary initialisation, including 
 * powering up and starting any associated physical interface. There are 
 * currently four supported interface types:
 * - None (turn everything off to conserve power)
 * - Serial
 * - USB
 * - Ethernet
 * 
 * The stop handlers release any associated resources and close down the 
 * associated physical interface
 ****************************************************************************/
extern const protocolHandlerTable_t protocolHandlerTable[] =
{
	{ PROTOCOL_NONE_CONFIGURED,	"None configured",					INTERFACE_NONE,   	nullptr,         			nullptr 				},
	{ PROTOCOL_GENT,			"Gent Vigilon Universal",			INTERFACE_SERIAL, 	serialStartGent,			serialStopGent			},
	{ PROTOCOL_KENTEC,			"Kentec Syncro AS",					INTERFACE_SERIAL, 	serialStartKentec,			serialStopKentec		},
	{ PROTOCOL_SIEMENS_ASCII,	"Siemens Cerberus CS1140 ASCII",	INTERFACE_SERIAL, 	serialStartSiemensASCII,	serialStopSiemensASCII	},
	{ PROTOCOL_MINERVA_ASCII,	"Tyco Minerva ASCII",				INTERFACE_SERIAL, 	serialStartMinervaASCII,	serialStopMinervaASCII	},
	{ PROTOCOL_ADVANCED,		"Advanced MXPro BMS I/F",			INTERFACE_SERIAL, 	serialStartAdvanced,		serialStopAdvanced		},
	{ PROTOCOL_NOTIFIER,		"Notifier ID3000",					INTERFACE_SERIAL, 	serialStartNotifier,		serialStopNotifier		},
	{ PROTOCOL_GENT_ASCII,		"Gent Vigilon ASCII",				INTERFACE_SERIAL, 	serialStartGentASCII,		serialStopGentASCII		},
	{ PROTOCOL_ZITON,			"Ziton ZP3",						INTERFACE_SERIAL,	serialStartZiton,			serialStopZiton			},
	{ PROTOCOL_RESERVED,		"Reserved",							INTERFACE_NONE,   	nullptr, 	          		nullptr					},
	{ PROTOCOL_ADVANCED_ASCII,	"Advanced MXPro ASCII",				INTERFACE_SERIAL, 	serialStartAdvancedASCII,	serialStopAdvancedASCII }
};

/****************************************************************************
 * Lookup table to get corresponding ASCII descriptor string from interface type
 * Used to support console messages
 ****************************************************************************/
extern const interfaceTypeTable_t interfaceTypeTable[] =
{
	/* Interface Type        Label    */
	{ INTERFACE_NONE,		"None"		},
	{ INTERFACE_SERIAL,		"serial"   	},
	{ INTERFACE_USB,		"USB"   	},
	{ INTERFACE_ETHERNET,	"Ethernet"  }
};

/****************************************************************************
 * setProtocolType
 * When starting a protocol it is important to first stop any protocol that 
 * is already running by calling the appropriate 'stop' handler. This will 
 * stop and release any resources used by that handler, including any
 * associated physical interface
 ****************************************************************************/
void setProtocolType ( uint8_t Id )
{
	static uint8_t currentId = 0;

	if ( protocolHandlerTable[currentId].protocolStopHandler != nullptr )
	{
		/* Looks like a protocol is already running */
		/* Stop it, even if it's the same one */
		/* There might be occasions when we want to stop and restart a protocol */
		protocolHandlerTable[currentId].protocolStopHandler();

		/* Report to Console */
		Log.info("Stopping protocol Id: %u, \"%s\" on %s interface", 
										currentId, 
										protocolHandlerTable[currentId].label,
										interfaceTypeTable [protocolHandlerTable[currentId].type].label );
	}

	/* Start the new protocol */
	if ( protocolHandlerTable[Id].protocolStartHandler != nullptr )
	{
		/* Report to Console */
		Log.info("Starting protocol Id: %u, \"%s\" on %s interface", 
											Id, 
											protocolHandlerTable[Id].label,
											interfaceTypeTable [protocolHandlerTable[Id].type].label );

		/* And start the protocol */
		protocolHandlerTable[Id].protocolStartHandler();
	}
	else if ( Id == 0 )
	{
		/* No serial protocol configured */

		/* Report to Console */
		Log.info( "No protocol configured");
	}
	else
	{
		/* Report to Console */
		Log.info( "Error starting protocol Id: %u, \"%s\" on \"%s\" interface", 
											Id, 
											protocolHandlerTable[Id].label,
											interfaceTypeTable [protocolHandlerTable[currentId].type].label );
		Log.info( "No protocol configured");
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
      if ( protocolRxHandler != nullptr )
      {
          protocolRxHandler ( rxChar );
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
    eventMessage.Type = particleEepromData.protocolId;

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
