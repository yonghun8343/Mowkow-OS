
/* eval.c */
#include "eval.h"
#include "parse.h"
#include "error.h"
#include <string.h>
#include <stdio.h>

Value* global_env;

Value* mkenv(Value* parent) {
    return cons(parent, NIL);
}

Value* envget(Value* env, Value* symbol) {
    Value* parent = car(env);
    Value* binds = cdr(env);
    while (!is_nil(binds)) {
        Value* entry = car(binds);
        if (strcmp(car(entry)->as.sval, symbol->as.sval) == 0) {
            return cdr(entry);
        }
        binds = cdr(binds);
    }
    if (is_nil(parent)) {
        raise_unbound_error(symbol->as.sval);
        return NIL;
    }
    return envget(parent, symbol);
}

void envset(Value* env, Value* symbol, Value* val) {
    Value* binds = cdr(env);
    while (!is_nil(binds)) {
        Value* entry = car(binds);
        if (strcmp(car(entry)->as.sval, symbol->as.sval) == 0) {
            setcdr(entry, val);
            return;
        }
        binds = cdr(binds);
    }
    Value* new_entry = cons(symbol, val);
    setcdr(env, cons(new_entry, cdr(env)));
}

/* Built-in Implementations */
Value* builtin_car(Value* args) { return car(car(args)); }
Value* builtin_cdr(Value* args) { return cdr(car(args)); }
Value* builtin_cons(Value* args) { return cons(car(args), car(cdr(args))); }
Value* builtin_add(Value* args) {
    Value *a = car(args), *b = car(cdr(args));
    return mkint(a->as.ival + b->as.ival);
}

Value* builtin_sub(Value* args) {
    Value *a = car(args), *b = car(cdr(args));
    return mkint(a->as.ival - b->as.ival);
}

Value* builtin_mul(Value* args) {
    Value *a = car(args), *b = car(cdr(args));
    return mkint(a->as.ival * b->as.ival);
}

Value* builtin_div(Value* args) {
    Value *a = car(args), *b = car(cdr(args));
    if (b->as.ival == 0) {
        raise_type_error("div");
        return NIL;
    }
    return mkint(a->as.ival / b->as.ival);
}

Value* builtin_inteq(Value* args) {
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error("=");
        return NIL;
    }

    Value* a = car(args);
    Value* b = car(cdr(args));

    if (!is_int(a) || !is_int(b)) {
        raise_type_error("=");
        return NIL;
    }

    if (a->as.ival == b->as.ival) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* builtin_intlt(Value* args) {
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error("<");
        return NIL;
    }
    
    Value* a = car(args);
    Value* b = car(cdr(args));

    if (!is_int(a) || !is_int(b)) {
        raise_type_error("<");
        return NIL;
    }

    if (a->as.ival < b->as.ival) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* builtin_intgt(Value* args) {
    /* 1. 인자 개수 확인: 정확히 2개여야 함 (isbinary) */
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error(">");
        return NIL;
    }

    Value* a = car(args);
    Value* b = car(cdr(args));

    /* 2. 타입 확인: 둘 다 정수여야 함 */
    if (!is_int(a) || !is_int(b)) {
        raise_type_error(">");
        return NIL;
    }

    /* 3. 비교 로직 */
    if (a->as.ival > b->as.ival) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* apply(Value* fn, Value* args) {
    if (fn->type == T_BUILTIN) return fn->as.fn(args);
    if (fn->type != T_CLOSURE) { raise_type_error("apply"); return NIL; }

    Value* env = mkenv(fn->as.clo.env);
    Value* params = fn->as.clo.params;
    Value* body = fn->as.clo.body;

    while (!is_nil(params)) {
        if (is_symbol(params)) { // Variadic
            envset(env, params, args);
            break;
        }
        envset(env, car(params), car(args));
        params = cdr(params);
        args = cdr(args);
    }

    Value* result = NIL;
    while (!is_nil(body)) {
        result = mk_eval(car(body), env);
        if (g_has_error) return NIL;
        body = cdr(body);
    }
    return result;
}

Value* builtin_apply(Value* args) {
    /* 1. 인자 개수 확인: 정확히 2개여야 함 (fn, list) */
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error("적용");
        return NIL;
    }

    Value* fn = car(args);
    Value* list_args = car(cdr(args));

    /* 2. 타입 확인: 두 번째 인자는 반드시 리스트여야 함 */
    if (!is_list(list_args)) {
        raise_syntax_error();
        return NIL;
    }

    /* 3. 함수 적용 호출 */
    return apply(fn, list_args);
}

/* [헬퍼 함수] 두 값의 동등성 재귀 비교 (builtin_eq용) */
int check_equality(Value* a, Value* b) {
    /* 1. 포인터가 같으면(같은 객체면) 참 */
    if (a == b) return 1;

    /* 2. 타입이 다르면 거짓 */
    if (a->type != b->type) return 0;

    /* 3. 타입별 값 비교 */
    switch (a->type) {
        case T_INTEGER:
            return a->as.ival == b->as.ival;
        case T_SYMBOL:
        case T_STRING:
            return strcmp(a->as.sval, b->as.sval) == 0;
        case T_NIL:
            return 1; /* 둘 다 NIL (위의 타입 체크 통과) */
        case T_BUILTIN:
            return a->as.fn == b->as.fn;
        case T_PAIR:
            /* 리스트는 재귀적으로 car와 cdr 비교 */
            return check_equality(car(a), car(b)) && check_equality(cdr(a), cdr(b));
        case T_CLOSURE:
        case T_MACRO:
            /* 클로저는 환경(포인터), 인자(리스트), 바디(리스트) 비교 */
            return (a->as.clo.env == b->as.clo.env) &&
                   check_equality(a->as.clo.params, b->as.clo.params) &&
                   check_equality(a->as.clo.body, b->as.clo.body);
        default:
            return 0;
    }
}

Value* builtin_eq(Value* args) {
    /* 1. 인자 개수 확인: 정확히 2개여야 함 (isbinary) */
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error("같다?");
        return NIL;
    }

    Value* a = car(args);
    Value* b = car(cdr(args));

    /* 2. 동등성 검사 수행 */
    if (check_equality(a, b)) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* builtin_ispair(Value* args) {
    /* 1. 인자 개수 확인: 정확히 1개여야 함 (isunary) */
    if (is_nil(args) || !is_nil(cdr(args))) {
        raise_args_error("짝?");
        return NIL;
    }

    /* 2. 대상 값 추출 */
    Value* target = car(args);

    /* 3. 타입 확인 (is_pair는 data.c에 정의됨) */
    if (is_pair(target)) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* builtin_isnil(Value* args) {
    /* 1. 인자 개수 확인: 정확히 1개여야 함 (isunary) */
    if (is_nil(args) || !is_nil(cdr(args))) {
        raise_args_error("공?");
        return NIL;
    }

    /* 2. 대상 값 추출 */
    Value* target = car(args);

    /* 3. NIL 여부 확인 (is_nil은 data.c에 정의됨) */
    if (is_nil(target)) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* builtin_not(Value* args) {
    /* 1. 인자 개수 확인: 정확히 1개여야 함 (isunary) */
    if (is_nil(args) || !is_nil(cdr(args))) {
        raise_args_error("부정");
        return NIL;
    }

    /* 2. 대상 값 추출 */
    Value* target = car(args);

    /* 3. 논리 부정: NIL이면 #참, 그 외(참)면 NIL 반환 */
    if (is_nil(target)) {
        return mksym("#참");
    } else {
        return NIL;
    }
}

Value* builtin_and(Value* args) {
    /* 1. 인자 개수 확인: 정확히 2개여야 함 (isbinary) */
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error("그리고");
        return NIL;
    }

    Value* a = car(args);
    Value* b = car(cdr(args));

    /* 2. 논리 처리 */
    /* 첫 번째 인자가 심볼 "#참"이면 두 번째 값 반환 */
    if (is_symbol(a) && strcmp(a->as.sval, "#참") == 0) {
        return b;
    } 
    /* 첫 번째 인자가 NIL(공/거짓)이면 NIL 반환 */
    else if (is_nil(a)) {
        return NIL;
    } 
    /* 그 외의 경우 타입 에러 (Strict Type Check) */
    else {
        raise_type_error("그리고");
        return NIL;
    }
}

Value* builtin_or(Value* args) {
    /* 1. 인자 개수 확인: 정확히 2개여야 함 (isbinary) */
    if (is_nil(args) || is_nil(cdr(args)) || !is_nil(cdr(cdr(args)))) {
        raise_args_error("또는");
        return NIL;
    }

    Value* a = car(args);
    Value* b = car(cdr(args));

    /* 2. 논리 처리 */
    /* 첫 번째 값이 NIL(거짓)이면 두 번째 값 반환 */
    if (is_nil(a)) {
        return b;
    } 
    /* 첫 번째 값이 참이면 첫 번째 값 반환 */
    else {
        return a;
    }
}


Value* mk_eval(Value* expr, Value* env) {
    if (g_has_error) return NIL;
    if (is_symbol(expr)) return envget(env, expr);
    if (!is_pair(expr)) return expr;
    // 순차 수식(Improper list) 체크가 필요하다면 여기서 is_list(expr) 호출

    Value* fun = car(expr);
    Value* args = cdr(expr);

    if (is_symbol(fun)) {
        char* fname = fun->as.sval;

        /* [인용] */
        if (strcmp(fname, "인용") == 0) return car(args);

        /* [정의] */
        if (strcmp(fname, "정의") == 0) {
            Value *sym = car(args);
            Value *val;
            if (is_pair(sym)) { // (정의 (f x) ...)
                 val = mkclosure(env, cdr(sym), cdr(args));
                 sym = car(sym);
            } else {
                 val = mk_eval(car(cdr(args)), env);
            }
            envset(env, sym, val);
            return NIL; // Defined symbol name
        }

        /* [만약] */
        if (strcmp(fname, "만약") == 0) {
            Value* cond = mk_eval(car(args), env);
            if (g_has_error) return NIL;
            if (is_nil(cond)) return mk_eval(car(cdr(cdr(args))), env);
            else return mk_eval(car(cdr(args)), env);
        }

        /* [람다] */
        if (strcmp(fname, "람다") == 0) {
            return mkclosure(env, car(args), cdr(args));
        }

        /* [조건] (cond) */
        if (strcmp(fname, "조건") == 0) {
            Value* clause_list = args;
            while (!is_nil(clause_list)) {
                Value* clause = car(clause_list);
                // 구문 체크: 절은 리스트여야 함
                if (!is_pair(clause)) { raise_syntax_error(); return NIL; }

                Value* cond_res = mk_eval(car(clause), env);
                if (g_has_error) return NIL;

                // 조건이 참(NIL이 아님)이면 해당 값 평가 후 반환
                if (!is_nil(cond_res)) {
                    return mk_eval(car(cdr(clause)), env);
                }
                clause_list = cdr(clause_list);
            }
            return NIL; // 모든 조건이 거짓이면 NIL
        }

        /* [매크로] (defmacro) */
        if (strcmp(fname, "매크로") == 0) {
            // (매크로 (이름 인자) 바디) 구조 확인
            if (is_nil(args) || is_nil(car(args))) { raise_args_error("매크로"); return NIL; }
            
            Value* header = car(args); // (이름 인자)
            if (!is_pair(header)) { raise_syntax_error(); return NIL; }

            Value* name = car(header);
            if (!is_symbol(name)) { raise_type_error("매크로"); return NIL; }

            // 매크로 객체 생성 및 환경 등록
            Value* macro = mkmacro(env, cdr(header), cdr(args));
            envset(env, name, macro);
            return NIL; // 반환값 없음 (혹은 name 반환)
        }

        /* [잠시] (let) - 추가됨 */
        if (strcmp(fname, "잠시") == 0) {
            // (잠시 ((이름 값) ...) 바디)
            if (is_nil(args) || is_nil(cdr(args))) { raise_args_error("잠시"); return NIL; }

            Value* binds = car(args);
            Value* body = car(cdr(args));
            Value* local_env = mkenv(env); // 새 스코프 생성

            while (!is_nil(binds)) {
                Value* pair = car(binds);
                if (!is_pair(pair)) { raise_syntax_error(); return NIL; }

                Value* sym = car(pair);
                if (!is_symbol(sym)) { raise_type_error("잠시"); return NIL; }

                // 값 평가 (파이썬 코드 로직에 따라 local_env 사용 -> let* 방식)
                Value* val_expr = car(cdr(pair));
                Value* val = mk_eval(val_expr, local_env); 
                if (g_has_error) return NIL;

                envset(local_env, sym, val);
                binds = cdr(binds);
            }
            // 바디 평가
            return mk_eval(body, local_env);
        }
    }

    /* 일반 함수 호출 처리 */
    Value* fn = mk_eval(fun, env);
    if (g_has_error) return NIL;
    
    if (fn->type == T_MACRO) {
        /* 매크로 내부 구조 추출 */
        Value* menv = fn->as.clo.env;
        Value* mparams = fn->as.clo.params;
        Value* mbody = fn->as.clo.body;
        
        /* 매크로 확장을 위한 임시 클로저 생성 (apply 함수 재사용을 위해) */
        Value* expander = mkclosure(menv, mparams, mbody);
        
        /* 매크로는 인자를 평가하지 않고(args 그대로) 적용하여 확장된 코드를 얻음 */
        Value* expansion = apply(expander, args);
        if (g_has_error) return NIL;
        
        /* 확장된 코드를 현재 환경에서 다시 평가 */
        return mk_eval(expansion, env);
    }

    // Evaluate arguments
    Value* ev_args = cplist(args);
    Value* p = ev_args;
    while (!is_nil(p)) {
        setcar(p, mk_eval(car(p), env));
        if (g_has_error) return NIL;
        p = cdr(p);
    }
    
    return apply(fn, ev_args);
}

void init_eval(void) {
    global_env = mkenv(NIL);
    
    /* 기존 등록 함수들 */
    envset(global_env, mksym("머"), mkbuiltin(builtin_car));
    envset(global_env, mksym("꼬"), mkbuiltin(builtin_cdr));
    envset(global_env, mksym("짝"), mkbuiltin(builtin_cons));
    envset(global_env, mksym("+"), mkbuiltin(builtin_add));
    envset(global_env, mksym("-"), mkbuiltin(builtin_sub));
    envset(global_env, mksym("*"), mkbuiltin(builtin_mul));
    envset(global_env, mksym("/"), mkbuiltin(builtin_div));
    envset(global_env, mksym("env"), mkbuiltin(builtin_print_env));
    envset(global_env, mksym("#참"), mksym("#참"));
    
    /* 논리/비교 함수 */
    envset(global_env, mksym("="), mkbuiltin(builtin_inteq));
    envset(global_env, mksym("<"), mkbuiltin(builtin_intlt));
    envset(global_env, mksym(">"), mkbuiltin(builtin_intgt));
    envset(global_env, mksym("같다?"), mkbuiltin(builtin_eq));
    envset(global_env, mksym("짝?"), mkbuiltin(builtin_ispair));
    envset(global_env, mksym("공?"), mkbuiltin(builtin_isnil));
    envset(global_env, mksym("부정"), mkbuiltin(builtin_not));
    envset(global_env, mksym("그리고"), mkbuiltin(builtin_and)); // 앞서 추가된 함수
    envset(global_env, mksym("또는"), mkbuiltin(builtin_or));    // 앞서 추가된 함수
    
    /* 기능 함수 */
    envset(global_env, mksym("적용"), mkbuiltin(builtin_apply));
}

/* [DEBUG] 환경 변수 전체 출력 함수 */
void print_env_scope(Value* env) {
    if (is_nil(global_env)) {
        printf("Environment is NIL.\n");
        return;
    }

    Value* binds = cdr(global_env); // (parent . binds) 구조에서 binds 가져오기
    int count = 0;

    printf("--- Current Scope Variables ---\n");
    
    while (!is_nil(binds)) {
        Value* entry = car(binds); // entry는 (key . value) 쌍
        
        if (is_pair(entry)) {
            Value* key = car(entry);
            Value* val = cdr(entry);

            printf("[%d] ", count++);
            
            // 키 출력
            if (is_symbol(key)) {
                printf("%s", key->as.sval);
            } else {
                print_value(key);
            }

            printf(" = ");

            // 값 출력 (값이 너무 길면 ... 처리 하거나 그냥 다 출력)
            print_value(val); 
            printf("\n");
        }
        
        binds = cdr(binds); // 다음 바인딩으로 이동
    }
    printf("-------------------------------\n");
}

/* 내장 함수로 등록하기 위한 래퍼 */
Value* builtin_print_env(Value* args) {
    // (환경) 호출 시 인자는 무시하고 현재 global_env를 출력하거나,
    // 클로저 내부 확인용으로 쓸 수 있습니다.
    // 여기서는 편의상 global_env를 출력합니다.
    print_env_scope(global_env);
    return NIL;
}
