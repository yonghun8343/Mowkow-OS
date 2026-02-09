#ifndef _HAN_H_
#define _HAN_H_

// 에디터용 한글 상태 구조체
typedef struct {
    int state;      // 0:초기, 1:자음, 2:모음, 3:받침, 4:이중받침
    int cho;        // 초성 인덱스
    int jung;       // 중성 인덱스
    int jong;       // 종성 인덱스
    int composing;  // 1: 현재 조합 중인 글자가 화면에 떠 있음 (덮어써야 함)
} HANGUL_STATE;

void hangul_init(HANGUL_STATE *h);
void hangul_process_key(HANGUL_STATE *h, int key);
void commit_state(HANGUL_STATE *h);

#endif
