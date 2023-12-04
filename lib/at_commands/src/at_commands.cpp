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
 * Module :     at_commands
 * Description: Provides an at command suite for configuring the Gateway
 *              via a serial connection
 *
 *              Based on standard at-command functionality as found on 
 *              many commercial modem or modem-type products
 *              So each command can be applied in one of four ways:
 *              - Test, reports the expected command syntax
 *              - Read, reports the current value of a configuration parameter
 *              - Write, writes a new value to the configuration parameter
 *              - Action, performs a command action using existing parameters
 *
 *              Used in conjunction with the Command Line Interpreter or CLI 
 *              functions
 *
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "cli.h"
#include "build.h"
#include "eeprom.h"
#include "uart.h"
#include "at_commands.h"
#include "panel_protocol.h"
#include "24LC01-RK.h"
#include <cstdlib>

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/* AT command IDs */
typedef enum
{
	/* General AT Commands */
	AT_CMD_HELP,

	/* Device Identification */
	AT_CMD_PATI,
	AT_CMD_PMFG,
	AT_CMD_PBBT,
	AT_CMD_PBBR,
	AT_CMD_PBBS,

	/* Panel Protocol */
	AT_CMD_PSPT,
	AT_CMD_PIPR,
	AT_CMD_PICF,
	AT_CMD_PSTS,
	AT_CMD_PMADR,

	/* GPIO */
	AT_CMD_PIOW,
	AT_CMD_PIOR,

	/* Nimbus */
	AT_CMD_PTGT,
	AT_CMD_PSID,
	AT_CMD_PXFR,

	/* Security */
	AT_CMD_PSAL,
	AT_CMD_PPWD,

	/* Cellular */
	AT_CMD_CPWR,

	/* Device */
	AT_CMD_PRST,
	AT_CMD_PRFD,

	/* Count */
	AT_CMD_ID_COUNT
} atCmdId_e;

/* Command Authority Levels */
/* Command access is controlled using a number of pre-defined authority levels */
/* Just two, keeping it simple */
typedef enum
{
	/* Authority levels for the commands */
	AUTH_LEVEL_NONE,
	AUTH_LEVEL_FACTORY,

	/* Count */
	AUT_LEVEL_COUNT
} authLevel_e;

/* Structure for AT Commmand table entries */
/* Using a table keeps everything tidy and scaleable */
typedef struct atCmdTableEntry
{
    atCmdId_e id;
    const char *command;
    const atCmdHandler_t cmdHandler;
	authLevel_e	authLevel;
    const char *description;
} atCmdTable_t;

/* AT Command types */
/* In general all of the AT commands can be used in four ways. Specifically:     */
/*                                                                               */
/* Type   Syntax           Description                                           */
/*                                                                               */
/* TEST   at+<cmd>=?       This command returns the list of parameters and value */
/*                         ranges set by the corresponding Write Command         */
/*                                                                               */
/* READ   at+<cmd>?        This command returns the currently set value of the   */
/*                         parameter or parameters                               */
/*                                                                               */
/* WRITE  at+<cmd>=<p1>[   This command sets the user-definable parameter values */
/*  ,<p2>[,<p3>[...]]]                                                           */
/*                                                                               */
/* ACTION at+<cmd>         Initiate a default action without parameters          */
/*                                                                               */
typedef enum
{
	AT_TYPE_TEST,		// 0
	AT_TYPE_READ,		// 1
	AT_TYPE_WRITE,		// 2
	AT_TYPE_ACTION,		// 3
	AT_TYPE_ERROR		// 4
} atCommandType_e;

/*****************************************************************************
 * Globals
 *****************************************************************************/

/* Product Identification 
 * Used in conjunction with supplementary configurable identification
 * information held in EEPROM */
extern char	hostModuleType [ ];
extern char	gatewayFirmwareVer [ ];
extern char	protocolLibraryVer [ ];
extern uint8_t		atCommandAccessLevel;

void resetParticleEeprom ( void );
void resetBaseboardEeprom ( void );

/*****************************************************************************
 * Constants - Generic CLI
 *****************************************************************************/
#define at_MAX_CTRL_CMD_SIZE       10
static const char stringPrompt[]  = ">>> ";

/*****************************************************************************
 * Constants - Command specific
 *****************************************************************************/
#define MAX_PASSWORD_LENGTH	7

/*****************************************************************************
 * Private Data
 *****************************************************************************/
char *mCommand;
uint16_t mCmdIndex;

/* Stores next token from command line */
char mCmdToken[AT_MAX_TOKEN_SIZE + 1];

/* String resources for building command responses */
char tmpStr [1024];
char responseStr [ 1024 ];

/* Authority Level */
authLevel_e authorityLevel = AUTH_LEVEL_NONE;

/*****************************************************************************
 * Private Functions
 *****************************************************************************/
static bool at_ProcessCmd(void);

/*****************************************************************************
 * Prototypes for AT command handlers
 *****************************************************************************/

/* General AT Commands */
static bool at_help_CmdHandler ( void );

/* Device Identification Commands */
static bool at_pati_CmdHandler ( void );
static bool at_pmfg_CmdHandler ( void );
static bool at_pbbt_CmdHandler ( void );
static bool at_pbbr_CmdHandler ( void );
static bool at_pbbs_CmdHandler ( void );

/* Panel Commands */
static bool at_pspt_CmdHandler ( void );
static bool at_pipr_CmdHandler ( void );
static bool at_picf_CmdHandler ( void );
static bool at_psts_CmdHandler ( void );
static bool at_pmadr_CmdHandler ( void );

/* GPIO Commands */
static bool at_pior_CmdHandler ( void );
static bool at_piow_CmdHandler ( void );

/* Nimbus Commands */
static bool at_ptgt_CmdHandler ( void );
static bool at_psid_CmdHandler ( void );
static bool at_pxfr_CmdHandler ( void );

/* Security Commands */
static bool at_psal_CmdHandler ( void );
static bool at_ppwd_CmdHandler ( void );

/* Cellular */
static bool at_cpwr_CmdHandler ( void );

/* Device Commands */
static bool at_prst_CmdHandler ( void );
static bool at_prfd_CmdHandler ( void );

/*****************************************************************************
 * AT Command Table
 * Keeps everything tidy and scaleable
 *****************************************************************************/
static const atCmdTable_t atCmdTable[] =
{
    /* Id       		Name    		Handler     		Description                     */

	/* AT Commands */
    { AT_CMD_HELP,		"at+help",		at_help_CmdHandler, AUTH_LEVEL_FACTORY, "List available commands"		},

	/* Device Identification */
    { AT_CMD_PATI, 		"at+pati",      at_pati_CmdHandler,	AUTH_LEVEL_FACTORY, "Show Manufacturers Information"},
    { AT_CMD_PMFG, 		"at+pmfg",      at_pmfg_CmdHandler, AUTH_LEVEL_FACTORY, "Set Manufacturer"				},
    { AT_CMD_PBBT, 		"at+pbbt",      at_pbbt_CmdHandler, AUTH_LEVEL_FACTORY, "Set Baseboard Type"			},
    { AT_CMD_PBBR, 		"at+pbbr",      at_pbbr_CmdHandler, AUTH_LEVEL_FACTORY, "Set Baseboard PCB Revision"	},
    { AT_CMD_PBBS, 		"at+pbbs",      at_pbbs_CmdHandler, AUTH_LEVEL_FACTORY, "Set Baseboard Serial Number"	},
   
	/* Panel Protocol */
    { AT_CMD_PSPT, 		"at+pspt",      at_pspt_CmdHandler, AUTH_LEVEL_FACTORY, "Set Protocol Type" 			},
    { AT_CMD_PIPR, 		"at+pipr",      at_pipr_CmdHandler, AUTH_LEVEL_FACTORY, "Set serial baud rate"  		},
    { AT_CMD_PICF, 		"at+picf",      at_picf_CmdHandler, AUTH_LEVEL_FACTORY, "Set serial framing"  			},
    { AT_CMD_PSTS, 		"at+psts",      at_psts_CmdHandler, AUTH_LEVEL_FACTORY, "Show panel status"  			},
    { AT_CMD_PMADR,		"at+pmadr",     at_pmadr_CmdHandler, AUTH_LEVEL_FACTORY, "Set Morley panel address"  	},

	/* GPIO */
    { AT_CMD_PIOR,	 	"at+pior",      at_pior_CmdHandler, AUTH_LEVEL_FACTORY, "Read digital inputs" 			},
    { AT_CMD_PIOW,	 	"at+piow",      at_piow_CmdHandler, AUTH_LEVEL_FACTORY, "Set digital outputs" 			},

	/* Nimbus */
    { AT_CMD_PTGT, 		"at+ptgt",      at_ptgt_CmdHandler, AUTH_LEVEL_FACTORY, "Set Nimbus Target"				},
    { AT_CMD_PSID, 		"at+psid",      at_psid_CmdHandler, AUTH_LEVEL_FACTORY, "Set Session ID"				},
    { AT_CMD_PXFR, 		"at+pxfr",      at_pxfr_CmdHandler, AUTH_LEVEL_FACTORY, "Nimbus Transfers"				},

	/* Security */
    { AT_CMD_PSAL, 		"at+psal",      at_psal_CmdHandler, AUTH_LEVEL_NONE, 	"Set Authority Level"			},
    { AT_CMD_PPWD, 		"at+ppwd",      at_ppwd_CmdHandler, AUTH_LEVEL_FACTORY, "Manage Password"				},

	/* Cellular */
    { AT_CMD_CPWR, 		"at+cpwr",      at_cpwr_CmdHandler, AUTH_LEVEL_FACTORY, "Cellular Power"			},

	/* Device */
    { AT_CMD_PRST, 		"at+prst",      at_prst_CmdHandler, AUTH_LEVEL_FACTORY, "Device Reset"					},
    { AT_CMD_PRFD, 		"at+prfd",      at_prfd_CmdHandler, AUTH_LEVEL_FACTORY, "Restore Factory Defaults"		}
};

/*****************************************************************************
 * at_Initialise
 *****************************************************************************/
void at_Initialise(void)
{
    cliInitialise();
    cliSetEcho();
    at_PrintPrompt(false);
}

/*****************************************************************************
 * at_GetToken
 * Get next token from command buffer into "cmdToken"
 *****************************************************************************/
void at_GetToken(void)
{
    char cmdChar;
	char inString = '\0';
    uint16_t i,j;

    /* Tokenise from command buffer */
    for (i = 0, j = 0; i < AT_MAX_TOKEN_SIZE; i++)
    {
        cmdChar = mCommand[mCmdIndex];

        if (cmdChar == (char)0)
        {
            /* Finished */
             break;
        }

		if ( inString )
		{
	        if ( cmdChar == inString )
			{
				/* Clear string flag */
				inString = '\0';

				/* Finished this token */
				break;
			}
			else
			{
				/* Collect all of the characters within the quotes */
                mCmdToken[j] = cmdChar;
                mCmdIndex++;
                j++;
			}
			continue;
		}

        /* ignore leading space and tabs */
        if ((cmdChar == ' ') || (cmdChar == ',') || (cmdChar == '\t') )
        {
            mCmdIndex++;
            if( j == 0 )
            {
                continue;
            }
            break;
        }

        /* test for separator */
        if ( (cmdChar == ',') || (cmdChar == ';') )
         {
            if( j == 0 )
            {
                mCmdToken[j] = cmdChar;
                mCmdIndex++;
                j++;
            }
            break;
         }

        /* test for separator */
        if ( (cmdChar == '=') || (cmdChar == '?') )
         {
            if( j == 0 )
            {
                mCmdToken[j] = cmdChar;
                mCmdIndex++;
                j++;
            }
            break;
         }

        /* test for start of string */
        if ( (cmdChar == '\'') || (cmdChar == '"') )
         {
			/* Set string flag */
			inString = cmdChar;

            mCmdIndex++;
            if( j == 0 )
            {
                continue;
            }
            break;
         }

        mCmdToken[j] = cmdChar;
        mCmdIndex++;
        j++;
    }

    mCmdToken[j] = (char)0;
}

/*****************************************************************************
 * at_GetFirstToken
 *****************************************************************************/
void at_GetFirstToken(char *cmd)
{
    mCommand = cmd;
    mCmdIndex = 0;
    at_GetToken();
}

/*****************************************************************************
 *  at_CommmandParser
 *****************************************************************************/
void at_CommmandParser(char *cmdline)
{
    /* Read first token */
    at_GetFirstToken(cmdline);

    if( mCmdToken[0] )
    {
        /* Process the command */
        at_ProcessCmd();
    }
    /* finished parse */
    at_PrintPrompt(false);
}

/*****************************************************************************
 * at_PrintPrompt
 *****************************************************************************/
void at_PrintPrompt(bool addCR)
{
    if (addCR)
    {
        Serial.write("\r");
    }

    Serial.write(stringPrompt);
}

extern void bluetoothWriteStr ( char * str );

/*****************************************************************************
 * at_ProcessCmd
 *****************************************************************************/
static bool at_ProcessCmd(void)
{
    bool result = false;
	bool res;
    uint16_t i;
    const atCmdTable_t *pAtCmdTable = atCmdTable;

	/* Clear the intermediate string resource used to build command response */
	tmpStr[0] = '\0';

	/* Scan the table looking for a match */
    for (i = 0; i < AT_CMD_ID_COUNT; i++, pAtCmdTable++)
    {
		/* I know strcasecmp is not in the C or C++ standard but it works here */
		/* I doubt this lapse will present any insurmountable portability issues */
        if (!strcasecmp(mCmdToken, pAtCmdTable->command))
        {
			/* Run the associated command handler */
            if (pAtCmdTable->cmdHandler != NULL_CMD_FN)
            {
				/* Found the command */
				result = true;

				/* All commands return a response string */
				/* Start building the response string */
				sprintf ( responseStr, "%s: \n", mCmdToken + 2 );

				/* Does the user have authority to run it ? */
				if ( authorityLevel >= pAtCmdTable->authLevel ) {
					/* Run the command */
					res = pAtCmdTable->cmdHandler();

					/* Append the overall OK or ERROR result */
					if ( res != false )
						strcat (responseStr, "OK\n");
					else
						strcat (responseStr, "ERROR\n");
				}
				else {
					/* The user is not authorised to run this command */
					strcat ( responseStr, "Authority required\nERROR\n");
				}
            }

			/* Command found, exit loop */
            break;
        }
    }

	/* Command not found */
    if (result == false)
    {
		sprintf( responseStr, "Unknown command: %s\n", mCmdToken);
    }

	/* Send response */
	Serial.write ( responseStr );
	/* Bluetooth responses limited to 236 bytes */
	responseStr[236] = '\0';
	bluetoothWriteStr ( responseStr );

	return result;
}

/*****************************************************************************
 * at_GetNumber
 * Accepts [0-9]
 * Returns the 64-bit numeric value and a validity flag
 *****************************************************************************/
uint64_t at_GetNumber ( bool* tokenValid )
{
	bool result = true;
	size_t i, len;
	char cmdChar;
	uint64_t val = 0;

	len = strlen (mCmdToken);
	for ( i=0; i < len; i++)
	{
		cmdChar = mCmdToken[i];
		if ((cmdChar < '0') || (cmdChar > '9')) {
			/* Token contains non numerics */
			result = false;
		}
		else {
            val = (val * 10) + (uint64_t)(cmdChar - '0');
		}
	}

	/* Was token a number ? */
	*tokenValid = result;

	/* Return final value */
	return val;
}

/*****************************************************************************
 * AT Command Handlers
 *****************************************************************************/

/*****************************************************************************
 * at_GetCommandType
 * We have received a valid command
 * Identify the command type, should be one of:
 * - TEST
 * - READ
 * - WRITE
 * - ACTION
 * - ERROR
 *
 * This format is common to all at commands
 *
 *****************************************************************************/
atCommandType_e at_GetCommandType ( void )
{
	/* The command type depends on any supplied token or tokens, if any */
	/* Get the first one */
	at_GetToken();

	/* And process it */
	if (mCmdToken[0])
	{
		if (mCmdToken[0]=='=')
		{
			/* '=' should be followed either by '?' or a parameter */
			/* Get the next token */
			at_GetToken();
			if (mCmdToken[0])
			{
				if (mCmdToken[0]=='?')
				{
					/* '=?' is the 'Test' command */
					return AT_TYPE_TEST;
				}
				else
				{
					/* At least one parameter has been supplied */
					/* Must be a 'Write' command */
					return AT_TYPE_WRITE;
				}
			}
			else
			{
				/* '=' on its own is not a valid AT command format  */
				return AT_TYPE_ERROR;
			}
		}
		else if (mCmdToken[0]=='?')
		{
			/* '?' on its own is the 'Read' command */
			return AT_TYPE_READ;
		}
		else
		{
			/* Syntax error */
			return AT_TYPE_ERROR;
		}
	}
	else
	{
		/* No tokens following the command */
		/* Must be the 'Action' command */
		return AT_TYPE_ACTION;
	}
}

/*****************************************************************************
 * at_help_CmdHandler
 * Display available AT commands
 *****************************************************************************/
static bool at_help_CmdHandler(void)
{
	char localStr [64];
    uint16_t i;
    bool result = false;
	atCommandType_e atCommandType;
    const atCmdTable_t *pAtCmdTable = atCmdTable;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_ACTION:
		/* Just display all of the available AT commands */
		sprintf ( tmpStr, "Available AT commands:\n" );

		/* Step through the AT command list... */
	    for (i = 0; i < AT_CMD_ID_COUNT; i++, pAtCmdTable++)
	    {
			/* ... and display the ASCII labels */
			sprintf ( localStr, " - %s, %s\n", pAtCmdTable->command, pAtCmdTable->description );
			strcat (tmpStr, localStr);
		}

		/* OK */
        result = true;
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Displays available AT commands\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

/*****************************************************************************
 * at_pati_CmdHandler
 * Show Manufacturers Information
 *****************************************************************************/
bool at_pati_CmdHandler ( void )
{
	char localStr [64];
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		// Show device identification information
		sprintf ( localStr, "%s\n", baseboardEepromData.manufacturer);
		strcat (tmpStr, localStr);
		sprintf ( localStr, "%s, %s\n", baseboardEepromData.baseboardType, baseboardEepromData.baseboardRevision );
		strcat (tmpStr, localStr);
		sprintf ( localStr, "SN: %s\n", baseboardEepromData.baseboardSerialNo);
		strcat (tmpStr, localStr);
		sprintf ( localStr, "%s, %s\n", hostModuleType, System.deviceID().c_str());
		strcat (tmpStr, localStr);
		sprintf ( localStr, "Device OS: %s\n", System.version().c_str());
		strcat (tmpStr, localStr);
		sprintf ( localStr, "Gateway firmware: %s\n", gatewayFirmwareVer);
		strcat (tmpStr, localStr);
		sprintf ( localStr, "Protocol library: %s\n", protocolLibraryVer);
		strcat (tmpStr, localStr);
	
		/* OK */
        result = true;
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Show Manufacturers Information\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

extern EEPROM_24LC01 baseboardEeprom;

/*****************************************************************************
 * at_pmfg_CmdHandler
 * Set Manufacturer Identification string in the baseboard EEPROM
 * Part of the Manufacturers Information
 *****************************************************************************/
bool at_pmfg_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "%s\n", baseboardEepromData.manufacturer);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		if ( strlen ( mCmdToken ) < sizeof ( baseboardEepromData.manufacturer ) )
		{
			/* Write to baseboard eeprom */
			strcpy(baseboardEepromData.manufacturer, mCmdToken);
			baseboardEeprom.put ( offsetof ( baseboardEepromData_t, manufacturer ), baseboardEepromData.manufacturer );

			/* OK */
			result = true;
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf(tmpStr, "Manufacturer: <manufacturer>\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

/*****************************************************************************
 * at_pbbt_CmdHandler
  * Set Baseboard Type in the baseboard EEPROM
 * Part of the Manufacturers Information
*****************************************************************************/
bool at_pbbt_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "%s\n", baseboardEepromData.baseboardType);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		if ( strlen ( mCmdToken ) < sizeof ( baseboardEepromData.baseboardType ) )
		{
			/* Update the configuration and write to baseboard eeprom */
			strcpy(baseboardEepromData.baseboardType, mCmdToken);
			baseboardEeprom.put ( offsetof ( baseboardEepromData_t, baseboardType ), baseboardEepromData.baseboardType );

			/* OK */
			result = true;
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Baseboard Type: <type>\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

/*****************************************************************************
 * at_pbbr_CmdHandler
 * Set Baseboard Revision in the baseboard EEPROM
 * Part of the Manufacturers Information
 *****************************************************************************/
bool at_pbbr_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "%s\n", baseboardEepromData.baseboardRevision);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		if ( strlen ( mCmdToken ) < sizeof ( baseboardEepromData.baseboardRevision ) )
		{
			/* Update the configuration and write to baseboard eeprom */
			strcpy(baseboardEepromData.baseboardRevision, mCmdToken);
			baseboardEeprom.put ( offsetof ( baseboardEepromData_t, baseboardRevision ), baseboardEepromData.baseboardRevision );

			/* OK */
			result = true;
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Baseboard Revision: <revision>\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

/*****************************************************************************
 * at_pbbs_CmdHandler
 * Set Baseboard Serial Number in the baseboard EEPROM
 * Part of the Manufacturers Information
 *****************************************************************************/
bool at_pbbs_CmdHandler ( void )
{
	static uint32_t lastValue = 0L;
	static uint32_t attempts = 0L;
    uint64_t val;
	char localStr[10];
	bool validToken;
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "SN: %s\n", baseboardEepromData.baseboardSerialNo);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied */
		/* Get the new serial number */
		/* Unsigned integer, > 0 and up to 7 digits */
		val = at_GetNumber ( &validToken );
		if ( validToken )
		{
			if ( ( val > 0LL ) && ( val <= 9999999LL ) )
			{
				/* Parameter is valid */
				
				/* The serial number can only be written once and thereafter cannot be modified
				* This is achieved by checking for the default value '0000000' in the EEPROM */
				if ( !strcmp ( "0000000", baseboardEepromData.baseboardSerialNo ) )
				{
					/* New device, set the serial number */
					sprintf ( localStr, "%07lu", ( uint32_t ) val );
					strcpy ( baseboardEepromData.baseboardSerialNo, localStr);
					baseboardEeprom.put(offsetof(baseboardEepromData_t, baseboardSerialNo), baseboardEepromData.baseboardSerialNo);

					/* OK */
					result = true;
				}
				else
				{
					/* Serial number has already been set */
					/* It is possible that a serial number has been set incorrectly so a mechanism is required */
					/* whereby an incorrect serial number can be changed                                       */
					/* The method chosen here is to force the user to enter the same new number three times    */
					if ( attempts == 0 )
					{
						lastValue = ( uint32_t ) val;
						attempts++;

					}
					else if ( lastValue == ( uint32_t ) val )
					{
						attempts++;
						if ( attempts > 2 )
						{
							/* OK, that's three consecutive attempts to write the same serial number */
							/* Amend the existing serial number */
							sprintf ( localStr, "%07lu", ( uint32_t ) val );
							strcpy ( baseboardEepromData.baseboardSerialNo, localStr);
							baseboardEeprom.put(offsetof(baseboardEepromData_t, baseboardSerialNo), baseboardEepromData.baseboardSerialNo);

							/* Reset counter */
							attempts = 0L;

							/* OK */
							result = true;
						}
					}
					else
					{
						/* Reset everything */
						attempts = 0L;
					}
				}
			}
		}
		if (!result) {
			/* Parameter is either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "SN: <1-9999999>\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

/*****************************************************************************
 * at_pspt_CmdHandler
 * Read/set the protocol type using the corresponding ID value
 * 
 * Note that any other necessary settings, such as baud rate, must be
 * configured separately
 *****************************************************************************/
bool at_pspt_CmdHandler ( void )
{
	char localStr [ 128 ];
	bool validToken;
	uint64_t val;
    uint16_t i;
	protocolId_e protocolId;
    bool result = false;
	atCommandType_e atCommandType;
    const protocolHandlerTable_t *pProtocolHandlerTable = protocolHandlerTable;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		if (protocolHandlerTable[particleEepromData.protocolId].type){
			sprintf ( tmpStr, "Protocol Type: %u, \"%s\" on %s interface\n", 
										particleEepromData.protocolId, 
										protocolHandlerTable[particleEepromData.protocolId].label, 
										interfaceTypeTable[protocolHandlerTable[particleEepromData.protocolId].type].label);
		}
		else {
			sprintf ( tmpStr, "Protocol Type: %u, \"%s\"\n", 
										particleEepromData.protocolId, 
										protocolHandlerTable[particleEepromData.protocolId].label );
		}

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, should be the numeric protocol ID */
		/* Update and apply new settings (subject to validation) */
		val = at_GetNumber ( &validToken );
		if (validToken ) {
			protocolId = ( protocolId_e ) val;
			if ( ( protocolId >= 0 ) && ( protocolId < PROTOCOL_COUNT ) ) {

				/* Update the configuration held in eeprom */
				/* Including serial baud rate and framing parameters for the new protocol */
				particleEepromData.protocolId = protocolId;
				EEPROM.put(offsetof(particleEepromData_t, protocolId), particleEepromData.protocolId);

				/* Apply the configuration */
				setProtocolType ( protocolId );

				/* OK */
				result = true;
			}
		}
		if (!result) {
			/* Parameter is either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Protocol Type: <type>:\n");
		/* ... and display the IDs and corresponding ASCII labels and physical interface */
		for (i = 0; i < PROTOCOL_COUNT; i++, pProtocolHandlerTable++)
		{
			if (pProtocolHandlerTable->type){
				sprintf ( localStr, " - %02u, %s (%s)\n",	pProtocolHandlerTable->id, 
															pProtocolHandlerTable->label, 
															interfaceTypeTable[pProtocolHandlerTable->type].label);
			}
			else {
				sprintf ( localStr, " - %02u, %s\n",		pProtocolHandlerTable->id, 
														pProtocolHandlerTable->label );
			}
			strcat ( tmpStr, localStr );
		}

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat (responseStr, tmpStr);

	return result;
}

/*****************************************************************************
 * at_pipr_CmdHandler
 * Read/Set the serial baud rate
 *****************************************************************************/
bool at_pipr_CmdHandler ( void )
{
	char localStr [ 32 ];
    int i;
    bool result = false;
	atCommandType_e atCommandType;
    const serialBaudTable_t *pSerialBaudTable = serialBaudTable;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Serial baud rate: %lu\n", particleEepromData.panelSerialBaud);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, scan the serial baud rate table */
		/* looking for a match */
		/* Should be one of the preferred numeric protocol values, 9600, 19200 etc */
	    for (i = 0; i < SERIAL_BAUD_ID_COUNT; i++, pSerialBaudTable++)
	    {
	        if (!strcmp(mCmdToken, pSerialBaudTable->label))
	        {
				/* Found a match */

				/* Update and apply the configuration */
				particleEepromData.panelSerialBaud = atoi(mCmdToken);
		      	EEPROM.put(offsetof(particleEepromData_t, panelSerialBaud), particleEepromData.panelSerialBaud);
				Serial1.begin (particleEepromData.panelSerialBaud, serialFramingTable [ particleEepromData.serialFramingId ].regValue);
			
				/* OK */
                result = true;
	            break;
	        }
	    }
		if (!result) {
			/* Parameter is either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Serial baud rate: <rate>:\n");

		/* Step through all of the available serial baud rates... */
		for (i = 0; i < SERIAL_BAUD_ID_COUNT; i++, pSerialBaudTable++)
		{
			/* ... and display the ASCII label */
			sprintf ( localStr, " - %s\n", pSerialBaudTable->label);
			strcat ( tmpStr, localStr );
		}

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_picf_CmdHandler
 * Read/Set the serial character framing
 *****************************************************************************/
bool at_picf_CmdHandler ( void )
{
	char localStr [32];
    int i;
    bool result = false;
	atCommandType_e atCommandType;
    const serialFramingTable_t *pSerialFramingTable = serialFramingTable;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Serial character framing: %s\n", serialFramingTable[particleEepromData.serialFramingId].label);
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, scan the serial framing table */
		/* looking for a match */
	    for (i = 0; i < SERIAL_FRAMING_ID_COUNT; i++, pSerialFramingTable++)
	    {
			/* I know strcasecmp is not in the C or C++ standard but it works here */
			/* I doubt this lapse will present any insurmountable portability issues */
	        if (!strcasecmp(mCmdToken, pSerialFramingTable->label))
	        {
				/* Found a match */

				/* Update and apply the configuration */
				particleEepromData.serialFramingId = i;
				Serial1.begin (particleEepromData.panelSerialBaud, serialFramingTable [ particleEepromData.serialFramingId ].regValue);
			
				/* OK */
                result = true;
	            break;
	        }
	    }
		if (!result) {
			/* Parameter is either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Serial character framing: <framing>:\n" );

		/* Step through all of the available serial framing types... */
		for (i = 0; i < SERIAL_FRAMING_ID_COUNT; i++, pSerialFramingTable++)
		{
			/* ... and display the ASCII label */
			sprintf ( localStr, " - %s\n", pSerialFramingTable->label );
			strcat ( tmpStr, localStr );
		}

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_psts_CmdHandler
 * Read panel status
 * 
 * This command reports Panel Status in terms of two timestamp values:
 * 
 * - Last Heard is the last time the Gateway received communication from the 
 *   Panel, and is updated on every valid message received. These messages 
 *   could be events of interest to Nimbus, but not necessarily
 * 
 * - Last Handshake is the last time the Gateway and Panel exchanged a 
 *   successful periodic handshake
 * 
 * The latter is only applicable to certain protocols. If the configured 
 * protocol does not support a periodic handshake, the Last Handshake value 
 * will always return zero. If no protocol is configured, both the Last 
 * Heard and Last Handshake values are returned as zero.
 *****************************************************************************/
bool at_psts_CmdHandler ( void )
{
    bool result = false;
	return result;
}

/*****************************************************************************
 * at_pmadr_CmdHandler
 * Protocol-specific command used to set the Morley ZX Panel Address <01-32>
 *****************************************************************************/
bool at_pmadr_CmdHandler ( void )
{
    uint64_t val;
	bool validToken;
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Morley ZX Panel Address: %u\n", particleEepromData.morleyZxPanelAddr);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, write new setting (subject to validation)
		 * Should be one of either 0 or 1 */
		val = at_GetNumber ( &validToken );
		if (validToken ) {
			if ((val >= 1 ) && (val <= 32 ))
			{
				/* Parameter is valid */
				/* Update the configuration */
				particleEepromData.morleyZxPanelAddr = val;
				EEPROM.put(offsetof(particleEepromData_t, morleyZxPanelAddr), particleEepromData.morleyZxPanelAddr);

				/* Apply the configuration */
				/* ??? */
			
				/* OK */
				result = true;
			}
		}
		if (!result) {
			/* Parameter is either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n" );
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Morley ZX Panel Address: <01-32>\n" );

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_piow_CmdHandler
 * Write a value to Gateway outputs
 *****************************************************************************/
bool at_piow_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Console message */
	sprintf ( tmpStr, "Not yet implemented\n" );

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* OK */
        result = true;
		break;

	case AT_TYPE_ACTION:
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_pior_CmdHandler
 * Read a value from Gateway inputs
 *****************************************************************************/
bool at_pior_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Console message */
	sprintf ( tmpStr, "Not yet implemented\n" );

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
		break;

	case AT_TYPE_WRITE:
		break;

	case AT_TYPE_ACTION:
		break;

	case AT_TYPE_TEST:
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_ptgt_CmdHandler
 * Configure Nimbus Target
 * This is a string prefix, typically 'nimbus/dev', or 'nimbus/pre'
 * identifying the target server
 *****************************************************************************/
bool at_ptgt_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Nimbus Target: \"%s\"\n", particleEepromData.nimbusTargetServer);

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		if ( strlen ( mCmdToken ) < sizeof ( particleEepromData.nimbusTargetServer ) )
		{
			/* Update the configuration and write to Particle eeprom */
			strcpy(particleEepromData.nimbusTargetServer, mCmdToken);
			EEPROM.put(offsetof(particleEepromData_t, nimbusTargetServer), particleEepromData.nimbusTargetServer);

			/* OK */
			result = true;
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Nimbus Target: <target>\n" );

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_psid_CmdHandler
 * Read/Set a Nimbus Session ID
 * This is a unique 32-bit number used by Nimbus Mobile applications
 * It is appended to Nimbus transfers
 *****************************************************************************/
bool at_psid_CmdHandler ( void )
{
	bool validToken;
    uint64_t val;
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Session ID: %lu\n", particleEepromData.nimbusSessionId );

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, write new setting (subject to validation)
		 * Should be an unsigned integer, 32 bits */
		val = at_GetNumber ( &validToken );
		if (validToken ) {
			if ( ( val >= 0LL ) && ( val <= UINT_MAX ) )
			{
				/* Parameter is valid */
				/* Update the configuration */
				particleEepromData.nimbusSessionId = val;
				EEPROM.put(offsetof(particleEepromData_t, nimbusSessionId), particleEepromData.nimbusSessionId );

				/* OK */
				result = true;
			}
		}
		if (!result) {
			/* Either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n" );
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Session ID: <id>\n" );

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_pxfr_CmdHandler
 * Enable/disable Nimbus transfers
 * TBD
 * Requires further clarification re what happens to incoming events
 *****************************************************************************/
bool at_pxfr_CmdHandler ( void )
{
    uint64_t val;
	bool validToken;
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Nimbus Transfers: %u (%s)\n", particleEepromData.enableNimbusTransfers, 
													( particleEepromData.enableNimbusTransfers ) ? "Enabled" : "Disabled");

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, write new setting (subject to validation)
		 * Should be numeric, one of either 0 or 1 */
		val = at_GetNumber ( &validToken );
		if (validToken ) {
			if ( ( val == 0LL ) || ( val == 1LL ) )
			{
				/* Parameter is valid */
				/* Update the configuration */
				particleEepromData.enableNimbusTransfers = val;
				EEPROM.put(offsetof(particleEepromData_t, enableNimbusTransfers), particleEepromData.enableNimbusTransfers);

				/* OK */
				result = true;
			}
		}
		if (!result) {
			/* Either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n" );
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Nimbus Transfers: <0 or 1>\n0 - Disabled\n1 - Enabled\n" );

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_psal_CmdHandler
 * Set Authority Level
 * 0 - Limited read-only access
 * 1 - Full read/write access
 *****************************************************************************/
bool at_psal_CmdHandler ( void )
{
    uint64_t val;
	bool validToken;
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current athority level */
		sprintf( tmpStr, "Authority Level: %u\n", authorityLevel );

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* Changing the authority level requires two parameters:
		 * - requested authority level (0 or 1)
		 * - the password string */

		/* Authority level */
		val = at_GetNumber ( &validToken );
		if (validToken ) {
			if ( val == 0LL ) {
				/* Parameter is valid */
				/* Password is not required to set this authority level */
				authorityLevel = ( authLevel_e ) val;

				/* OK */
				result = true;
			}
			else if ( val == 1LL ) {
				/* A password is required to set level 1 */
				/* This should be the next parameter */
				at_GetToken();
				if (mCmdToken[0])
				{
					/* We have something, is it the password ? */
					if (strcmp(particleEepromData.atCommandPassword, mCmdToken) == 0)
					{
						/* Password match, set the level */
						authorityLevel = ( authLevel_e ) val;

						/* OK */
						result = true;
					}
					else {
						sprintf( tmpStr, "Invalid password\n" );
					}
				}
				else {
					sprintf( tmpStr, "Password required to set this level\n" );
				}
			}
			else {
				sprintf( tmpStr, "Level must be <0 or 1>\n" );
			}
		}
		if (!result) {
			/* Problem with parameter(s) */
			strcat ( tmpStr, "Invalid parameter\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf( tmpStr, "<level>,<password>\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_ppwd_CmdHandler
 * Manage password
 * Can be used to change the password
 *****************************************************************************/
bool at_ppwd_CmdHandler ( void )
{
	char oldPasswd [ 8 ];
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_WRITE:
		/* Two parameters required, the old and the new password */
		/* Old one first */
		if ( strlen (mCmdToken) <= MAX_PASSWORD_LENGTH ) {
			strcpy (oldPasswd, mCmdToken);

			/* Get the new one */
			at_GetToken();
			if ( mCmdToken[0] && ( strlen ( mCmdToken ) <= MAX_PASSWORD_LENGTH ) ) {
				/* OK we have two passwords, the old and the new */
				/* Does the first password match the stored value ? */
				if (strcmp(particleEepromData.atCommandPassword, oldPasswd) == 0) {
					/* Password match, update the stored password value */
					strcpy(particleEepromData.atCommandPassword, mCmdToken);

					/* OK */
					result = true;
				}
				else {
					sprintf ( tmpStr, "Password doesn't match\n" );
				}
			}
		}
		if (!result) {
			/* Invalid parameter(s) */
			strcat ( tmpStr, "Invalid parameter(s)\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Set new password\n");
		strcat ( tmpStr, "Syntax: <old passwd>,<new passwd>\n");
		strcat ( tmpStr, "Each up to 7 characters\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_cpwr_CmdHandler
 * Enable/disable power to the cellular modem. Used to prevent the modem from
 * powering up unless you really want it to. Useful during development when 
 * the device might be undergoing frequent reset or power cycles for an 
 * extended period
 * 
 * From Particle:
 * You must not turn off and on cellular more than every 10 minutes (6 times 
 * per hour). Your SIM can be blocked by your mobile carrier for aggressive 
 * reconnection if you reconnect to cellular very frequently 
 ******************************************************************************/
bool at_cpwr_CmdHandler ( void )
{
    uint64_t val;
	bool validToken;
    bool result = false;
	atCommandType_e atCommandType;

	atCommandType = at_GetCommandType();
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
	case AT_TYPE_ACTION:
		/* Report current setting */
		sprintf ( tmpStr, "Cellular power: %u (%s)\n", particleEepromData.cellPower, 
													( particleEepromData.cellPower ) ? "On" : "Off" );

		/* OK */
        result = true;
		break;

	case AT_TYPE_WRITE:
		/* A parameter has been supplied, write new setting (subject to validation)
		 * Should be numeric, one of either 0 or 1 */
		val = at_GetNumber ( &validToken );
		if (validToken ) {
			if ((val == 0 ) || (val ==1 )) {
				/* Parameter is valid */
				/* Update the configuration */
				particleEepromData.cellPower = val;
				EEPROM.put(offsetof(particleEepromData_t, cellPower), particleEepromData.cellPower);

				/* Apply the configuration */
				/* ??? */
			
				/* OK */
				result = true;
			}
		}
		if (!result) {
			/* Parameter is either non numeric or out of range */
			sprintf ( tmpStr, "Invalid parameter\n");
		}
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Cellular power: <0 or 1>\n0 - Off\n1 - On\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_prst_CmdHandler
 * Can be used to invoke device reset
 * Can also be used to report last reset reason
 *****************************************************************************/
bool at_prst_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_READ:
		/* Report reason for last reset */
		sprintf ( tmpStr, "Reset reason: " );
		switch (System.resetReason()) {
			case RESET_REASON_NONE: 			strcat ( tmpStr, "Information is not available"); break;
			case RESET_REASON_UNKNOWN: 			strcat ( tmpStr, "Unspecified reset reason"); break;
			case RESET_REASON_PIN_RESET: 		strcat ( tmpStr, "Reset button or reset pin"); break;
			case RESET_REASON_POWER_MANAGEMENT: strcat ( tmpStr, "Low-power management reset"); break;
			case RESET_REASON_POWER_DOWN: 		strcat ( tmpStr, "Power-down reset"); break;
			case RESET_REASON_POWER_BROWNOUT: 	strcat ( tmpStr, "Brownout reset"); break;
			case RESET_REASON_WATCHDOG: 		strcat ( tmpStr, "Hardware watchdog reset"); break;
			case RESET_REASON_UPDATE: 			strcat ( tmpStr, "Successful firmware update"); break;
			case RESET_REASON_UPDATE_ERROR: 	strcat ( tmpStr, "Firmware update error, deprecated"); break;
			case RESET_REASON_UPDATE_TIMEOUT: 	strcat ( tmpStr, "Firmware update timeout"); break;
			case RESET_REASON_FACTORY_RESET: 	strcat ( tmpStr, "Factory reset requested"); break;
			case RESET_REASON_SAFE_MODE: 		strcat ( tmpStr, "Safe mode requested"); break;
			case RESET_REASON_DFU_MODE: 		strcat ( tmpStr, "DFU mode requested"); break;
			case RESET_REASON_PANIC: 			strcat ( tmpStr, "System panic"); break;
			case RESET_REASON_USER: 			strcat ( tmpStr, "User-requested reset"); break;
			default:							strcat ( tmpStr, "Unknown"); break;
		}
		strcat (tmpStr, "\n");
		/* OK */
        result = true;
		break;

	case AT_TYPE_ACTION:
		/* Invoke soft reset */
		System.reset();

		/* OK */
        result = true;
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Report reason for last reset, or invoke soft reset\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}

/*****************************************************************************
 * at_prfd_CmdHandler
 * Restore Factory Defaults
 *****************************************************************************/
bool at_prfd_CmdHandler ( void )
{
    bool result = false;
	atCommandType_e atCommandType;

	/* Determine command type - one of READ, WRITE, TEST or ACTION */
	atCommandType = at_GetCommandType();

	/* Handle command according to type */
	switch ( atCommandType )
	{
	case AT_TYPE_ACTION:
		/* Restore Factory Defaults */
		resetParticleEeprom();
		resetBaseboardEeprom();

		/* OK */
        result = true;
		break;

	case AT_TYPE_TEST:
		/* Explain command purpose/syntax */
		sprintf ( tmpStr, "Restore Factory Defaults\n");

		/* OK */
        result = true;
		break;
	}

	/* Append command response to the response string */
	strcat ( responseStr, tmpStr );

	return result;
}
