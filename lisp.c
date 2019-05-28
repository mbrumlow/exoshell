
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define _GNU_SOURCE
#include <unistd.h>

#include <ecl/ecl.h>

#include "util.h"
#include "proto.h"

static int target_pty = 0; 
static int control_in = 0;
static int control_out = 0;

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


char *term_get_line(uint32_t n) {

  uint32_t ret; 
  char *s; 
  
  if(proto_send_req(control_out, PROTO_REQ_LINE) != PROTO_OK) {
    // TODO: log error.
    printf("error: failed to request line from terminal.\n");
    return NULL; 
  }
  
  if(proto_send_req(control_out, n) != PROTO_OK) {
    // TODO: log error.
    printf("error: request line did not finish..\n");
    return NULL;
  }
  
  ret = proto_read_string(control_in, &s);
  if(ret == PROTO_OK) return s;
  if(ret == PROTO_NONE) return NULL;

  printf("error: request line failed.\n");
  // TODO: log error. 
  
  return NULL; 
}

char *append_lisp_string(char *d, char *s) {

  int i,j;
  char *p1, *p2;
  
  if(!s) return d;

  if(d) i = strlen(d);
  else i = 0;

  // Ugly -- c++ vectors would have been nice here. 
  j = 3; // 2 For quotes, 1 for space at end
  p1 = s;
  while(*p1++ != '\0') {
    if(*p1 == '"') j++; // Extra slot for escape char.  
    j++;
  }
  
  d = realloc(d, i + j + 1);
  if(!d) return NULL; 

  p1 = s;
  p2 = d + i;

  *p2++ = '"';  
  while(*p1 != '\0') {
    if(*p1 == '"') *p2++ = '\\';
    *p2++ = *p1++; 
  }
  *p2++ = *p1++; 
  p2--; 
  *p2++ = '"';
  *p2++ = ' ';
  *p2++ = '\0'; 
  
  return d; 
}

cl_object term_get_lines(cl_object l) {

  char *line = NULL;
  char *list = NULL;
  uint32_t n, i = 0; 
  uint32_t lines = fix(l);
  cl_object ret;
  
  if(control_out == 0 || control_in == 0) return Cnil; 

  // The goal:
  // '( "string" "string" "string" )
  // '( "\"The\" String" "some String" )
  
  list = malloc(4);
  if(!list) return Cnil;
  list[0] = '\'';
  list[1] = '(';
  list[2] = ' ';
  list[3] = '\0'; 
  
  for(i = 0; i < lines; i++) {
    line = term_get_line(i);
    if(!line) break; 
    list = append_lisp_string(list, line);
    free(line); 
    if(!list) return Cnil; 
  }

  n = strlen(list);
  list = realloc(list, n + 2);
  list[n++] = ')';
  list[n++] = '\0'; 

  ret = lisp(list); 
  free(list);
  return ret;
}

void initialize_cl(int argc, char **argv) {
  ecl_set_option(ECL_OPT_TRAP_SIGINT, 0);
  cl_boot(argc, argv);
  atexit(cl_shutdown);

  lisp("(load \"initrc.lisp\")");
  
  DEFUN("term-send-raw", term_send_raw, 1);
  DEFUN("term-get-lines", term_get_lines, 1);
}

int exoshell(int argc, char **argv, int in, int out, int target) {

  control_in = in;
  control_out = out;
  target_pty = target; 
  
  initialize_cl(argc,argv);

  for(;;) {

    uint32_t req = proto_read_req(in); 
    switch(req){
    case PROTO_SHELL_START:
      lisp("(exoshell-start)");
      proto_send_req(out, PROTO_SHELL_END);
      break;

    case PROTO_ERROR:
      printf("IPC error\n");
      return -1;

    default:
      printf("IPC unknown: %d\n", req);
      return -1; 
    }

  }

  return 0;
    
}

