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
#include "build.h"
#include <cstdint>
#include <stdint.h>
#include <stdbool.h>

/* UART baud rate ID enumeration 
 * Different modules have different processors with support for different baud rates 
 * - B524 has nRF52840
 * - M524 has Realtek RTL8721DM
 * See the Particle Reference dosumentation or relevant processor data sheets for details */
#if defined M524
typedef enum
{
	SERIAL_BAUD_300,		// 0
	SERIAL_BAUD_600,		// 1
	SERIAL_BAUD_1200,		// 2
	SERIAL_BAUD_2400,		// 3
	SERIAL_BAUD_4800,		// 4
	SERIAL_BAUD_9600,		// 5
	SERIAL_BAUD_19200,		// 6
	SERIAL_BAUD_38400,		// 7
	SERIAL_BAUD_57600,		// 8
	SERIAL_BAUD_115200,		// 9
	SERIAL_BAUD_230400,		// 10
	SERIAL_BAUD_ID_COUNT	// 11
} serialBaudId_e;
#else
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
#endif

/* UART framing ID enumeration
 * Different modules have different processors with support for different framing options 
 * - B524 has nRF52840
 * - M524 has Realtek RTL8721DM
 * See the Particle Reference dosumentation or relevant processor data sheets for details */
#if defined M524
typedef enum
{
	SERIAL_FRAMING_8N1,		// 0
	SERIAL_FRAMING_8N2,		// 1
	SERIAL_FRAMING_8E1,		// 2
	SERIAL_FRAMING_8E2,		// 3
	SERIAL_FRAMING_8O1,		// 4
	SERIAL_FRAMING_8O2,		// 5
	SERIAL_FRAMING_7E1,		// 6
	SERIAL_FRAMING_7E2,		// 7
	SERIAL_FRAMING_7O1,		// 8
	SERIAL_FRAMING_7O2,		// 9
	SERIAL_FRAMING_ID_COUNT	// 10
} serialFramingId_e;
#else
typedef enum
{
	SERIAL_FRAMING_8N1,		// 0
	SERIAL_FRAMING_8E1,		// 1
	SERIAL_FRAMING_ID_COUNT	// 2
} serialFramingId_e;
#endif

/*****************************************************************************
 * Details for the serial uart baud rate settings are held in a table, 
 * along with the corresponding register value and ASCII label
 *****************************************************************************/
typedef struct
{
	serialBaudId_e id;
	const char *label;
} serialBaudTable_t;

/*****************************************************************************
 * Details for the serial uart character framing settings are held in a table, 
 * along with the corresponding ASCII label and register value for the Particle
 * platform
 *****************************************************************************/
typedef struct
{
	serialFramingId_e id;
	const char *label;
	uint32_t	regValue;
} serialFramingTable_t;

/****************************************************************************
 * Globals
 ****************************************************************************/
/* Table of permissible serial framing types */
extern const serialFramingTable_t serialFramingTable[];

/* Table of permissible serial baud rates */
extern const serialBaudTable_t serialBaudTable[];

/****************************************************************************
 * Prototypes
 ****************************************************************************/

/* Uart start/stop */
void startUart2 ( uint32_t baudRate );

#endif /* _UART_H_ */
