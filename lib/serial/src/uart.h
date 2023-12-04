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
 * Module :     uart
 * Description: Low level PIC uart functions
 *
 *****************************************************************************/

#ifndef _UART_H
#define _UART_H

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <cstdint>
#include <stdint.h>
#include <stdbool.h>

/* UART baud rate ID enumeration */
typedef enum
{
	SERIAL_BAUD_1200,		// 0
	SERIAL_BAUD_2400,		// 1
	SERIAL_BAUD_4800,		// 2
	SERIAL_BAUD_9600,		// 3
	SERIAL_BAUD_19200,		// 4
	SERIAL_BAUD_38400,		// 5
	SERIAL_BAUD_57600,		// 6
	SERIAL_BAUD_115200,		// 7
	SERIAL_BAUD_ID_COUNT	// 8
} serialBaudId_e;

/* UART framing ID enumeration */
typedef enum
{
	SERIAL_FRAMING_8N1,		// 0
	SERIAL_FRAMING_8E1,		// 1
	SERIAL_FRAMING_ID_COUNT	// 4
} serialFramingId_e;

/*****************************************************************************
 * Details for the serial uart baud rate settings are held in a table, 
 * along with the corresponding register value and ASCII label
 *****************************************************************************/
typedef struct serialBaudTableEntry
{
	serialBaudId_e id;
	const char *label;
} serialBaudTable_t;

/*****************************************************************************
 * Details for the serial uart character framing settings are held in a table, 
 * along with the corresponding ASCII label and register value for the Particle
 * platform
 *****************************************************************************/
typedef struct serialFramingTableEntry
{
	serialFramingId_e id;
	const char *label;
	uint32_t	regValue;
} serialFramingTable_t;

/****************************************************************************
 * Prototypes
 ****************************************************************************/

/* Uart start/stop */
void startUart2 ( uint32_t baudRate );

#endif /* _UART_H_ */
