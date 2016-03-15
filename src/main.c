#include "lisp0.h"

struct lisp0_state state = {
  .pool = {0}, .env = 0, .x = 0,. y = 0, .z = 0,
  .sp = NULL, .bp = NULL, .builder = NULL, .gc_stack = NULL,
};

char c_stack[2048];

int
lisp0_start(term_t *stack){
  state.sp = stack;
  mem_init(&(state.pool),(void*)(0x10000000));
  return lisp0_main(&state);
}

/* int */
/* main(){ */
/*   term_t stack[4096]; */
/*   return lisp0_start(stack+sizeof(stack)/sizeof(term_t)); */
/* } */
