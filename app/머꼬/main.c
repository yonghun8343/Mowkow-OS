#include "data.h"
#include "parse.h"
#include "eval.h"
#include "error.h"
#include <stdio.h>
#include "../include/apilib.h" 

void init_all() {
    init_mowkow(); // 데이터 풀 초기화
    init_reader(); // 리더 초기화
    init_eval();   // 환경 및 내장함수 초기화
}

void load_file(Value* env, char* path) {
    int fh = api_fopen(path, 0);
    if (fh == 0) {
        printf("[Error] 파일 '%s'을(를) 열 수 없습니다.\n", path);
        return;
    }

    int filesize = api_fsize(fh, 0);
    if (filesize >= BUFFER_SIZE - 1) {
        printf("[Error] 파일 '%s'이(가) 너무 큽니다.\n", path);
        api_fclose(fh);
        return;
    }

    api_fread(YY_reader.buf, filesize, fh);
    YY_reader.buf[filesize] = '\0'; // 널 문자 종료
    api_fclose(fh);

    YY_reader.pos = 0;
    YY_reader.depth = 0;

    next_token();

    while (1) {
        /* 에러 상태면 중단 */
        if (g_has_error) {
            printf("오류 발생으로 로딩을 중단합니다.\n");
            reset_error();
            break;
        }

        /* EOF 확인 (토큰이 없으면 종료) */
        /* parse.c 로직상 depth가 0일 때 버퍼 끝에 도달하면 "" 토큰을 반환하고 종료함 */
        if (get_current_token()[0] == '\0') {
            break;
        }

        /* 수식 읽기 */
        Value* expr = read_expr();
        
        /* 파싱 중 에러 체크 */
        if (g_has_error) break;

        /* 실행 (Eval) */
        if (expr != NIL) {
            Value* res = mk_eval(expr, env);
            
            /* 실행 중 에러 체크 */
            if (g_has_error) break;

            /* 결과 출력 (파이썬 코드와 동일하게 NIL이 아니면 출력) */
            /* 라이브러리 로딩 시 출력이 싫다면 이 부분을 주석 처리 */
            if (!is_nil(res)) {
                print_value(res);
                printf("\n");
            }
        }
    }

    /* 6. REPL 복귀를 위한 버퍼 초기화 */
    YY_reader.buf[0] = '\0';
    YY_reader.pos = 0;
}

void run_repl() {
    printf("머꼬 C 포트 v1.0\n");
    while (1) {
        reset_error();
        
        // 토큰이 없으면 채우기
        if (get_current_token()[0] == '\0') {
            reader_fill_buffer();
            next_token();
        }
        if (get_current_token()[0] == '\0') continue; // Empty line

        Value* expr = read_expr();
        if (g_has_error) {
            YY_reader.pos = 0; YY_reader.buf[0] = '\0'; YY_reader.token[0] = '\0';
            continue;
        }

        if (expr != NIL) {
            Value* res = mk_eval(expr, global_env);
            if (!g_has_error && res) {
                print_value(res);
                printf("\n");
            }
        }
    }
}

void HariMain(void) {
    init_all();
    load_file(global_env, "library.scm");
    run_repl();
}
