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
 * Module :     cli
 * Description: Command Line Interpreter
 *              Handles commands typed into the console
 *
 *****************************************************************************/

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include "build.h"
#include "cli.h"
#include "cli_edit.h"
#include "cli_hist.h"
#include "at_commands.h"

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/* A state machine is used to track escape sequences */
/* during VT100 cursor control */
typedef enum
{
    CLI_ESC_SEQ_0,
    CLI_ESC_SEQ_1,
    CLI_ESC_SEQ_2,
    CLI_ESC_SEQ_3
} CLI_ESC_STATE;

/*****************************************************************************
 * Data
 *****************************************************************************/

/* Resources for tracking state machine */
CLI_ESC_STATE esc_state;

/* Console characteristics */
uint8_t cliEcho;
uint8_t isVT100;

/* Resources for holding typed commands */
uint8_t line_idx;
uint8_t line_end;
char rx_line[MAX_CMDLINE];

/*************************************************************************
 *  Prototypes
 **************************************************************************/
static uint8_t check_esc_sequence(uint8_t rx_char);
void add_char_to_line_buffer(char rx_char);
static void local_edit(uint8_t rx_char);

/*************************************************************************
 * cliInitialise
 **************************************************************************/
void cliInitialise(void)
{
    history_init();

    /* initialise edit */
    line_idx = 0u;
    line_end = 0u;
    cliEcho = true;
    rx_line[0] = (char)0;
    isVT100 = false;

    esc_state = CLI_ESC_SEQ_0;
}

/*************************************************************************
 * cliEcho control - Set/Clr/Get
 **************************************************************************/
void cliSetEcho(void)
{
    cliEcho = true;
}

void cliClrEcho(void)
{
    cliEcho = false;
}

bool cliGetEcho(void)
{
    return cliEcho;
}

/*************************************************************************
 * add_char_to_line_buffer
 * Add non "control" character into the
 * current line
 **************************************************************************/
void add_char_to_line_buffer(char rx_char)
{
    /* check that won't go off the end of the line */
    if ((line_end + 1u) < MAX_CMDLINE)
    {
        /* if we're at end of line, just append incoming character */
        if (line_idx == line_end)
        {
            /* store character at the end of the buffer */
            rx_line[line_idx] = rx_char;
            line_idx++;

            /* add terminator to modified line */
            rx_line[line_idx] = (char)0;

            /* adjust end index */
            line_end = line_idx;

            /* and echo character */
            if (cliEcho == true)
            {
                 Serial.write(rx_char);
            }
        }
        else
        {
            uint8_t i, n = 0u;

            /* must be editing the line, have to move existing chars up one */
            for (i = line_end + 1u; i > line_idx; i--)
            {
                rx_line[i] = rx_line[i - 1u];
                n++; /* count how many we moved for later */
            }

            /* then store char in current position */
            rx_line[line_idx] = rx_char;

            /* add terminator to modified line */
            line_end++;
            rx_line[line_end] = (char)0;

            /* redisplay the rest of the line */
            if (cliEcho == true)
            {
                Serial.printf(&rx_line[(uint16_t)line_idx]);
            }

            /* move back to original posn */
            move_cursor_back_n(n - 1u);
            line_idx++;
        }
    }
}

/* cursor control sequences ( VT100 ) */

/*********************************************************
 * move_cursor_back_n  
 * move cursor back n characters
 *********************************************************/
void move_cursor_back_n(uint8_t len)
{
    static const char cur_left[] =
    { (char)ESC, '[', 'D', 0u };

    if (len)
    {
        /* if the terminal is a VT100 , can use quicker sequence */
        if (isVT100 == true)
        {
            if (len > (size_t)1)
            {
                Serial.printf("%c[%uD", ESC, len);
            }
            else
            {
                Serial.printf("%s", cur_left);
            }
        }
        else
        {
            /* not a VT100 terminal - do it the slow ( but sure ) way */
            while (len)
            {
                len--;
                Serial.write ( BS );
            }
        }
    }
}

/*********************************************************
 * clear_to_eol
 * clear from cursor to end of line
 *********************************************************/
void clear_to_eol(void)
{
    uint8_t len, i;

    len = line_end - line_idx;
    for (i = 0u; i < len; i++)
    {
        Serial.write(' ');
    }

    /* move back to original place */
    move_cursor_back_n(len);
}

/*********************************************************
 * delete_string
 * delete "count" characters from input line
 *********************************************************/
void delete_string(uint8_t len)
{
    int8_t i;
    uint8_t j;
    char *p;

    if (line_end != line_idx)
    {
        /* check for deleting last character on the line */
        if ((len == 1u) && (line_idx == line_end - 1u))
        {
            line_end--;
            rx_line[line_idx] = (char)0;
            Serial.write(' ');
            move_cursor_back_n(len);
        }
        else
        {
            if (line_idx + len > line_end)
            {
                len = line_end - line_idx;
            }

            if (len != 0u)
            {
                /* clear from cursor to end of line */
                clear_to_eol();

                /* remove "count" characters from rx_line */
                /* by moving the rest down */
                j = line_idx;
                for (i = ((int8_t)line_end - ((int8_t)line_idx + (int8_t)len) + 1); --i >= 0; j++)
                {
                    rx_line[j] = rx_line[j + len];
                }
                line_end -= len;
                rx_line[line_end] = (char)0;

                p = &rx_line[(uint16_t)line_idx];
                /* output new string */
                Serial.printf(p);

                /* adjust cursor */
                move_cursor_back_n(strlen(p));
            }
        }

    }
}

/*********************************************************
 * edit_del_char
 *********************************************************/
void edit_del_char(void)
{
    delete_string(1u);
}

/*********************************************************
 * edit_bk_char
 *********************************************************/
void edit_bk_char(void)
{
    if (line_idx)
    {
        move_cursor_back_n(1u);
        line_idx--;
    }
}

/*********************************************************
 * edit_fd_char
 *********************************************************/
void edit_fd_char(void)
{
    if (line_idx < line_end)
    {
        Serial.write(rx_line[line_idx]);
        line_idx++;
    }
}

/*************************************************************************
 * local_edit
 * performs local edit decoding and action of control characters
 **************************************************************************/
static void local_edit(unsigned char rx_char)
{
    switch (rx_char)
    {
        case CR: /* CR causes entire line to be forwarded */
            edit_accept_line();
            break;

        case CTRL_B: /* move cursor back one character */
            edit_bk_char();
            break;

        case CTRL_F: /* moves the cursor forward one character */
            edit_fd_char();
            break;

        case BS:
            /* backspace character processing */
            edit_bk_del_char();
            break;

        case CTRL_N: /* enters next line in history buffer */
            edit_h_next();
            break;

        case CTRL_P: /* enters previous line in command buffer */
            edit_h_prev();
            break;

#ifdef CLI_EDIT

        case CTRL_D: /* deletes the character at the cursor */
        case DEL:
            edit_del_char ();
            break;

        case CTRL_E: /* jumps to the end of the current command */
            edit_end_line ();
            break;

        case CTRL_A: /* jump to first character of command line */
            edit_beg_line ();
            break;

        case CTRL_K: /* deletes from cursor to end of command line */
            edit_del_eol ();
            break;

        case CTRL_L: /* repeats current line on a new line */
        case CTRL_R: /* repeats current line on a new line */
            edit_redisplay ();
            break;

        case CTRL_U: /* deletes from cursor to beginning of command line */
        case CTRL_X: /* deletes from cursor to beginning of command line */
            edit_del_beg ();
            break;

        case CTRL_W: /* deletes last word typed */
            edit_del_word ();
            break;

#endif
        default:
            /* ignore */
            break;
    }
}

/*************************************************************************
 * edit_accept_line
 * Performs appropriate actions to pass the entire command
 * line to the rest of the system.
 * Updates the history.
 **************************************************************************/
void edit_accept_line(void)
{
    char *str;

    /* Echo newline back to client */

    if (cliEcho == true)
    {
        Serial.write("\n");
    }

    /* make sure there is a 0 on the end of the string */
    rx_line[line_end] = (char)0;

    str = rx_line;

    /* if nothing else on the line */
    if (line_end != 0u)
    {
        /* do not store passwords in the history ! */
        if (cliEcho == true)
        {
            /* add rx_line to history */
            str = history_add();
            /* this should return a pointer to a history entry */
            if (str != NULL)
            {
                /* see if it the same as the command line typed */
                if (strcmp(str, rx_line) != 0)
                {
                    /* a command substitution took place , echo the
                     new command line after the sustitution */
                    Serial.printf("%s\n", str);
                }
            }
        }
    }

    /* clear the command line buffer indexes */
    line_idx = 0u;
    line_end = 0u;

    /* pass buffer to Parser */
    at_CommmandParser(str);
}

/*************************************************************************
 * cliScan
 * Called as often as required to process command line
 **************************************************************************/
void cliScan(void)
{
    uint8_t rx_char;
    uint8_t ch;

    // process all pending uart rx data
    for (;;)
    {
        if ( Serial.available() == 0 )      // see if character
        {
            break;
        }

        // get next character
        rx_char = Serial.read();

        /* translate VT100 cursor sequences etc   */
        ch = check_esc_sequence(rx_char);

        /* ignore rubbish during ESC sequence  */

        if (ch != 0u)
        {
            /* check for control character */
            if ((ch < (uint8_t)' ') || (ch >= (uint8_t)DEL))
            {
                /* perform local edit functions etc */
                local_edit(ch);
            }
            else
            {
                /* normal character processing */
                add_char_to_line_buffer(rx_char);
            }
        }
    }
}

/*********************************************************
 * check for VT100 cursor control characters
 * and translate if possible
 *  ESC [ A - cursor up
 *  ESC [ B - cursor down
 *  ESC [ C - cursor forward
 *  ESC [ D - cursor backward
 *  ESC [ 4 ~ - delete key
 *  NB ESC can mean cursor control keys , or just ESC key
 * returns translated character
 *  0 if unrecognised "esc" sequence
 * Also translates some other ESC sequences to
 * meta-characters
 *  ESC B - ESC_B
 *  ESC D - ESC_D
 *  ESC F - ESC_F
 *********************************************************/
static uint8_t check_esc_sequence(uint8_t rx_char)
{
    uint8_t result = 0u;

    /* check that the esc processing is active */
    switch (esc_state)
    {
        /* ESC decode not active - check for ESC character */
        case CLI_ESC_SEQ_0:
            if (rx_char != ESC)
            {
                result = rx_char;
            }
            else
            {
                /* start esc mode decode */
                esc_state = CLI_ESC_SEQ_1;
            }
            break;

            /* any chars received while in ESC mode are silently */
            /* discarded while an attempt is made to decode the */
            /* sequence */

        case CLI_ESC_SEQ_1:
            /* seen ESC char already */
            esc_state = CLI_ESC_SEQ_0;
            if (rx_char == (uint8_t)'[')
            {
                /* VT100 cursor control sequence start */
                esc_state = CLI_ESC_SEQ_2;
            }
            break;

        case CLI_ESC_SEQ_2:
            esc_state = CLI_ESC_SEQ_0;
            switch (rx_char)
            {
                /* cursor up  translate to CTRL_P */
                case (uint8_t)'A':
                    isVT100 = true;
                    result = CTRL_P;
                    break;

                    /* cursor down  translate to CTRL_N */
                case (uint8_t)'B':
                    isVT100 = true;
                    result = CTRL_N;
                    break;

                    /* cursor forward translate to CTRL_F */
                case (uint8_t)'C':
                    isVT100 = true;
                    result = CTRL_F;
                    break;

                    /* cursor backward  translate to CTRL_B */
                case (uint8_t)'D':
                    isVT100 = true;
                    result = CTRL_B;
                    break;

                case (uint8_t)'4':
                    esc_state = CLI_ESC_SEQ_3;
                    break;

                /* unrecognised sequence ? */
                default:
                    break;
            }
            break;

        case CLI_ESC_SEQ_3:
            esc_state = CLI_ESC_SEQ_0;
            if (rx_char == (uint8_t)'~')
            {
                /* VT100 delete key - translate to DEL */
                result = DEL;
            }
            break;

        default:
            /* unrecognised */
            esc_state = CLI_ESC_SEQ_0;
            break;
    }

    return result;
}

