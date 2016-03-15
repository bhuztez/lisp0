#include <stdio.h>
#include "internal.h"

int
lisp0_main(struct lisp0_state*state){
  init_env(state);

  for(;;){
    term_t t = read_term(state);
    if(IS_ERROR(t)){
      if (VALUE(t)!=E_NO_VALUE)
        print_term(state,t);
      return VALUE(t)!=E_NO_VALUE;
    }

    t = eval_term(state,t);
    print_term(state,t);
  };
}
