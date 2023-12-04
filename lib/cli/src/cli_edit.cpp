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
 * Module :     cli_edit
 * Description: Suite of edit functions for the CLI
 *
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "cli.h"
#include "at_commands.h"
#include "cli_edit.h"
#include "cli_hist.h"
#include "build.h"

/*********************************************************
* edit_bk_del_char
*********************************************************/
void edit_bk_del_char (void)
{
    if (line_idx)
    {
    	move_cursor_back_n (1u);
    	line_idx--;
    	delete_string (1u);
    }
}


/*********************************************************
* edit_del_eol
*********************************************************/
void edit_del_eol (void)
{
    clear_to_eol ();
    rx_line[line_idx] = (char)0;
    line_end = line_idx;
}

/*********************************************************
* edit_del_word
*********************************************************/
void edit_del_word(void)
{
    uint8_t cnt;
    uint8_t idx;

    cnt = 0u;
    idx = line_idx;

    /* scan back for word delimiter */
    while (cnt < line_idx)
    {
        if (rx_line[idx] == ' ')
        {
        	break;
        }
        idx--;
        cnt++;
    }
    move_cursor_back_n (cnt);
    line_idx -= cnt;
    delete_string (cnt);
}

/*********************************************************
* edit_del_beg
*********************************************************/
void edit_del_beg (void)
{
    uint8_t cnt;

    if (line_idx)
    {

    	cnt = line_idx;
    	move_cursor_back_n (line_idx);
    	line_idx = 0u;
    	delete_string (cnt);
    }
}

/*********************************************************
* edit_beg_line
*********************************************************/
void edit_beg_line (void)
{
    if (line_idx)
    {
    	move_cursor_back_n (line_idx);
    	line_idx = 0u;
    }
}

/*********************************************************
* edit_end_line
*********************************************************/
void edit_end_line (void)
{
    if (line_idx != line_end)
    {
    	Serial.printf("%s", &rx_line[line_idx] );
        line_idx = line_end;
    }
}

/*********************************************************
* edit_redisplay
*********************************************************/
void edit_redisplay (void)
{
    /* back to start of line */
    move_cursor_back_n (line_idx);
    line_idx = 0u;

    clear_to_eol ();

    /* output current line */
    Serial.printf("%s", rx_line );
    line_idx = (uint8_t)strlen (rx_line);
    line_end = line_idx;
}

/*********************************************************
* edit_h_next
*********************************************************/
void edit_h_next (void)
{
    /* back to start of line */
    move_cursor_back_n (line_idx);
    line_idx = 0u;

    clear_to_eol ();

    history_get_next ();

    /* output current line */
    Serial.printf("%s",rx_line );
    line_idx = (uint8_t)strlen (rx_line);
    line_end = line_idx;

}

/*********************************************************
* edit_h_prev
*********************************************************/
void edit_h_prev (void)
{
    /* back to start of line */
    move_cursor_back_n (line_idx);
    line_idx = 0u;
    clear_to_eol ();

    history_get_prev ();

    /* output current line */
    Serial.printf("%s",rx_line );
    line_idx = (uint8_t)strlen (rx_line);
   line_end = line_idx;
}
