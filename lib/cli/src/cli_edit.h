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

#ifndef CLI_EDIT_H_
#define CLI_EDIT_H_

extern void move_cursor_back_n (uint8_t len);
extern void clear_to_eol (void);
extern void delete_string ( uint8_t len);

extern void edit_accept_line (void);
extern void edit_del_char (void);
extern void edit_del_eol (void);
extern void edit_bk_del_char (void);
extern void edit_del_word (void);
extern void edit_del_beg (void);
extern void edit_beg_line (void);
extern void edit_bk_char (void);
extern void edit_end_line (void);
extern void edit_fd_char (void);
extern void edit_redisplay (void);
extern void edit_h_next (void);
extern void edit_h_prev (void);


#endif /* CLI_EDIT_H_ */
