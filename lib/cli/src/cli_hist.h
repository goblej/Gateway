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
 * Module :     cli_hist
 * Description: Suite of command history functions for the CLI
 *
 *****************************************************************************/

#ifndef CLI_HIST_H_
#define CLI_HIST_H_
#include "build.h"

/*****************************************************************************
 * Public functions
 *****************************************************************************/
extern void history_init(void);
extern char *history_add (void);
extern char *history_store_new (const char *str);
extern void history_get_prev (void);
extern void history_get_next (void);

/*****************************************************************************
 * Data
 *****************************************************************************/

// history
extern char history[CMD_HISTORY_SIZE][MAX_CMDLINE];
extern uint8_t hist_idx;
extern int8_t hist_iterator;
extern uint8_t hist_first;

#endif /* CLI_HIST_H_ */
