#ifndef _COMPAT_H_
#define _COMPAT_H_

#include "nano.h"

#define mvwaddstr(win, y, x, str) { wmove(win, y, x); waddstr(win, str); }
#define mvwaddch(win, y, x, ch)   { wmove(win, y, x); waddch(win, ch); }

#define TOPWIN_Y_OFFSET    0
#define EDIT_Y_OFFSET      2
#define BOTTOMWIN_Y_OFFSET (LINES - 3)
#define A_REVERSE         1

extern void *topwin;
extern void *edit;
extern void *bottomwin;
extern int current_x;
extern int current_y;
extern int editwineob;
extern int editwinrows;
extern int current_win;
extern int reverse_mode;
extern filestruct *fileage;
extern filestruct *filebot;
extern filestruct *current;
extern filestruct *cutbuffer;

void *realloc(void *ptr, int size);
void wmove(void *win, int y, int x);
void waddstr(void *win, char *str);
void wrefresh(void *win);
int wgetch(void *win);
void wattron(void *win, int attr);
void wattroff(void *win, int attr);

#endif  /* _COMPAT_H_ */
