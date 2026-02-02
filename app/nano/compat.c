#include "compat.h"
#include "nano.h"
#include "../include/apilib.h"


int current_win = -1;
int reverse_mode = 0;

void *topwin    = (void *)1;
void *edit      = (void *)2;
void *bottomwin = (void *)3;

extern int cur_x;
extern int cur_y;

void *realloc(void *ptr, int size)
{
    if (ptr == 0) return api_malloc(size);
    int old_len = 0;
    if (ptr) {
        char *p = (char *)ptr;
        while (*p) {
            p++;
        }
        old_len = p - (char *)ptr;
    }

    void *new_ptr = api_malloc(size);
    if (new_ptr != 0) {
        char *src = (char *)ptr;
        char *dst = (char *)new_ptr;
        int i;
        for (i=0; i<old_len && i<size; i++) {
            dst[i] = src[i];
        }
        for (; i<size; i++) {
            dst[i] = 0;
        }
    }

    api_free(ptr, old_len+1);
    return new_ptr;
}

void wmove(void *win, int y, int x) 
{
    int base_y = 0;

    if (win == (void *)1) {
        base_y = TOPWIN_Y_OFFSET;
    } else if (win == (void *)2) {
        base_y = EDIT_Y_OFFSET;
    } else if (win == (void *)3) {
        base_y = BOTTOMWIN_Y_OFFSET;
    }

    cur_y = base_y + y;
    cur_x = x;
}

void waddstr(void *win, char *str) 
{
    int px = cur_x * 8 + 8;
    int py = cur_y * 16 + 28;
    int len = 0;
    char *p = str;
    while(*p++) {
        len++;
    }

    int bg_col, txt_col;

    if (reverse_mode == 0) {
        bg_col = 0;
        txt_col = 7;
    } else {
        bg_col = 7;
        txt_col = 0;
    
    }
    api_boxfilwin(current_win, px, py, px + len*8, py + 16, bg_col);
    api_putstrwin(current_win, px, py, txt_col, len, str);

    cur_x += len;
}

void waddch(void *win, int c) {
    char s[2];
    s[0] = (char)c;
    s[1] = 0; // Null Terminator
    waddstr(win, s);
}

void wrefresh(void *win) 
{
    api_refreshwin(current_win, 0, 0, 700, 450);
}

int wgetch(void *win)
{
    int key;

    key = api_getkey(1);

    if (key == 0x0A) {
        return 13;
    }

    if (key == 0x08) {
        return 127;
    }

    return key;
}

void wattron(void *win, int attr) {
    if (attr == 1) reverse_mode = 1;
    return;
}

void wattroff(void *win, int attr) {
    if (attr == 1) reverse_mode = 0;
    return;
}
