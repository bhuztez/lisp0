#include <stdio.h>
#include "internal.h"

int
read_char(){
  static uint8_t buf[128];
  static uint32_t begin=0,end=0;

  if(begin>=end){
    ssize_t l = -1;
    for(;l<0;)
      l = syscall(SYS_read,0,buf+end,sizeof(buf)-end);
    if(l==0)
      return EOF;
    end+=l;
  }

  int result = buf[begin++];
  if(begin==sizeof(buf)){
    begin=0;
    end=0;
  }

  return result;
}


term_t
parse(struct lisp0_state*state,int c){
  static size_t index = 0;
  static char buf[16] = {0};

  switch(c){
  case '_':
  case 'A' ... 'Z':
  case 'a' ... 'z':
  case '0' ... '9':
    if(index >= sizeof(buf))
      return ERR(E_ATOM_TOO_LONG);

    buf[index]=c;
    ++(index);
    return ERR(E_NO_VALUE);
  default:
    ;
  };

  if(state->builder){
    if(index)
      list_builder_add_term(state,atom(state,index,buf));
    index=0;

    switch(c){
    case '(':
      push_list_builder(state);
      return ERR(E_NO_VALUE);
    case ')':{
      term_t result = pop_list_builder(state,NIL);
      if(!(state->builder))
        return result;
      list_builder_add_term(state,result);
      return ERR(E_NO_VALUE);
    };
    case EOF: case ' ': case '\r': case '\n': case '\f': case '\t': case '\v':
      return ERR(E_NO_VALUE);
    default:
      ;
    };
  }else{
    term_t result=(index)?atom(state,index,buf):ERR(E_NO_VALUE);
    index=0;
    switch(c){
    case '(':
      push_list_builder(state);
      return result;
    case EOF: case ' ': case '\r': case '\n': case '\f': case '\t': case '\v':
      return result;
    case ')':
      return ERR(E_RIGHT_PAREN);
    default:
      ;
    };
  }
  return ERR(E_INVALID_CHAR);
}


term_t
read_term(struct lisp0_state*state){
  for(;;){
    int ch=read_char();
    term_t term=parse(state,ch);

    if(!IS_ERROR(term))
      return term;

    if((VALUE(term)!=E_NO_VALUE))
      return term;

    if(ch==EOF)
      return term;
  }
}
