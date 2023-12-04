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

#ifndef CLI_H_
#define CLI_H_

#include "Particle.h"

/*****************************************************************************
 * Globals
 *****************************************************************************/

extern uint8_t line_idx;
extern uint8_t line_end;
extern uint8_t rx_rd_idx;
extern uint8_t rx_wr_idx;
extern char rx_line[];

/*****************************************************************************
 * Definitions
 *****************************************************************************/

/* special characters */
#define CR 		13u
#define LF 		10u
#define NL 		10u
#define BS 		8u
#define DEL 	127u
#define ESC 	27u
#define EOL		(LF)

/* used to time the VT100 escape sequences for e.g. cursor control */
#define ESC_TIMER_VALUE		20u	// milliseconds


/* special control characters used for command line editing */
/* value = (char & 0x31)                                    */
/* eg Ctrl W = ASCII 'w' (0x77) or 'W' (0x57) & 0x31        */
/*           = 0x17 or decimal 23                           */

#define CTRL_A	1u		/* jump to first character of command line */
#define CTRL_B  2u	    /* move cursor back one character */
#define CTRL_D  4u      /* deletes the character at the cursor */
#define CTRL_E  5u      /* jumps to the end of the current command */
#define CTRL_F  6u      /* moves the cursor forward one character */
#define CTRL_K  11u     /* deletes from cursor to end of command line */
#define CTRL_L  12u     /* repeates current line on a new line */
#define CTRL_N  14u     /* enters next line in history buffer */
#define CTRL_P  16u     /* enters previous line in command buffer */
#define CTRL_U  21u     /* deletes from cursor to beginning of command line*/
#define CTRL_W	23u	    /* deletes last word typed */

#define CTRL_R	18u	    /* same as CTRL_L */
#define CTRL_X  24u	    /* same as CTRL_U  */

/*****************************************************************************
 * Public Functions
 *****************************************************************************/
void cliInitialise(void);
void cliSetEcho(void);
void cliClrEcho(void);
bool cliGetEcho(void);
void edit_accept_line(void);
void add_char_to_line_buffer(char rx_char);
void cliScan(void);
void move_cursor_back_n (uint8_t len);

#endif /* CLI_H_ */
