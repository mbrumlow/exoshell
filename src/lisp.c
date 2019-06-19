
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define _GNU_SOURCE
#include <unistd.h>

#include <ecl/ecl.h>

#include "util.h"
#include "proto.h"

static int target_pty = 0;
static int control_in = 0;
static int control_out = 0;
static int data_in = 0;
static int data_out = 0;

pthread_mutex_t data_mutex;

#define DEFUN(name,fun,args)                    \
  cl_def_c_function(c_string_to_object(name),   \
                    (cl_objectfn_fixed)fun,     \
                    args)

cl_object
lisp (const char *call)
{
  return cl_safe_eval (c_string_to_object (call), Cnil, Cnil);
}

char *
elc_object_to_c_string (cl_object o)
{

  int l = o->string.dim;
  char *s = malloc (l + 1);
  memset (s, 0, l + 1);

  for (int i = 0; i < l; i++)
    {
      s[i] = (char) o->string.self[i];
    }

  return s;
}

cl_object
term_send_raw (cl_object i)
{

  cl_object ret = Cnil;
  char *buf = elc_object_to_c_string (i);

  if (target_pty)
    {
      write (target_pty, buf, i->string.dim);
      ret = Ct;
    }

  if (buf)
    {
      free (buf);
    }

  return ret;
}

char *
term_get_line (uint32_t n)
{

  char *result = NULL;
  uint32_t ret;
  char *s;

  pthread_mutex_lock (&data_mutex);

  if (proto_send_req (data_out, PROTO_REQ_LINE) != PROTO_OK)
    {
      // TODO: log error.
      printf ("error: failed to request line from terminal.\n");
      return NULL;
    }

  if (proto_send_req (data_out, n) != PROTO_OK)
    {
      // TODO: log error.
      printf ("error: request line did not finish..\n");
      goto out;
    }

  ret = proto_read_string (data_in, &s);
  if (ret == PROTO_NONE)
    {
      goto out;
    }

  if (ret == PROTO_OK)
    {
      result = s;
    }
  else
    {
      printf ("error: request line failed.\n");
      // TODO: log error. 
      goto out;
    }

out:
  pthread_mutex_unlock (&data_mutex);
  return result;
}

static cl_object
term_get_last_line (cl_object l)
{

  char *line = NULL;
  uint32_t n = fix (l);
  cl_object ret = Cnil;

  if (control_out == 0 || control_in == 0)
    return Cnil;

  line = term_get_line (n);
  if (!line)
    return Cnil;

  ret = ecl_make_simple_base_string (line, -1);

  if (line)
    free (line);
  return ret;
}

void
initialize_cl (int argc, char **argv)
{

  char buf[4096];

  ecl_set_option (ECL_OPT_TRAP_SIGINT, 0);
  cl_boot (argc, argv);
  atexit (cl_shutdown);

  snprintf (buf, sizeof (buf), "(setq exoshell-dist \"%s/\")", DATADIR);
  lisp (buf);

  //  lisp("(load \"initrc.lisp\")");

  DEFUN ("term-send-raw", term_send_raw, 1);
  DEFUN ("term-get-last-line", term_get_last_line, 1);

  snprintf (buf, sizeof (buf), "(load \"%s/initrc.lisp\")", DATADIR);
  lisp (buf);

}

/* void *swank(void *ptr) { */

/*   ecl_import_current_thread(ECL_NIL, ECL_NIL); */

/*   lisp("(start-swank)");  */

/*   ecl_release_current_thread(); */

/*   return NULL; */
/* } */

int
exoshell (int argc, char **argv, int in, int out, int din, int dout,
	  int target)
{

  pthread_t swank_thread;
  cl_object ret;

  pthread_mutex_init (&data_mutex, NULL);

  control_in = in;
  control_out = out;
  data_in = din;
  data_out = dout;
  target_pty = target;

  initialize_cl (argc, argv);

  /* if(pthread_create(&swank_thread, NULL, &swank, NULL)) { */
  /*   printf("error: failed to start swank thread\n"); */
  /* } */

  for (;;)
    {

      uint32_t req = proto_read_req (in);
      switch (req)
	{
	case PROTO_SHELL_START:
	  ret = lisp ("(exoshell-start)");
	  cl_print (1, ret);
	  proto_send_req (out, PROTO_SHELL_END);
	  break;

	case PROTO_ERROR:
	  printf ("IPC error\n");
	  return -1;

	default:
	  printf ("IPC unknown: %d\n", req);
	  return -1;
	}

    }

  /* if(pthread_join(swank_thread, NULL)) { */
  /*   printf("error: failed to join swank thread\n"); */
  /* } */

  pthread_mutex_destroy (&data_mutex);

  return 0;

}
