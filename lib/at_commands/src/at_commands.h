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

#ifndef AT_COMMANDS_H
#define AT_COMMANDS_H

/*****************************************************************************
 * Includes
 *****************************************************************************/

/*****************************************************************************
 * Definitions
 *****************************************************************************/
typedef enum
{
    AT_CMD_TOKEN_NUMERIC,
    AT_CMD_TOKEN_NON_NUMERIC,
    AT_CMD_TOKEN_SIGNED_NUMERIC
} at_CmdTokenType_t;

/* Function prototype for at command handler table entries */
typedef bool (*atCmdHandler_t)(void);

/*****************************************************************************
 * Constants
 *****************************************************************************/
#define AT_MAX_TOKEN_SIZE	40
#define NULL_CMD_FN			((void *)0)

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
void at_Initialise(void);
void at_PrintPrompt(bool addCR);
void at_CommmandParser(char *cmdline);

/*****************************************************************************/
#endif /* AT_COMMANDS_H */
