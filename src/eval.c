#include "internal.h"

bool
atom_eq(char *x,char *y){
  for(;(*x)||(*y);++x,++y)
    if(*x!=*y)
      return false;
  return true;
}


bool
eq(term_t x, term_t y){
  if((TAG(x)==TAG_ATOM)&&(TAG(y)==TAG_ATOM))
    return x==y;

  if(TAG(x)||TAG(y))
    return false;

  if((!x)&&(!y))
    return true;

  if((PTAG(x)!=PTAG_ATOM)||(PTAG(y)!=PTAG_ATOM))
    return false;

  if(x==y)
    return true;

  return atom_eq(((struct atom*)x)->s,((struct atom*)y)->s);
}


term_t
subst(term_t env,term_t t){
  for(term_t e=env;e;e=CDR(e))
    if(eq(t,CAR(CAR(e))))
      return CDR(CAR(e));

  return ERR(E_UNBOUND);
}


term_t
eval_term(struct lisp0_state*state,term_t term){
  X = term;
  CALL(L_eval);
  return X;

 L_eval:
  if(IS_ERROR(X)){
    RETURN(X);
  }

  if(IS_ATOM(X)){
    RETURN(subst(ENV,X));
  }

  if(!IS_LIST(X)){
    RETURN(ERR(E_BAD_EXPR));
  }

  PUSH(CDR(X));
  X = CAR(X);
  CALL(L_eval);
  Y = POP();

  switch(TAG(X)){
  case TAG_ERROR: RETURN(X);
  case TAG_PRIMITIVE:
    switch(VALUE(X)){
    case PRIM_QUOTE:  goto L_quote;
    case PRIM_ATOM:   goto L_atom;
    case PRIM_EQ:     goto L_eq;
    case PRIM_COND:   goto L_cond;
    case PRIM_CAR:    goto L_car;
    case PRIM_CDR:    goto L_cdr;
    case PRIM_CONS:   goto L_cons;
    case PRIM_LABEL:  goto L_label;
    case PRIM_LAMBDA: goto L_lambda;
    case PRIM_MACRO:  goto L_macro;
    default:          break;
    };
    break;
  case TAG_POINTER:
    if(!X) break;
    switch(PTAG(X)){
    case PTAG_LAMBDA: goto L_eval_lambda;
    case PTAG_MACRO:  goto L_eval_macro;
    default: break;
    };
  default: break;
  }

  RETURN(ERR(E_NOT_CALLABLE));

 L_quote:
  PARSE_ARG1();
  RETURN(X);

 L_atom:
  PARSE_AND_EVAL_ARG1();
  RETURN(((!X)||IS_ATOM(X))?TRUE:FALSE);

 L_eq:
  PARSE_AND_EVAL_ARG2();
  RETURN(eq(Y,X)?TRUE:FALSE);

 L_cond:
  PARSE_ARG();
  PUSH(CDR(Y));
  Y = CAR(Y);
  PARSE_ARG2();
  PUSH(Y);
  CLEAR(Y);
  CALL(L_eval);
  Y = POP();
  switch(X){
  case TRUE:
    X = Y;
    CLEAR(Y);
    goto L_eval;
  case FALSE:
    Y = POP();
    goto L_cond;
  default:
    RETURN(ERR(E_COND_END));
  };

 L_car:
  PARSE_AND_EVAL_ARG1();
  if(!IS_LIST(X)){
    RETURN(ERR(E_ARGUMENT));
  }
  RETURN(CAR(X));

 L_cdr:
  PARSE_AND_EVAL_ARG1();
  if(!IS_LIST(X)){
    RETURN(ERR(E_ARGUMENT));
  }
  RETURN(CDR(X));

 L_cons:
  PARSE_AND_EVAL_ARG2();
 L_cons_1:
  RETURN(cons(state,Y,X));

 L_label:
  PARSE_ARG2();
  PUSH(X);
  X = Y;
  CLEAR(Y);
  CALL(L_eval);
  RETURN_ERROR(X);
  Y = POP();
  CALL(L_cons_1);
  Y = X;
  X = ENV;
  CALL(L_cons_1);
  ENV = X;
  RETURN(CDR(CAR(ENV)));

 L_lambda:
  PARSE_ARG2();
  RETURN(mklist(state,X,Y,PTAG_LAMBDA));

 L_macro:
  PARSE_ARG2();
  RETURN(mklist(state,X,Y,PTAG_MACRO));

 L_eval_lambda:
  PUSH(X);
  push_list_builder(state);
  CALL(L_eval_list);
  Y = pop_list_builder(state,NIL);
  RETURN_ERROR(X);
  X = POP();
  PUSH(CDR(X));
  X = CAR(X);
  push_list_builder(state);
  CALL(L_zip);
  Y = POP();
  PUSH(ENV);
  ENV = pop_list_builder(state,ENV);
  RETURN_ERROR(X);
  X = Y;
  CLEAR(Y);
  CALL(L_eval);
  RETURN_ERROR(ENV);
  ENV = POP();
  RETURN(X);

 L_eval_list:
  if(!Y){
    RETURN(Y);
  }
  PARSE_ARG();
  X = CAR(Y);
  PUSH(CDR(Y));
  CLEAR(Y);
  CALL(L_eval);
  RETURN_ERROR(X);
  list_builder_add_term(state,X);
  Y = POP();
  goto L_eval_list;

 L_eval_macro:
  PUSH(CDR(X));
  X = CAR(X);
  push_list_builder(state);
  CALL(L_zip);
  Y = POP();
  PUSH(ENV);
  ENV = pop_list_builder(state,ENV);
  RETURN_ERROR(X);
  X = Y;
  CLEAR(Y);
  CALL(L_eval);
  ENV = POP();
  goto L_eval;

 L_zip:
  if((!X)||(!Y))
    goto L_zip_finish;

  RETURN_ERROR(X);
  RETURN_ERROR(Y);

  if(!IS_LIST(X)){
    RETURN(ERR(E_IMPROPER_LIST));
  }
  if(!IS_LIST(Y)){
    RETURN(ERR(E_IMPROPER_LIST));
  }
  list_builder_add_term(state,cons(state,CAR(X),CAR(Y)));
  X = CDR(X);
  Y = CDR(Y);
  goto L_zip;

 L_zip_finish:
  if(X||Y){
    RETURN(ERR(E_ARGUMENT));
  }

  RETURN(NIL);
}

void
setenv(struct lisp0_state*state,uint32_t size,const char *s,term_t prim){
  X = cons(state,atom(state,size,s),prim);
  ENV = cons(state,X,ENV);
}

void
init_env(struct lisp0_state*state){
  setenv(state,5,"macro",  PRIM(MACRO));
  setenv(state,6,"lambda", PRIM(LAMBDA));
  setenv(state,5,"label",  PRIM(LABEL));
  setenv(state,4,"cons",   PRIM(CONS));
  setenv(state,3,"cdr",    PRIM(CDR));
  setenv(state,3,"car",    PRIM(CAR));
  setenv(state,4,"cond",   PRIM(COND));
  setenv(state,2,"eq",     PRIM(EQ));
  setenv(state,4,"atom",   PRIM(ATOM));
  setenv(state,5,"quote",  PRIM(QUOTE));
}
