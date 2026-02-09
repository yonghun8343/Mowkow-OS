#include "tip.h"
#include "compat.h" 
#include "han.h"

extern filestruct *current; 
extern int current_y;       
extern int current_x;       

extern void insert_char_at_cursor(int key);  
extern void delete_char_at_cursor();

static int jong2cho[] = {
    -1, 0, 1, -1, 2, -1, -1, 3, 5, -1, -1, -1, -1, -1, -1, -1, 
    6, 7, -1, 9, 10, 11, 12, 14, 15, 16, 17, 18
};

static unsigned char U2J_cho[] = { 2,3,4,5,6, 7,8,9,10,11, 12,13,14,15,16, 17,18,19,20 };
static unsigned char U2J_jung[] = { 3,4,5,6,7, 10,11,12,13,14,15, 18,19,20,21,22,23, 26,27,28,29 };
static unsigned char U2J_jong[] = { 0, 2,3,4,5,6,7, 8,9,10,11,12,13,14,15,16, 17,19,20,21,22, 23,24,25,26,27,28,29 };

static char KeyToChoIdx[128] = {
    [0 ... 127] = -1,
    ['r']=0, ['R']=1, ['s']=2, ['e']=3, ['E']=4, ['f']=5, ['a']=6, ['q']=7, ['Q']=8, ['t']=9,
    ['T']=10, ['d']=11, ['w']=12, ['W']=13, ['c']=14, ['z']=15, ['x']=16, ['v']=17, ['g']=18
};
static char KeyToJungIdx[128] = {
    [0 ... 127] = -1,
    ['k']=0, ['o']=1, ['i']=2, ['O']=3, ['j']=4, ['p']=5, ['u']=6, ['P']=7,
    ['h']=8, ['y']=12, ['n']=13, ['b']=17, ['m']=18, ['l']=20
};
static char KeyToJongIdx[128] = {
    [0 ... 127] = -1,
    ['r']=1, ['R']=2, ['s']=4, ['e']=7, ['f']=8, ['a']=16, ['q']=17, ['t']=19,
    ['T']=20, ['d']=21, ['w']=22, ['c']=23, ['z']=24, ['x']=25, ['v']=26, ['g']=27
};

static int get_composite_jung(int j1, int j2) {
    if (j1==8) { if(j2==0) return 9; if(j2==1) return 10; if(j2==20) return 11; }
    if (j1==13) { if(j2==4) return 14; if(j2==5) return 15; if(j2==20) return 16; }
    if (j1==18 && j2==20) return 19;
    return -1;
}

static int split_composite_jung(int c) {
    if (c>=9 && c<=11) return 8;
    if (c>=14 && c<=16) return 13;
    if (c==19) return 18;
    return -1;
}

static int get_composite_jong(int cur, int next) {
    if (cur==1 && next==9) return 3; if (cur==4 && next==12) return 5; if (cur==4 && next==18) return 6;
    if (cur==8) {
        if(next==0)return 9; if(next==6)return 10; if(next==7)return 11; if(next==9)return 12;
        if(next==16)return 13; if(next==17)return 14; if(next==18)return 15;
    }
    if (cur==17 && next==9) return 18;
    return -1;
}

static int get_first_jong(int c) {
    if (c==3) return 1; if (c==5||c==6) return 4;
    if (c>=9 && c<=15) return 8; if (c==18) return 17;
    return -1;
}

static int get_second_jong(int c) {
    switch(c) {
        case 3: return 9; case 5: return 12; case 6: return 18; case 9: return 0;
        case 10: return 6; case 11: return 7; case 12: return 9; case 13: return 16;
        case 14: return 17; case 15: return 18; case 18: return 9;
    }
    return -1;
}

void insert_utf8_seq(int cho, int jung, int jong) {
    if (cho < 0 || cho > 18 || jung < 0 || jung > 20) return;
    int real_jong = (jong < 0) ? 0 : jong;
    unsigned int uni = 0xAC00 + (cho * 588) + (jung * 28) + real_jong;
    
    char u8[3];
    u8[0] = 0xE0 | ((uni >> 12) & 0x0F);
    u8[1] = 0x80 | ((uni >> 6) & 0x3F);
    u8[2] = 0x80 | (uni & 0x3F);
    
    // 에디터 데이터에 3바이트 삽입
    insert_char_at_cursor(u8[0]);
    insert_char_at_cursor(u8[1]);
    insert_char_at_cursor(u8[2]);
}

/* --- [오토마타 초기화] --- */
void hangul_init(HANGUL_STATE *h) {
    h->state = 0;
    h->cho = h->jung = h->jong = -1;
    h->composing = 0;
}

void commit_state(HANGUL_STATE *h) {
    if (h->composing) {
        h->composing = 0; 
        // 화면 갱신은 이미 되어있음
    }
    hangul_init(h);
}

static void update_visual(HANGUL_STATE *h) {
    // 1. 기존 조합 중인 글자가 있다면 지움 (덮어쓰기 위해 Backspace)
    if (h->composing) {
        delete_char_at_cursor(); // 종성(3byte) 삭제
        delete_char_at_cursor(); // 중성(3byte) 삭제
        delete_char_at_cursor(); // 초성(3byte) 삭제
    }

    // 2. 새 조합 글자 삽입
    if (h->state > 0 && h->jung != -1) {
        insert_utf8_seq(h->cho, h->jung, h->jong);
        h->composing = 1; // 조합 중 표시
    } else {
        // 초성만 있거나 상태가 없을 땐 표시 안함 (apihan.c 정책 따름)
        h->composing = 0;
    }

    // 3. 화면 갱신 요청
    update_line(current);
    wrefresh_rows(current_y, current_y);
}

void hangul_process_key(HANGUL_STATE *h, int key) {
    int c = KeyToChoIdx[key];
    int u = KeyToJungIdx[key];
    int o = KeyToJongIdx[key];

    // 1. 한글 자모가 아닌 경우 (숫자, 특수문자 등)
    if (c == -1 && u == -1) { 
        commit_state(h); 
        insert_char_at_cursor(key); 
        update_line(current);
        wrefresh_rows(current_y, current_y);
        return;
    }

    switch(h->state) {
        case 0: // 초기
            if (c != -1) { h->state=1; h->cho=c; }
            else if (u != -1) { 
                commit_state(h);
            }
            break;
            
        case 1: // 초성 (ㄱ)
            if (u != -1) { h->state=2; h->jung=u; } // ㄱ+ㅏ
            else if (c != -1) { 
                // ㄱ+ㄴ -> ㄱ 확정, ㄴ 시작
                commit_state(h);
                hangul_process_key(h, key); // [재귀 호출] 현재 키(ㄴ)로 다시 시작
                return; // 재귀에서 처리했으므로 리턴
            }
            break;

        case 2: // 중성 (가)
            if (o != -1) { h->state=3; h->jong=o; } // 가+ㄴ
            else if (u != -1) {
                int comp = get_composite_jung(h->jung, u);
                if (comp != -1) { h->jung = comp; } // ㅗ+ㅏ=ㅘ
                else {
                    // 가+ㅓ -> 가 확정, ㅓ 시작
                    commit_state(h);
                    hangul_process_key(h, key); // [재귀 호출]
                    return;
                }
            } else if (c != -1) {
                // 가+ㄱ (초성) -> 가 확정, ㄱ 시작
                commit_state(h);
                hangul_process_key(h, key); // [재귀 호출]
                return;
            }
            break;

        case 3: // 종성 (각)
            if (u != -1) { // 각+ㅏ -> 가, 가 (이건 로직이 복잡하여 재귀 안 씀)
                int prev_cho = h->cho;
                int prev_jung = h->jung;
                int prev_jong = h->jong;
                int next_cho = jong2cho[prev_jong];

                // 1. 앞글자(가) 확정
                h->jong = -1; // 종성 제거
                update_visual(h);
                commit_state(h); // '가'로 확정됨

                // 2. 뒷글자(가) 상태로 전이
                h->state = 2; 
                h->cho = next_cho; 
                h->jung = u; 
                h->jong = -1;
            } else if (c != -1) {
                int comp = get_composite_jong(h->jong, c);
                if (comp != -1) { h->state=4; h->jong=comp; } // 겹받침
                else {
                    // 각+ㄷ -> 각 확정, ㄷ 시작
                    commit_state(h);
                    hangul_process_key(h, key); // [재귀 호출]
                    return;
                }
            }
            break;
            
        case 4: // 겹받침 (닭)
            if (u != -1) { // 닭+ㅏ -> 달, 가
                int complex = h->jong;
                int j1 = get_first_jong(complex);
                int j2 = get_second_jong(complex);

                // 앞글자(달) 확정
                h->jong = j1;
                update_visual(h);
                commit_state(h);

                // 뒷글자(가) 상태
                h->state = 2;
                h->cho = j2;
                h->jung = u;
                h->jong = -1;
            } else if (c != -1) {
                // 닭+ㄱ -> 닭 확정, ㄱ 시작
                commit_state(h);
                hangul_process_key(h, key); // [재귀 호출]
                return;
            }
            break;
    }

    update_visual(h);
}
