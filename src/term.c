#include "internal.h"

term_t
atom(struct lisp0_state*state,uint32_t n,const char*s){
  static const uint8_t code[]={
    1, 2, 3, 4, 5, 6, 7, 8, 9,10, 0, 0, 0, 0, 0, 0,
    0,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
    26,27,28,29,30,31,32,33,34,35,36, 0, 0, 0, 0,63,
    0,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    52,53,54,55,56,57,58,59,60,61,62, 0, 0, 0, 0, 0
  };

  if(n<=8){
    uint8_t t[8]={0};
    for(uint32_t i=0;i<n;++i)
      t[i]=code[s[i]-'0'];

    union{
      uint8_t s[8];
      term_t t;
    }u;

    u.s[0] = (n<<4)|(TAG_ATOM);
    u.s[1] = (t[0]<<2)|(t[1]>>4);
    u.s[2] = (t[1]<<4)|(t[2]>>2);
    u.s[3] = (t[2]<<6)|(t[3]);
    u.s[4] = (t[4]<<2)|(t[5]>>4);
    u.s[5] = (t[5]<<4)|(t[6]>>2);
    u.s[6] = (t[6]<<6)|(t[7]);
    return u.t;
  }

  struct atom*t=gc_alloc(state,sizeof(struct atom)+n+1,PTAG_ATOM);

  t->size=n;
  t->gch.next = state->gc_stack;
  state->gc_stack = &(t->gch);

  for(uint32_t i=0;i<n;++i)
    t->s[i]=s[i];

  t->s[n]=0;
  return (term_t)t;
}


term_t
mklist(struct lisp0_state*state,term_t head,term_t tail,uintptr_t tag){
  struct list*list=gc_alloc(state,sizeof(struct list),tag);
  list->head=head;
  list->tail=tail;

  list->gch.next = state->gc_stack;
  state->gc_stack = &(list->gch);

  return (term_t)list;
}


term_t
cons(struct lisp0_state*state,term_t head,term_t tail){
  return mklist(state,head,tail,PTAG_LIST);
}


void
push_list_builder(struct lisp0_state*state){
  struct list_builder *new_builder=(struct list_builder*)gc_alloc(state,sizeof(struct list_builder),PTAG_LIST_BUILDER);
  new_builder->next=state->builder;
  state->builder=new_builder;
  state->builder->result=NIL;
  state->builder->last=&(state->builder->result);
  state->builder->gch.next=&(state->builder->gch);
}


term_t
pop_list_builder(struct lisp0_state*state,term_t last){
  struct list_builder*builder = state->builder;

  if(builder->last!=&(builder->result)){
    builder->gch.next->next = state->gc_stack;
    state->gc_stack = (struct gc_header*)(builder->result);
  }

  *(builder->last)=last;
  term_t result = builder->result;

  state->builder=builder->next;
  mem_free(&(state->pool),builder);
  return result;
}


void
list_builder_add_term(struct lisp0_state*state,term_t term){
  state->z = term;
  struct list*list=gc_alloc(state,sizeof(struct list),PTAG_LIST);
  CLEAR(state->z);

  list->head=term;
  list->tail=NIL;
  list->gch.next=NULL;

  *(state->builder->last)=(term_t)list;
  state->builder->gch.next->next = &(list->gch);

  state->builder->last=&(list->tail);
  state->builder->gch.next = &(list->gch);
}
