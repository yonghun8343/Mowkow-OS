#include "compat.h"
#include "nano.h"
#include "proto.h"
#include "../include/apilib.h"
#include <stdio.h>
#include <stdarg.h>


int current_win = -1;
int reverse_mode = 0;

extern void *topwin;
extern void *edit;
extern void *bottomwin;

extern int cur_x;
extern int cur_y;

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
        if (key == 0x9D) { // Ctrl Key Released
            ctrl_pressed = 0;
            continue;
        }
        if (key >= 'a' && key <= 'z') {
            if (ctrl_pressed) {
                // Ctrl + a(97) -> 1 (Control_A)
                // 공식: 키값 - 'a' + 1
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

void wattron(void *win, int attr) {
    if (attr == 1) reverse_mode = 1;
    return;
}

void wattroff(void *win, int attr) {
    if (attr == 1) reverse_mode = 0;
    return;
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
