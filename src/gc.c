#include "internal.h"

#define MARK(ptr) (((struct block_header*)(ptr)-1)->marked)

void
mark(term_t t){
  if(IS_PTR(t))
    MARK(t) = 1;
}

void
gc(struct lisp0_state*state){
  mark(state->env);
  mark(state->x);
  mark(state->y);
  mark(state->z);

  term_t *sp = state->sp, *bp = state->bp;
  for(;bp;++sp){
    for(;sp<bp;++sp)
      mark(*sp);
    bp = (term_t*)*sp++;
  }

  for(struct list_builder *builder=state->builder;builder;builder=builder->next)
    for(struct list*list=(struct list*)(builder->result);list;list=(struct list*)list->tail)
      mark(list->head);

  struct gc_header **gch=&(state->gc_stack);

  for(struct gc_header*current=state->gc_stack;current;){
    if(MARK(current)){
      gch = &(current->next);
      MARK(current)=0;
      switch(PTAG(current)){
      case PTAG_LIST:
      case PTAG_LAMBDA:
      case PTAG_MACRO:
        mark(CAR(current));
        mark(CDR(current));
        break;
      default:
        ;
      };
      current = current -> next;
    }else{
      *gch = current->next;
      mem_free(&(state->pool),current);
      current = *gch;
    }
  }
}

void *
gc_alloc(struct lisp0_state*state,uint32_t n,uint8_t tag){
  if(!n)
    return NULL;

  void *ptr=mem_alloc(&(state->pool),n,tag,false);
  if(ptr)
    return ptr;

  gc(state);

  return mem_alloc(&(state->pool),n,tag,true);
}
