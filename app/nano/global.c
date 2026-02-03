/**************************************************************************
 *   global.c                                                             *
 *                                                                        *
 *   Copyright (C) 1999 Chris Allegretta                                  *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 1, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 *   GNU General Public License for more details.                         *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.            *
 *                                                                        *
 **************************************************************************/

#include "nano.h"

int cur_x = 0, cur_y = 0;
int center_x = 0, center_y = 0;		/* Center of screen */
void *edit;				/* The file portion of the editor  */
void *topwin;				/* Top line of screen */
void *bottomwin;			/* Bottom buffer */
int file;				/* Actual file pointer */
char filename[132] = "";		/* Name of the file */
int modified = 0;			/* Has file been modified? */
int editwinrows = 0;			/* How many rows long is the edit
					   window? */
int editwineob = 0;			/* Last Line in edit buffer
					   (0 - editwineob) */
filestruct *current;			/* Current buffer pointer */
int current_x = 0, current_y = 0;	/* Current position of X and Y in
					   the editor - relative to edit
					   window (0,0) */
int posible_max = 0;			/* The X value we'd like to be able to get to on a line */
filestruct *fileage = 0;		/* Our file buffer */
filestruct *edittop = 0;		/* Pointer to the top of the edit
					   buffer with respect to the
					   file struct */

filestruct *editbot = 0;		/* Same for the bottom */
filestruct *filebot = 0;		/* Last node in the file struct */
filestruct *cutbuffer = 0;		/* A place to store cut text */
filestruct *cutbottom = 0;		/* Pointer to end of cutbuffer */
int keep_cutbuffer = 0;			/* Clear out the cutbuffer? */

char answer[132] = "";			/* Answer str to many questions */
char last_search[132] = "";		/* Last string we searched for */
char last_replace[132] = "";		/* Last replacement string */
int totlines = 0;			/* Total number of lines in the
					   file */
int suspend = 0;			/* Can TIP be suspended */
int case_sensitive = 0;			/* Are we doing case sensitive
					   searches */
int placewewant = 0;			/* The collum we'd like the cursor
					    to jump to when we go to the
					    next or previous line */

int statblank = 0;			/* Number of keystrokes left after
					   we call statubar() before we
					   actually blank the statusbar */

shortcut main_list[MAIN_LIST_LEN] = 
{
{TIP_GOTO_KEY, "Goto Line"},
{TIP_EXIT_KEY, "Exit"},
{TIP_WRITEOUT_KEY, "Write Out"},
{TIP_INSERTFILE_KEY, "Read File"},
{TIP_REPLACE_KEY, "Replace"},
{TIP_WHEREIS_KEY, "Where Is"},
{TIP_PREVPAGE_KEY, "Prev Page"},
{TIP_NEXTPAGE_KEY, "Next Page"},
{TIP_CUT_KEY, "Cut Text"},
{TIP_UNCUT_KEY, "Uncut Txt"},
{TIP_CURSORPOS_KEY, "Cur Pos"},
{TIP_SPELLING_KEY, "To Spell"}
};

shortcut whereis_list[WHEREIS_LIST_LEN] = 
{
{TIP_CASE_KEY, "Case Sens"},
{TIP_CANCEL_KEY, "Cancel"},
{TIP_FIRSTLINE_KEY, "First Line"},
{TIP_LASTLINE_KEY, "Last Line"},
};

shortcut replace_list[REPLACE_LIST_LEN] = 
{
{TIP_CASE_KEY, "Case Sens"},
{TIP_CANCEL_KEY, "Cancel"},
{TIP_FIRSTLINE_KEY, "First Line"},
{TIP_LASTLINE_KEY, "Last Line"},
};

shortcut goto_list[GOTO_LIST_LEN] = 
{
{TIP_FIRSTLINE_KEY, "First Line"},
{TIP_LASTLINE_KEY, "Last Line"},
{TIP_CANCEL_KEY, "Cancel"},
};

shortcut writefile_list[WRITEFILE_LIST_LEN] = 
{
{TIP_CANCEL_KEY, "Cancel"},
};
