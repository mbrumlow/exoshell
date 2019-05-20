
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define _GNU_SOURCE
#include <unistd.h>

#include <ecl/ecl.h>

static int target_pty = 0; 

#define DEFUN(name,fun,args)                    \
  cl_def_c_function(c_string_to_object(name),   \
                    (cl_objectfn_fixed)fun,     \
                    args)

cl_object lisp(const char *call) {
  return cl_safe_eval(c_string_to_object(call), Cnil, Cnil);
}

char *elc_object_to_c_string(cl_object o) {

  int l = o->string.dim;
  char *s = malloc(l+1);
  memset(s, 0, l+1);

  for(int i=0; i < l; i++) {
    s[i] = (char) o->string.self[i];
  }

  return s;
}

cl_object term_send_raw(cl_object i) {

  cl_object ret = Cnil;
  char *buf = elc_object_to_c_string(i);
  
  if(target_pty) {
    write(target_pty, buf, i->string.dim);
    ret = Ct; 
  } 
  
  if(buf) {
    free(buf);
  }
  
  return ret;
}

void initialize_cl(int argc, char **argv) {
  ecl_set_option(ECL_OPT_TRAP_SIGINT, 0);
  cl_boot(argc, argv);
  atexit(cl_shutdown);

  lisp("(load \"initrc.lisp\")");
  
  DEFUN("term-send-raw", term_send_raw, 1);
}


void exoshell(int argc, char **argv, int in, int out, int target) {

  char command;
  
  target_pty = target; 
  
  initialize_cl(argc,argv);
  
  for(;;) {

    int rc = read(in, &command, 1);
    if( rc == -1 ) {
      if( errno != EINTR)
        break;
      else
        continue;
    }

    switch(command){
    case 's':
      lisp("(exoshell-start)");
      write(out, "c", 1); 
      break;
    }
  }

}

