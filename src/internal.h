#pragma once
#include <unistd.h>
#include <syscall.h>
#include "lisp0.h"

struct list{
  struct gc_header gch;
  term_t head,tail;
};

struct atom{
  struct gc_header gch;
  uint32_t size;
  char s[];
};

enum error{
  E_NO_VALUE,
  E_ATOM_TOO_LONG,
  E_RIGHT_PAREN,
  E_INVALID_CHAR,
  E_UNBOUND,
  E_IMPROPER_LIST,
  E_ARGUMENT,
  E_BAD_PRIMITIVE,
  E_COND_END,
  E_NOT_CALLABLE,
  E_BAD_EXPR,
};

enum tag{
  TAG_POINTER = 0,
  TAG_PRIMITIVE,
  TAG_ATOM,
  TAG_ERROR = 7,
};

enum ptr_tag{
  PTAG_LIST_BUILDER,
  PTAG_ATOM,
  PTAG_LIST,
  PTAG_LAMBDA,
  PTAG_MACRO,
};

enum prim{
  PRIM_QUOTE,
  PRIM_ATOM,
  PRIM_EQ,
  PRIM_COND,
  PRIM_CAR,
  PRIM_CDR,
  PRIM_CONS,
  PRIM_LABEL,
  PRIM_LAMBDA,
  PRIM_MACRO,
};


#define TAG(x) ((x)&0b111)
#define VALUE(x) ((x)>>3)
#define MKVAL(t,v) (((v)<<3)|t)

#define PTAG(x) (mem_tag((void*)(x)))
#define LST(x) ((struct list*)(x))
#define CAR(x) (LST(x)->head)
#define CDR(x) (LST(x)->tail)

#define ERR(x) (MKVAL(TAG_ERROR,(x)))
#define PRIM(x) (MKVAL(TAG_PRIMITIVE,(PRIM_##x)))

#define IS_ERROR(x) ((TAG(x)==TAG_ERROR))
#define IS_PTR(x) ((x)&&(TAG(x)==TAG_POINTER))
#define IS_ATOM(x) ((TAG(x)==TAG_ATOM)||(IS_PTR(x)&&(PTAG(x)==PTAG_ATOM)))
#define IS_LIST(x) (IS_PTR(x)&&(PTAG(x)==PTAG_LIST))

#define NIL (0)
#define TRUE (TAG_ATOM|0x696EE340ULL)
#define FALSE (TAG_ATOM|0xA4375CAA50ULL)


#define X (state->x)
#define Y (state->y)
#define ENV (state->env)
#define PUSH(value) (*(--(state->sp))=(term_t)(value));
#define POP() (*(state->sp++))
#define CLEAR(x) (x) = ERR(E_NO_VALUE);

#define RETURN(value)            \
  X = (term_t)(value);           \
  CLEAR(Y);                      \
  state->sp = state->bp;         \
  state->bp = (term_t*)POP();    \
  goto *(void*)POP();

#define PREPARE_CALL(l_ret)     \
  PUSH(&&l_ret);                \
  PUSH(state->bp);              \
  state->bp = state->sp;

#define _LABEL(cnt) LL_##cnt

#define _CALL(l_fun,cnt)       \
  PREPARE_CALL(_LABEL(cnt));   \
  goto l_fun;                  \
_LABEL(cnt):

#define CALL(l_fun) _CALL(l_fun,__COUNTER__)

#define RETURN_ERROR(x)      \
  if(IS_ERROR(x)){           \
    RETURN(x);               \
  }

#define PARSE_ARG()          \
  RETURN_ERROR(Y);           \
  if(!IS_LIST(Y)){           \
    RETURN(ERR(E_ARGUMENT)); \
  }

#define PARSE_ARG1()         \
  PARSE_ARG();               \
  if(CDR(Y)){                \
    RETURN(ERR(E_ARGUMENT)); \
  }                          \
  X = CAR(Y);

#define PARSE_ARG2()         \
  PARSE_ARG();               \
  X = CAR(Y);                \
  Y = CDR(Y);                \
  PARSE_ARG();               \
  if(CDR(Y)){                \
    RETURN(ERR(E_ARGUMENT)); \
  }                          \
  Y = CAR(Y);

#define PARSE_AND_EVAL_ARG1() \
  PARSE_ARG1();               \
  CLEAR(Y);                   \
  CALL(L_eval);               \
  RETURN_ERROR(X);

#define PARSE_AND_EVAL_ARG2()  \
  PARSE_ARG2();                \
  PUSH(Y);                     \
  CLEAR(Y);                    \
  CALL(L_eval);                \
  RETURN_ERROR(X);             \
  Y = POP();                   \
  PUSH(X);                     \
  X = Y;                       \
  CLEAR(Y);                    \
  CALL(L_eval);                \
  RETURN_ERROR(X);             \
  Y = POP();


void *
gc_alloc(struct lisp0_state*,uint32_t n,uint8_t tag);

term_t
atom(struct lisp0_state*,uint32_t n,const char*s);

term_t
mklist(struct lisp0_state*state,term_t head,term_t tail,uintptr_t tag);

term_t
cons(struct lisp0_state*state,term_t head,term_t tail);

void
push_list_builder(struct lisp0_state*state);

term_t
pop_list_builder(struct lisp0_state*state,term_t last);

void
list_builder_add_term(struct lisp0_state*state,term_t term);

term_t
read_term(struct lisp0_state*state);

void
init_env(struct lisp0_state*state);

term_t
eval_term(struct lisp0_state*state,term_t term);

void
print_term(struct lisp0_state*state,term_t term);
