#include "internal.h"

void
do_write(uint32_t size,const char*buf){
  ssize_t l=-1;
  for(uint32_t i=0;i<size;i+=l){
    for(;l<0;)
      l=syscall(SYS_write,1,buf+i,size-i);
  }
}

void
write_s(uint32_t size,const char*s){
  static char buf[128];
  static uint32_t index=0;

  if(size==0){
    do_write(index,buf);
    index = 0;
    return;
  }

  for(uint32_t i=0;;){
    for(;(i<size)&&(index<sizeof(buf));++index,++i)
      buf[index]=s[i];

    if(i>=size)
      break;

    do_write(sizeof(buf),buf);
    index = 0;
  }
}

void
write_atom(struct atom*a){
  write_s(a->size,a->s);
}

void
write_short_atom(term_t term){
  static const char alphabet[]="\0000123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_";

  const uint8_t *u=(uint8_t*)&term;
  char s[9]={0};

  s[0] = alphabet[u[1]>>2];
  s[1] = alphabet[((u[1]<<4)|(u[2]>>4))&0x3F];
  s[2] = alphabet[((u[2]<<2)|(u[3]>>6))&0x3F];
  s[3] = alphabet[u[3]&0x3F];
  s[4] = alphabet[u[4]>>2];
  s[5] = alphabet[((u[4]<<4)|(u[5]>>4))&0x3F];
  s[6] = alphabet[((u[5]<<2)|(u[6]>>6))&0x3F];
  s[7] = alphabet[u[6]&0x3F];

  write_s(u[0]>>4,s);
}

#define ERROR_MSG(e,l) case e: write_s(l, #e); break;

void
print_term(struct lisp0_state*state,term_t term){
  X = term;
  CALL(L_print_term);
  write_s(1,"\n");
  write_s(0,NULL);
  return;

 L_print_term:
  switch(TAG(X)){
  case TAG_POINTER:
    if(!X){
      write_s(2,"()");
      break;
    }
    switch(PTAG(X)){
    case PTAG_ATOM:
      write_atom((struct atom*)X);
      break;
    case PTAG_LIST:
      goto L_print_list;
    case PTAG_LAMBDA:
      write_s(8,"#LAMBDA<");
      PUSH(CDR(X));
      X = CAR(X);
      CALL(L_print_term);
      write_s(1," ");
      X = POP();
      CALL(L_print_term);
      write_s(1,">");
      break;
    case PTAG_MACRO:
      write_s(7,"#MACRO<");
      PUSH(CDR(X));
      X = CAR(X);
      CALL(L_print_term);
      write_s(1," ");
      X = POP();
      CALL(L_print_term);
      write_s(1,">");
      break;
    default:
      write_s(8,"#INVALID_PTR");
      break;
    };
    break;
  case TAG_ATOM:
    write_short_atom(X);
    break;
  case TAG_PRIMITIVE:
    write_s(6,"#PRIM<");
    switch(VALUE(X)){
    case PRIM_QUOTE:  write_s(5,"quote");  break;
    case PRIM_ATOM:   write_s(4,"atom");   break;
    case PRIM_EQ:     write_s(5,"eq");     break;
    case PRIM_COND:   write_s(4,"cond");   break;
    case PRIM_CAR:    write_s(3,"car");    break;
    case PRIM_CDR:    write_s(3,"cdr");    break;
    case PRIM_CONS:   write_s(4,"cons");   break;
    case PRIM_LABEL:  write_s(5,"label");  break;
    case PRIM_LAMBDA: write_s(6,"lambda"); break;
    case PRIM_MACRO:  write_s(5,"macro");  break;
    default:          write_s(3,"BAD");    break;
    };
    write_s(1,">");
    break;
  case TAG_ERROR:
    write_s(1,"#");

    switch(VALUE(X)){
    ERROR_MSG(E_NO_VALUE,      10);
    ERROR_MSG(E_ATOM_TOO_LONG, 15);
    ERROR_MSG(E_RIGHT_PAREN,   13);
    ERROR_MSG(E_INVALID_CHAR,  14);
    ERROR_MSG(E_UNBOUND,       9);
    ERROR_MSG(E_IMPROPER_LIST, 15);
    ERROR_MSG(E_ARGUMENT,      10);
    ERROR_MSG(E_BAD_PRIMITIVE, 15);
    ERROR_MSG(E_COND_END,      10);
    ERROR_MSG(E_NOT_CALLABLE,  14);
    ERROR_MSG(E_BAD_EXPR,      10);
    default: write_s(9,"E_UNKNOWN"); break;
    };
    break;
  default:
    write_s(8,"#INVALID");
    break;
  };
  RETURN(NIL);

 L_print_list:
  write_s(1,"(");
 L_print_list_loop:
  PUSH(CDR(X));
  X = CAR(X);
  CALL(L_print_term);
  X = POP();
  if(IS_LIST(X)){
    write_s(1," ");
    goto L_print_list_loop;
  }
  if(X){
    write_s(1,".");
    CALL(L_print_term);
  }
  write_s(1,")");
  RETURN(NIL);
}
