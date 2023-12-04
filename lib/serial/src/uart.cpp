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

/*****************************************************************************
 * Includes                                                                  
 *****************************************************************************/
#include "uart.h"
#include <cstdint>
#include "Particle.h"

/****************************************************************************
 * Globals
  ***************************************************************************/

/****************************************************************************
 * Lookup table to get ASCII label and 16-bit register configuration 
 * values from uart baud Id
 ****************************************************************************/
extern const serialBaudTable_t serialBaudTable[] =
{  /* Baud Rate Id          Label    */
	{ SERIAL_BAUD_1200,		"1200"   },
	{ SERIAL_BAUD_2400,		"2400"   },
	{ SERIAL_BAUD_4800,		"4800"   },
	{ SERIAL_BAUD_9600,		"9600"   },
	{ SERIAL_BAUD_19200,	"19200"  },
	{ SERIAL_BAUD_38400,	"38400"  },
	{ SERIAL_BAUD_57600,	"57600"  },
	{ SERIAL_BAUD_115200,	"115200" }
};

/****************************************************************************
 * Lookup table to get ASCII label from uart framing Id
 ****************************************************************************/
extern const serialFramingTable_t serialFramingTable[] =
{  /* Framing Id            Label   Reg Value */
	{ SERIAL_FRAMING_8N1,	"8n1",	SERIAL_8N1 },
	{ SERIAL_FRAMING_8E1,	"8e1",	SERIAL_8E1 }
};

/******************************************************************************
 * UART Functions
 ******************************************************************************/

void startUart2 ( uint32_t baudRate )
{
//    panel.baud(9600);
}

