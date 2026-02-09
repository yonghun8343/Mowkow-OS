#include "mylib.h"
#include "apilib.h"
#include "apihan.h" 
#include <stdarg.h>



static struct HANGUL_STATE h_state;
static int lang_mode = 0; // 0:Eng, 1:Kor

char *gets(char *buf) {
    int i = 0;
    int key;

    /* 오토마타 초기화 */
    apihan_init(&h_state);

    while (1) {
        key = api_getkey(1);

        /* [Enter] 입력 종료 */
        if (key == 0x0A) {
            api_putstr("\n");
            break;
        }

        /* [Backspace] */
        if (key == 0x08) {
            apihan_backspace(&h_state, buf, &i);
            continue;
        }

        /* [Tab] 스페이스 4칸 */
        if (key == 0x09) {
            if (h_state.state > 0) {
                apihan_init(&h_state);
            }

            int k;
            for (k=0; k<4; k++) {
                if (i < 255) {
                    buf[i++] = ' ';
                    api_putstr(" ");
                }
            }
            continue;
        }
        
        /* [F1] 한영 전환 */
        if (key == 0x3B) {
            lang_mode ^= 1;
            // 전환 시 조합 중인 글자 처리가 애매하므로 리셋
            apihan_init(&h_state);
            continue;
        }

        /* 일반 입력 */
        if (lang_mode == 1) {
            apihan_run(&h_state, key, buf, &i);
        } else {
            // 영어 모드
            if (h_state.state > 0) apihan_init(&h_state); // 혹시 모를 잔여 상태 리셋
            
            buf[i++] = key;
            char s[2] = {key, 0};
            api_putstr(s);
        }
    }
    buf[i] = 0;
    return buf;
}

int isdigit(char c) { return (c >= '0' && c <= '9'); }
int putchar(int c) { char buf[2]={(char)c,0}; api_putstr(buf); return c; }
int puts(const char *s) { api_putstr((char *)s); api_putstr("\n"); return 0; }
void itoa(int value, char *str, int base) { 
    char *ptr = str; char *low; if (base < 2 || base > 36) { *str = '\0'; return; }
    char *rc = ptr; int sign = value; if (sign < 0 && base == 10) value = -value;
    do { *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[value % base]; value /= base; } while (value);
    if (sign < 0 && base == 10) *ptr++ = '-'; *ptr = '\0'; low = rc; char *high = ptr - 1;
    while (low < high) { char temp = *low; *low++ = *high; *high-- = temp; }
}
int printf(const char* format, ...) { 
    va_list ap; char buf[1024]; char temp[32]; char *p = buf; const char *f = format;
    va_start(ap, format);
    while (*f) {
        if (*f == '%') {
            f++;
            switch (*f) {
                case 'd': { int i = va_arg(ap, int); itoa(i, temp, 10); char *t = temp; while (*t) *p++ = *t++; break; }
                case 'x': { int i = va_arg(ap, int); itoa(i, temp, 16); char *t = temp; while (*t) *p++ = *t++; break; }
                case 's': { char *s = va_arg(ap, char*); if (!s) s = "(null)"; while (*s) *p++ = *s++; break; }
                case 'c': { int c = va_arg(ap, int); *p++ = (char)c; break; }
                default: { *p++ = '%'; *p++ = *f; break; }
            }
        } else { *p++ = *f; }
        f++; if (p - buf > 1000) break; 
    }
    *p = '\0'; api_putstr(buf); va_end(ap); return (p - buf);
}
void exit(int status) { api_end(); }
void *malloc(int size) { return (void *)api_malloc(size); }
