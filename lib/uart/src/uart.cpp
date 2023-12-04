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
#include "build.h"
#include <cstdint>
#include "Particle.h"

/****************************************************************************
 * Globals
  ***************************************************************************/

/****************************************************************************
 * Lookup table of supported uart baud rates for the Fire Panel
 ****************************************************************************/
/* Different modules support different baud rates */
#if defined M524
extern const serialBaudTable_t serialBaudTable[] =
{  /* Baud Rate Id          Label    */
	{ SERIAL_BAUD_300,		"300"   },
	{ SERIAL_BAUD_600,		"600"   },
	{ SERIAL_BAUD_1200,		"1200"   },
	{ SERIAL_BAUD_2400,		"2400"   },
	{ SERIAL_BAUD_4800,		"4800"   },
	{ SERIAL_BAUD_9600,		"9600"   },
	{ SERIAL_BAUD_19200,	"19200"  },
	{ SERIAL_BAUD_38400,	"38400"  },
	{ SERIAL_BAUD_57600,	"57600"  },
	{ SERIAL_BAUD_115200,	"115200" },
	{ SERIAL_BAUD_230400,	"230400" }
};
#else
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
#endif

/****************************************************************************
 * Lookup table of supported uart framing options for the Fire Panel
 ****************************************************************************/
#if defined M524
extern const serialFramingTable_t serialFramingTable[] =
{  /* Framing Id            Label   Reg Value */
	{ SERIAL_FRAMING_8N1,	"8n1",	SERIAL_8N1 },
	{ SERIAL_FRAMING_8N2,	"8n2",	SERIAL_8N2 },
	{ SERIAL_FRAMING_8E1,	"8e1",	SERIAL_8E1 },
	{ SERIAL_FRAMING_8E2,	"8e2",	SERIAL_8E2 },
	{ SERIAL_FRAMING_8O1,	"8o1",	SERIAL_8O1 },
	{ SERIAL_FRAMING_8O2,	"8o2",	SERIAL_8O2 },
	{ SERIAL_FRAMING_7E1,	"7e1",	SERIAL_7E1 },
	{ SERIAL_FRAMING_7E2,	"7e2",	SERIAL_7E2 },
	{ SERIAL_FRAMING_7O1,	"7o1",	SERIAL_7O1 },
	{ SERIAL_FRAMING_7O2,	"7o2",	SERIAL_7O2 }
};
#else
extern const serialFramingTable_t serialFramingTable[] =
{  /* Framing Id            Label   Reg Value */
	{ SERIAL_FRAMING_8N1,	"8n1",	SERIAL_8N1 },
	{ SERIAL_FRAMING_8E1,	"8e1",	SERIAL_8E1 }
};
#endif

/******************************************************************************
 * UART Functions
 ******************************************************************************/

void startUart2 ( uint32_t baudRate )
{
//    panel.baud(9600);
}

