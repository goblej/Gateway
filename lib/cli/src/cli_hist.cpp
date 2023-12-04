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

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "build.h"
#include "cli.h"
#include "at_commands.h"
#include "cli_edit.h"
#include "cli_hist.h"

/*****************************************************************************
 * Data
 *****************************************************************************/

char history[CMD_HISTORY_SIZE][MAX_CMDLINE];
uint8_t hist_idx;
int8_t hist_iterator;
uint8_t hist_first;

/*************************************************************************
 *  history_init
 **************************************************************************/
void history_init(void)
{
    int i;

    /* clear history */
    for (i = 0; i < CMD_HISTORY_SIZE; i++)
    {
        history[i][0] = 0;
    }
    hist_idx = 0;
    hist_iterator = -1;
    hist_first = 0;
}

/*************************************************************************
 *  history_add
 *  - parse current command line for command substitution
 *  - add line to history buffer
 *  - returns pointer to substituted command in history
 *    buffer, NULL if an error occurs
 **************************************************************************/
char *history_add (void)
{
	char *result;
    uint8_t last_hist;
    char *cmd_line;

    last_hist = hist_idx;
    cmd_line = rx_line;

    /* reset for "next/previous" */
    hist_iterator = -1;

    if (cmd_line)
    {
        /* add command line to history */
        /* if not the same as the latest history */
        if (strcmp (history[last_hist],
                    cmd_line) != 0)
        {
            /*  store a new history item and return a pointer to it */
            result =  history_store_new (cmd_line);
        }
        else
        {
            /* return pointer to last one */
            result = history[last_hist];
        }
    }
    else
    {
    	/* if all else fails - an error has occurred */
    	result = (char *)0;
    }
    return result;
}

/*************************************************************************
 *  history_store_new
 *  - store "str" in next history buffer position 
 *  - returns pointer to new history buffer
 **************************************************************************/
char *history_store_new (const char *str)
{
    /* if current history is blank , overwrite that one */
    if (strlen (history[hist_idx]) != (size_t)0)
    {
        /* add a new history */
        hist_idx++;
        if (hist_idx >= CMD_HISTORY_SIZE)
        {
        	hist_idx = 0u;
        }

        /* see if the history has wrapped */
        if (hist_idx == hist_first)
        {
            hist_first++;
            if (hist_first >= CMD_HISTORY_SIZE)
            {
            	hist_first = 0u;
            }
        }
    }

    /* store new history string */
    strcpy (history[hist_idx],str);
    return history[hist_idx];
}

/*************************************************************************
 *  history_get_prev
 **************************************************************************/
void history_get_prev (void)
{
    uint8_t idx;

    if (hist_iterator < 0)
    {
        /* get "first prev" i.e current */
        idx = hist_idx;
    }
    else
    {
        /* get prev */
        idx = (uint8_t)hist_iterator;
        /* see if reached first history */
        if (idx != hist_first)
        {
            /* back up to previous history */
        	if( idx )
            {
        		idx--;
            }
        	else
            {
            	idx = CMD_HISTORY_SIZE - 1u;
            }
        }
    }
    /* copy history line to buffer */
    strcpy (rx_line,
            history[idx]);

    /* set new idx's */
    hist_iterator = (int8_t)idx;
    line_end = (uint8_t)strlen (rx_line);
    line_idx = line_end;
}

/*************************************************************************
 *  history_get_next
 **************************************************************************/
void history_get_next (void)
{
    uint8_t idx;

    /* have to have previously done "previous" */
    if (hist_iterator >= 0)
    {
        /* get_next */
        idx = (uint8_t)hist_iterator;
        /* see if we have reached the end of the stored history */
        if (idx != hist_idx)
        {
            /* forward to next history */
            idx++;
            if (idx >= CMD_HISTORY_SIZE)
            {
            	idx = 0u;
            }

            hist_iterator = (int8_t)idx;
            /* copy next history line to buffer */
            strcpy (rx_line, history[idx]);
            line_end = (uint8_t)strlen (rx_line);
            line_idx = line_end;
        }
        else
        {
            /* clear iterator in case "get_prev" called next time */
            hist_iterator = -1;
        	/* show a blank line */
        	rx_line[0] = (char)0;
        	line_end = 0u;
        	line_idx = 0u;
        }
    }
    else
    {
    	/* can't get next */
    	/* show a blank line */
    	rx_line[0] = (char)0;
    	line_end = 0u;
    	line_idx = 0u;
    }
}

