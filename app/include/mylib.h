/* mylib.h */
#ifndef MYLIB_H
#define MYLIB_H

/* Input Functions */
char *gets(char *buf);

/* Character & String Utilities */
int isdigit(char c);
void itoa(int value, char *str, int base);

/* Output Functions */
int putchar(int c);
int puts(const char *s);
int printf(const char* format, ...);

/* System & Memory Functions */
void exit(int status);
void *malloc(int size);

#endif
