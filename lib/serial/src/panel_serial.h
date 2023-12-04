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

#ifndef PANEL_SERIAL_H
#define PANEL_SERIAL_H

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/*****************************************************************************
 * Protocol ID enumeration
 * Protocol selection is done using these ID numbers
 * rather than the ASCII name
 * ID 0 (default) is used to disable the UART, since
 * not all installations require a serial protocol
 *****************************************************************************/
typedef enum
{
	SERIAL_NONE_CONFIGURED,			// 0
	SERIAL_GENT,					// 1
	SERIAL_KENTEC,					// 2
	SERIAL_SIEMENS_ASCII,			// 3
	SERIAL_MINERVA_ASCII,			// 4
	SERIAL_ADVANCED,				// 5
	SERIAL_NOTIFIER,				// 6
	SERIAL_GENT_ASCII,				// 7
	SERIAL_ZITON,					// 8
	SERIAL_RESERVED,				// 9
	SERIAL_ADVANCED_ASCII,			// 10
	SERIAL_PROTOCOL_COUNT			// 11
} serialProtocolId_e;

/* Pointers to protocol-specific serial handler functions */
typedef void (*serialRxHandler_t) ( uint8_t );
typedef void (*serialStartHandler_t) ( void );
typedef void (*serialStopHandler_t) ( void );

/*****************************************************************************
 * Details for the serial start/stop handlers are held in a table,
 * along with the corresponding default UART settings
 * It is envisaged that whenever a serial protocol is started the
 * default UART settings are applied in the first instance
 * These can be changed later as required
 *****************************************************************************/
typedef struct serialHandlerTableEntry
{
	serialProtocolId_e id;
	const char *label;
	const serialStartHandler_t serialStartHandler;
	const serialStopHandler_t  serialStopHandler;
	uint32_t baudRate;
	uint8_t framingId;
} serialHandlerTable_t;

/*****************************************************************************
 * Details for Gateway configuration are all held in one structure in EEPROM
 * - this is copied from EEPROM at startup
 * - written back to EEPROM whenever it is changed, eg by at commands
 * Currently 16 bytes
 *****************************************************************************/
typedef struct
{
    uint32_t	magic;
	uint32_t	panelSerialBaud;
	bool		verbose;
	uint8_t		gpioOutputVal;
	uint8_t		serialFramingId;
	uint8_t		serialProtocolId;
} eepromData_t;

/*****************************************************************************
 * Public functions
 *****************************************************************************/
void serialScan ( void );
void setSerialProtocol ( uint8_t Id );
void serialTxEventToNimbus ( void );

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


#endif /* PANEL_SERIAL_H_ */
