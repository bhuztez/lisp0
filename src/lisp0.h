#pragma once
#include "tlsf.h"

typedef uintptr_t term_t;

struct gc_header{
  struct gc_header *next;
};

struct list_builder{
  struct list_builder*next;
  term_t result;
  term_t*last;
  struct gc_header gch;
};

struct lisp0_state{
  struct memory_pool pool;
  term_t env,x,y,z;
  term_t *sp,*bp;
  struct list_builder *builder;
  struct gc_header *gc_stack;
};

int
lisp0_main(struct lisp0_state*state);
