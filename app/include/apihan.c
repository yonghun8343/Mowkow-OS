#include "apilib.h"
#include "apihan.h"

/* ============================================================
   [Tables] 커널 hangul.c의 테이블과 로직 이식
   ============================================================ */
static int jong2cho[] = {
    -1, 0, 1, -1, 2, -1, -1, 3, 5, -1, -1, -1, -1, -1, -1, -1, 
    6, 7, -1, 9, 10, 11, 12, 14, 15, 16, 17, 18
};

/* 유니코드 매핑 테이블 */
static unsigned char U2J_cho[] = { 2,3,4,5,6, 7,8,9,10,11, 12,13,14,15,16, 17,18,19,20 };
static unsigned char U2J_jung[] = { 3,4,5,6,7, 10,11,12,13,14,15, 18,19,20,21,22,23, 26,27,28,29 };
static unsigned char U2J_jong[] = { 0, 2,3,4,5,6,7, 8,9,10,11,12,13,14,15,16, 17,19,20,21,22, 23,24,25,26,27,28,29 };

/* 키보드 매핑 (런타임 초기화 없이 정적 배열 사용 권장) */
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

/* ============================================================
   [Helpers] 합성/분해 로직 (커널과 동일)
   ============================================================ */
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

/* ============================================================
   [Display & Buffer] 화면 출력 및 버퍼 관리
   ============================================================ */
void apihan_init(struct HANGUL_STATE *h) {
    h->state = 0;
    h->cho = h->jung = h->jong = -1;
    h->view_width = 0;
}

/* UTF-8 변환 (3바이트) */
static int compose_utf8(char *dest, int cho, int jung, int jong) {
    if (cho < 0 || cho > 18 || jung < 0 || jung > 20) return 0;
    int real_jong = (jong < 0) ? 0 : jong;
    unsigned int uni = 0xAC00 + (cho * 588) + (jung * 28) + real_jong;
    
    dest[0] = 0xE0 | ((uni >> 12) & 0x0F);
    dest[1] = 0x80 | ((uni >> 6) & 0x3F);
    dest[2] = 0x80 | (uni & 0x3F);
    dest[3] = 0;
    return 3;
}

/* 화면의 조합 중인 글자 지우기 */
static void clear_view(struct HANGUL_STATE *h) {
    int i;
    char bs[2] = {0x08, 0};
    /* view_width만큼 백스페이스 출력 */
    for(i=0; i<h->view_width; i++) {
        api_putstr(bs); 
    }
    h->view_width = 0;
}

/* 현재 상태를 화면에 그리기 (버퍼 반영 X) */
static void draw_view(struct HANGUL_STATE *h) {
    char buf[4] = {0};
    if (h->state == 0) return;
    
    clear_view(h); // 기존 것 지우기

    if (h->jung != -1) { // 중성까지 있으면 완성형(가) 형태로 출력
        compose_utf8(buf, h->cho, h->jung, h->jong);
        api_putstr(buf);
        h->view_width = 2; // 한글은 백스페이스 2번(16px) 필요
    } else { 
    }
}

/* 조합된 글자를 버퍼에 확정(Flush)하고 화면 뷰 초기화 */
static void flush_state(struct HANGUL_STATE *h, char *buf, int *pos) {
    char utf8[4];
    if (h->state == 0) return;

    clear_view(h); // 프리뷰 지우기

    if (h->jung != -1) {
        int len = compose_utf8(utf8, h->cho, h->jung, h->jong);
        int i;
        for(i=0; i<len; i++) buf[(*pos)++] = utf8[i];
        api_putstr(utf8); // 확정된 글자 출력
    } else {
        // 초성만 있다가 flush 되면? (예: 'ㄱ' 치고 엔터)
        // 해당 자음을 버퍼에 넣어야 함. 
        // 자모 매핑 테이블이 없으므로 여기서는 무시됨(증발).
        // 완벽한 구현을 위해선 호환 자모 변환이 필요.
    }
    
    // 상태 리셋
    h->state = 0;
    h->cho = h->jung = h->jong = -1;
    h->view_width = 0;
}

static void erase_visual(int count) {
    char eraser[] = {0x08, ' ', 0x08, 0}; 
    int i;
    for (i = 0; i < count; i++) {
        api_putstr(eraser);
    }
}

/* ============================================================
   [Core Logic] 오토마타 실행
   ============================================================ */
void apihan_run(struct HANGUL_STATE *h, int key, char *buf, int *pos) {
    if (key == 0) return;

    /* 1. 특수키 처리 */
    if (key >= 128) { // Function keys etc.
        flush_state(h, buf, pos);
        // 여기서 버퍼에 넣을지 말지 결정 필요. 보통 무시.
        return;
    }

    /* 2. 한글 자모 인덱스 변환 */
    int c = KeyToChoIdx[key];
    int u = KeyToJungIdx[key];
    int o = KeyToJongIdx[key];

    /* 3. 한글 아님 (숫자, 영문 등) */
    if (c == -1 && u == -1) {
        flush_state(h, buf, pos);
        buf[(*pos)++] = key;
        char s[2] = {key, 0};
        api_putstr(s);
        return;
    }

    /* 4. 상태 전이 로직 (hangul.c 기반) */
    // 기존 프리뷰 지우기
    clear_view(h);

    switch(h->state) {
        case 0: // 초기
            if (c != -1) { h->state=1; h->cho=c; }
            else if (u != -1) { 
                // 모음 단독: flush 후 출력
                flush_state(h, buf, pos); // 빈 상태 flush (의미 없음)
                // 모음 단독은 완성형으로 만들 수 없으므로 그냥 둠(증발) 혹은 호환자모 처리 필요
            }
            break;
        case 1: // 초성
            if (u != -1) { h->state=2; h->jung=u; } // ㄱ+ㅏ=가
            else if (c != -1) { 
                h->state=1; h->cho=c; 
            }
            break;
        case 2: // 중성
            if (o != -1) { h->state=3; h->jong=o; } // 가+ㄴ=간
            else if (u != -1) {
                int comp = get_composite_jung(h->jung, u);
                if (comp != -1) { h->jung = comp; } // ㅗ+ㅏ=ㅘ
                else {
                    char utf8[4];
                    int len = compose_utf8(utf8, h->cho, h->jung, h->jong);
                    int i;
                    for(i=0; i<len; i++) buf[(*pos)++] = utf8[i];
                    api_putstr(utf8); // 화면에 확정 출력

                    h->state=0; h->cho=-1; h->jung=-1; h->jong=-1; h->view_width=0;
                }
            } else if (c != -1) {
                // 가+ㄱ (종성 아님, 초성으로 옴) -> '가' flush, 'ㄱ' 시작
                char utf8[4];
                compose_utf8(utf8, h->cho, h->jung, h->jong);
                int i; for(i=0; i<3; i++) buf[(*pos)++] = utf8[i];
                api_putstr(utf8);
                
                h->state=1; h->cho=c; h->jung=-1; h->jong=-1; h->view_width=0;
            }
            break;
        case 3: // 종성
            if (u != -1) { // 각+ㅏ -> 가, 가
                int prev_cho = h->cho;
                int prev_jung = h->jung;
                int prev_jong = h->jong;
                int next_cho = jong2cho[prev_jong];

                // 앞 글자(가) 확정
                char utf8[4];
                compose_utf8(utf8, prev_cho, prev_jung, 0); // 종성 뺌
                int i; for(i=0; i<3; i++) buf[(*pos)++] = utf8[i];
                api_putstr(utf8);

                // 뒷 글자(가) 시작
                h->state=2; h->cho=next_cho; h->jung=u; h->jong=-1; h->view_width=0;
            } else if (c != -1) {
                int comp = get_composite_jong(h->jong, c);
                if (comp != -1) { h->state=4; h->jong=comp; } // 겹받침
                else {
                    // 각+ㄷ -> '각' flush, 'ㄷ' 시작
                    char utf8[4];
                    compose_utf8(utf8, h->cho, h->jung, h->jong);
                    int i; for(i=0; i<3; i++) buf[(*pos)++] = utf8[i];
                    api_putstr(utf8);
                    
                    h->state=1; h->cho=c; h->jung=-1; h->jong=-1; h->view_width=0;
                }
            }
            break;
        case 4: // 겹받침
            if (u != -1) { // 닭+ㅏ -> 달, 가
                int complex = h->jong;
                int j1 = get_first_jong(complex);
                int j2 = get_second_jong(complex); // 초성이 될 녀석

                // 앞 글자(달) 확정
                char utf8[4];
                compose_utf8(utf8, h->cho, h->jung, j1);
                int i; for(i=0; i<3; i++) buf[(*pos)++] = utf8[i];
                api_putstr(utf8);
                
                // 뒷 글자(가) 시작
                h->state=2; h->cho=j2; h->jung=u; h->jong=-1; h->view_width=0;
            } else if (c != -1) {
                // 닭+ㄱ -> '닭' flush, 'ㄱ' 시작
                char utf8[4];
                compose_utf8(utf8, h->cho, h->jung, h->jong);
                int i; for(i=0; i<3; i++) buf[(*pos)++] = utf8[i];
                api_putstr(utf8);
                
                h->state=1; h->cho=c; h->jung=-1; h->jong=-1; h->view_width=0;
            }
            break;
    }

    // 새 상태 그리기 (프리뷰)
    if (h->state > 0 && h->jung != -1) {
        char utf8[4];
        compose_utf8(utf8, h->cho, h->jung, h->jong);
        api_putstr(utf8);
        h->view_width = 2;
    } else if (h->state == 1) {
        // 초성만 있는 경우 시각적 피드백 없음 (임시)
        h->view_width = 0;
    }
}

/* 백스페이스 처리 */
void apihan_backspace(struct HANGUL_STATE *h, char *buf, int *pos) {
    // 1. 조합 중인 한글이 있는 경우 (State Regression)
    if (h->state > 0) {
        // 프리뷰 지우기 (단순 커서 이동이 아니라 공백으로 덮어씀)
        if (h->view_width > 0) erase_visual(h->view_width);

        // 상태 되돌리기 (기존 로직 유지)
        switch(h->state) {
            case 1: h->state=0; h->cho=-1; break;
            case 2: 
                {
                    int pj = split_composite_jung(h->jung);
                    if(pj != -1) h->jung = pj;
                    else { h->state=1; h->jung=-1; }
                }
                break;
            case 3: h->state=2; h->jong=-1; break;
            case 4: h->jong = get_first_jong(h->jong); h->state=3; break;
        }

        // 되돌린 상태 다시 그리기
        if (h->state > 0 && h->jung != -1) {
            char utf8[4];
            compose_utf8(utf8, h->cho, h->jung, h->jong);
            api_putstr(utf8);
            h->view_width = 2;
        } else {
            h->view_width = 0;
        }
    } 
    // 2. 조합 중이 아님 -> 완성된 글자 버퍼에서 삭제
    else if (*pos > 0) {
        // 버퍼 마지막 바이트 확인
        unsigned char last = (unsigned char)buf[*pos - 1];
        
        if (last < 0x80) { 
            // ASCII (1바이트, 1칸)
            (*pos)--;
            erase_visual(1);
        } else {
            // UTF-8 한글 (3바이트, 2칸 가정)
            // 안전하게 3바이트 확인
            if (*pos >= 3) {
                (*pos) -= 3;
                erase_visual(2); // 한글은 2칸 지움
            }
        }
    }
}
