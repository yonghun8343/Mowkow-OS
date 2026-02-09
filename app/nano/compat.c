#include "compat.h"
#include "tip.h"
#include "proto.h"
#include "../include/apilib.h"
#include "../include/mylib.h"
#include <stdio.h>
#include <stdarg.h>


int current_win = -1;
int reverse_mode = 0;

extern void *topwin;
extern void *edit;
extern void *bottomwin;

extern int cur_x;
extern int cur_y;

extern char *winbuf_global;
extern int win_width_global;

int ctrl_pressed = 0;

int wgetch(void *win)
{
    int key;

    for (;;) {
        key = api_getkey(1);

        if (key == 0x1D) { // Ctrl Key Pressed
            ctrl_pressed = 1;
            continue;
        }
        if (key >= 'a' && key <= 'z') {
            if (ctrl_pressed) {
                // Ctrl + a(97) -> 1 (Control_A)
                // 공식: 키값 - 'a' + 1
                ctrl_pressed = 0; // Reset Ctrl State
                return key - 'a' + 1; 
            }
            return key;
        }

        if (key == 0x0A) {
            return 13;
        }

        if (key == 0x08) {
            return 127;
        }
        
        return key;
    }
}

void *nano_malloc(int size)
{
    int *p = (int *)api_malloc(size + 4);
    if (p == 0) return 0;

    *p = size;

    return (void *)(p + 1);
}

void nano_free(void *ptr) 
{
    if (ptr == 0) return;

    int *p = ((int *)ptr) - 1;

    int size = *p;
    api_free((char *)p, size + 4);
    return;
}

void *realloc(void *ptr, int size)
{
    if (ptr == 0) return nano_malloc(size);
    int *header = ((int *)ptr) - 1;
    int old_len = *header;
    void *new_ptr = nano_malloc(size);
    if (new_ptr == 0) return 0;

    char *src = (char *)ptr;
    char *dst = (char *)new_ptr;
    int copy_len = (old_len < size) ? old_len : size;

    int i;
    for (i=0; i<copy_len; i++) dst[i] = src[i];

    if (size > old_len) dst[copy_len] = 0; // Null Terminator
    
    nano_free(ptr);

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
    int len = my_strlen(str);

    int bg_col = (reverse_mode) ? 7 : 0;  
    int txt_col = (reverse_mode) ? 0 : 7;
    
    int rect_h = 16;
    int rect_w = len * 8;

    if (winbuf_global != 0) {
        int h, w;
        for (h = 0; h < rect_h; h++) {
            int offset = (py + h) * win_width_global + px;

            for (w = 0; w < rect_w; w++) {
                winbuf_global[offset + w] = (char)bg_col;
            }
        }
    }

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

void wrefresh_rows(int start_row, int end_row) 
{
    int top_margin = 28; 
    int line_height = 16;
    
    int y0 = top_margin + start_row * line_height;
    int y1 = top_margin + (end_row + 1) * line_height; 

    if (y0 < 0) y0 = 0;
    if (y1 > 450) y1 = 450; 

    api_refreshwin(current_win, 0, y0, 700, y1);
}

void wattron(void *win, int attr) {
    if (attr == 1) reverse_mode = 1;
    return;
}

void wattroff(void *win, int attr) {
    if (attr == 1) reverse_mode = 0;
    return;
}

void endwin(int win) {
    api_closewin(win);
}

void redrawin(int win) {
    api_refreshwin(current_win, 0, 0, 700, 450);
}

int my_strlen(char *str) {
    int len = 0;
    while (*str++) len++;
    return len;
}

int my_tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

char *my_strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

char *my_strncpy(char *dest, char *src, int n) {
    char *d = dest;
    int i;
    for (i = 0; i < n; i++) {
        if (src[i] == '\0') {
            break;
        }
        d[i] = src[i];
    }
    d[i] = '\0';
    return dest;
}

char *my_strstr(char *haystack, char *needle) {
    char *h, *n;

    if (*needle == 0) {
        return haystack;
    }

    for (; *haystack != 0; haystack++) {
        if (*haystack == *needle) {
            h = haystack;
            n = needle;
            while (*h != 0 && *n != 0 && *h == *n) {
                h++;
                n++;
            }

            if (*n == 0) {
                return haystack;
            }
        }
    }
    return 0;
}

int my_atoi(char *str) {
    int res = 0;
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}
