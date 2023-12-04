/*****************************************************************************
 *
 * Copyright (C) 2017 Fire Fighting Enterprises Ltd
 *
 * This program is an unpublished trade secret of Fire Fighting Enterprises UK
 *
 * All rights thereto are reserved. Use or copying of all or any portion
 * of this program is prohibited except with express written authorisation
 * from Fire Fighting Enterprises UK
 *
 ******************************************************************************
 *
 * Module :     build
 * Description: Controls overall build
 *              Provides some control of application size
 *
 *****************************************************************************/

#ifndef EEPROM_H
#define EEPROM_H

/****************************************************************************
 * Includes
 ****************************************************************************/
#include <cstdint>

/*****************************************************************************
 * Manufacturers information is held in one structure in a small dedicated 
 * EEPROM on the baseboard (128 bytes).
 * This includes the following information, set during manufacturer using AT 
 * commands:
 * - manufacturer
 * - board type (eg Fixed or Mobile)
 * - PCB revision
 * - Serial number
 * 
 * Currently 80 bytes
 * 
 * Due to the board-specific nature of this information, it can't be held in 
 * the Particle EEPROM, as the module might be swapped out into another 
 * baseboard.
 *****************************************************************************/
typedef struct
{
  /* Magic number to verify data integrity */
  uint32_t	magic;

	/* Product Identification */
	char		manufacturer [32];
	char		baseboardType [ 32 ];
	char		baseboardRevision  [ 8 ];
	char		baseboardSerialNo [ 8 ];
} baseboardEepromData_t;

/*****************************************************************************
 * Details for Gateway configuration are all held in one structure in the 
 * on-module Particle EEPROM
 * - this is validated and copied into RAM at startup
 * - written back to EEPROM whenever it is changed, eg by AT commands
 * Currently ~60 bytes
 *****************************************************************************/
typedef struct
{
	/* Magic number for EEPROM integrity validation */
    uint32_t	magic;
	
	/* Panel */
	uint32_t	panelSerialBaud;
	uint8_t		panelSerialFraming;
	uint8_t		serialFramingId;
	uint8_t		protocolId;
	uint8_t		morleyZxPanelAddr;

	/* Nimbus */
	uint32_t	nimbusSessionId;
	char		nimbusTargetServer [ 32 ];
	bool		enableNimbusTransfers;
	
	/* Cellular */
	bool		cellPower;
	
	/* GPIO */
	uint8_t		gpioOutputVal;
	
	/* AT Commands */
	uint8_t		timestampFormat;
	
	/* Security */
	char		atCommandPassword [ 8 ];

	/* Misc */
	bool		verbose;
} particleEepromData_t;

/*****************************************************************************
 * Global variables 
 *****************************************************************************/
extern baseboardEepromData_t baseboardEepromData;
extern particleEepromData_t particleEepromData;

#endif /* EEPROM_H */
