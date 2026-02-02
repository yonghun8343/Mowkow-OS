#ifndef _NANO_H_
#define _NANO_H_

#define COLORS          8
#define COLOR_PAIRS     64
#define COLS            80
#define ESCDELAY        0
#define LINES           24
#define TABSIZE         4

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

typedef struct filestruct {
    char *data;
    struct filestruct *next;	/* Next node */
    struct filestruct *prev;	/* Previous node */
    long bytes;			/* # of Bytes before this line */
    int wrapline;		/* Is this line newly created by a wrap */
} filestruct;

#endif  /* _NANO_H_ */
