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

#ifndef PANEL_PROTOCOL_H
#define PANEL_PROTOCOL_H

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Protocol ID Enumeration
 * Protocol selection is done using these ID numbers
 * rather than the ASCII name
 * 
 * ID 0 (default) is used to disable any associated hardware, since
 * in general not all installations require a protocol
 * 
 * Note too that in general a protocol can be one of serial (UART), USB or 
 * Ethernet, depending on the panel type
 *****************************************************************************/
typedef enum
{
	PROTOCOL_NONE_CONFIGURED,		// 0
	PROTOCOL_GENT,					// 1
	PROTOCOL_KENTEC,				// 2
	PROTOCOL_SIEMENS_ASCII,			// 3
	PROTOCOL_MINERVA_ASCII,			// 4
	PROTOCOL_ADVANCED,				// 5
	PROTOCOL_NOTIFIER,				// 6
	PROTOCOL_GENT_ASCII,			// 7
	PROTOCOL_ZITON,					// 8
	PROTOCOL_RESERVED,				// 9
	PROTOCOL_ADVANCED_ASCII,		// 10
	PROTOCOL_COUNT					// 11
} protocolId_e;

/*****************************************************************************
 * Interface Type Enumeration
 *
 * A protocol can be one of serial (UART), USB or Ethernet, 
 * depending on the panel type
 * 
 * None is also valid, since some use cases might not require a protocol.
 * In these cases any associated hardware is shut down to reduce power
 *****************************************************************************/
typedef enum
{
	INTERFACE_NONE,		// 0
	INTERFACE_SERIAL,	// 1
	INTERFACE_USB,		// 2
	INTERFACE_ETHERNET,	// 3
	INTERFACE_COUNT		// 4
} interfaceType_e;

/* Pointers to protocol-specific serial handler functions */
typedef void (*protocolRxHandler_t) ( uint8_t );
typedef void (*protocolStartHandler_t) ( void );
typedef void (*protocolStopHandler_t) ( void );

/*****************************************************************************
 * Details for the serial start/stop handlers are held in a table,
 * along with the corresponding default UART settings
 * It is envisaged that whenever a serial protocol is started the
 * default UART settings are applied in the first instance
 * These can be changed later as required
 *****************************************************************************/
typedef struct
{
	protocolId_e id;
	const char *label;
	interfaceType_e type; // One of None, Serial (Uart), USB or Ethernet
	const protocolStartHandler_t protocolStartHandler;
	const protocolStopHandler_t  protocolStopHandler;
} protocolHandlerTable_t;

/*****************************************************************************
 * Interface type descriptor strings are held in a table
 * Serial, USB, Ethernet etc
 *****************************************************************************/
typedef struct
{
	interfaceType_e type;
	const char *label;
} interfaceTypeTable_t;

/*****************************************************************************
 * Global
 *****************************************************************************/
/* Handlers to start/stop protocols are found in this table */
extern const protocolHandlerTable_t protocolHandlerTable[];

/* String descriptors for the available interfaces */
extern const interfaceTypeTable_t interfaceTypeTable[];

/*****************************************************************************
 * Public Functions - Generic
 *****************************************************************************/
void serialScan ( void );
void setProtocolType ( uint8_t Id );
void serialTxEventToNimbus ( void );

/*****************************************************************************
 * Public Functions - Protocol specific
 * For clarity, note that by convention serial, USB and Ethernet handlers have
 * the corresponding prefix 'serial', 'usb' and 'ethernet'
 *****************************************************************************/
void serialStartAdvanced ( void );
void serialStartGent ( void );
void serialStartKentec ( void );
void serialStartSiemensASCII ( void );
void serialStartMinervaASCII ( void );
void serialStartNotifier ( void );
void serialStartGentASCII ( void );
void serialStartZiton ( void );
void serialStartAdvancedASCII ( void );

void serialStopAdvanced ( void );
void serialStopGent ( void );
void serialStopKentec ( void );
void serialStopSiemensASCII ( void );
void serialStopMinervaASCII ( void );
void serialStopNotifier ( void );
void serialStopGentASCII ( void );
void serialStopZiton ( void );
void serialStopAdvancedASCII ( void );

#endif /* PANEL_PROTOCOL_H_ */
