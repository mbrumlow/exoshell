
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pty.h>
#include <termios.h>
#include <errno.h>

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <ecl/ecl.h>

#include "term.h"

struct tstate {
  int state;
  int pos;
  int row, col;
  int top, bot, rows, cols; 
  int x, y; 
  unsigned char buf[4096]; 
};


int sigpipe = 0; 

// Handle signals SIGINT,  SIGWINCH.
static void signal_handler(int signo); 

static void signal_handler(int signo) {
  
  char sig = 0; 
  
  switch(signo) {
  case SIGINT:
    sig = SIGINT;
    signal(SIGINT, signal_handler);
    break;
  case SIGWINCH:
    sig = SIGWINCH;
    signal(SIGWINCH, signal_handler);
    break;
  }
  
  if(sigpipe && sig) {
    int opt = fcntl(sigpipe, F_GETFL);
    fcntl(sigpipe, F_SETFL, opt | O_NONBLOCK);
    write(sigpipe, &sig, 1);
    fcntl(sigpipe, F_SETFL, opt);
  }
}

#define DEFUN(name,fun,args)                    \
  cl_def_c_function(c_string_to_object(name),   \
                    (cl_objectfn_fixed)fun,     \
                    args)

// Define a function to run arbitrary Lisp expressions
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


int target_term_fd = 0; 
cl_object send_raw(cl_object i) {

  char *s = elc_object_to_c_string(i);
  if(target_term_fd) {
    write(target_term_fd, s, i->string.dim);
  }
  free(s);
  return Cnil;
}

void initialize_cl(int argc, char **argv) {
  ecl_set_option(ECL_OPT_TRAP_SIGINT, 0);
  cl_boot(argc, argv);
  atexit(cl_shutdown);

  lisp("(load \"initrc.lisp\")");
  DEFUN("send-raw", send_raw, 1);
}

void close_minibuffer(struct term *target, struct term *control) {

  struct winsize wbuf;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);

  term_set_extra(control, "\033[m"); 
  term_select(control);
  term_clear(control);
  term_unset_extra(control);
  
  term_unset_extra(target); 
  term_select(target);
  term_resize(target, 1, wbuf.ws_row, wbuf.ws_col);
}

void open_minibuffer(struct term *target, struct term *control, int control_pty, int size) {
    
    struct winsize wbuf;
    
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
    term_resize(target, 1, wbuf.ws_row - size, wbuf.ws_col); 

    term_init(control, wbuf.ws_row-size+2, wbuf.ws_row, wbuf.ws_col,
              STDIN_FILENO, control_pty);
    control->cursor_row = wbuf.ws_row-size + 2;
    control->cursor_column = 1;

    term_set_extra(control, "\033[32m\033[40m\033[1m");  
    term_select(control);
    term_clear(control); 
    term_set_extra(target, "\033[m"); 
}

int main(int argc, char **argv) {

  int ret; 
  int pid;
  int control_pid; 
  int wstatus;
  int target_pty;
  int control_pty; 
  int signal_pipe[2];
  int control_pipe_in[2];
  int control_pipe_out[2]; 
  fd_set readfds;
  sigset_t sigmask;
  struct winsize wbuf;
  struct termios tios;
  struct term target;
  struct term control; 
  struct term *last = NULL;
  struct term *current = NULL;
  
  unsigned char input[4096], output[4096];

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  
  pid = forkpty(&target_pty, NULL, NULL, &wbuf); 
  if(!pid){ // child
    char *cmd = "/bin/bash";
    char *argv[2];
    argv[0] = "bash";
    argv[1] = NULL;
    execvp(cmd, argv);
  }

  pipe(control_pipe_in);
  pipe(control_pipe_out);

  // make this accessible to the lisp fork.
  // This will be removed later and injecting text to the target terminal will
  // be done over IPC via control_pipe_out. 
  target_term_fd = target_pty; 
  
  control_pid = forkpty(&control_pty, NULL, NULL, &wbuf);
  if(!control_pid) { // child

    close(control_pipe_in[1]);
    close(control_pipe_out[0]);
    initialize_cl(argc,argv);

    for(;;) {
      char c;
      int rc = read(control_pipe_in[0], &c, 1);
      if( rc == -1 ) {
        if( errno != EINTR)
          break;
        else
          continue;
      }

      switch(c){
      case 's':
        lisp("(start-shell)");
        dprintf(control_pipe_out[1], "c");
        break;
      }
    }
     
     _exit(0);
  }

  
  // Initialize target terminal. 
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, target_pty);

  // Set up pipe based signal handler. 
  pipe(signal_pipe);
  sigpipe = signal_pipe[1];
  
  // Clear the host term. 
  dprintf(1, "\033[2J\033[;H");
  
  signal(SIGINT, signal_handler);
  signal(SIGWINCH, signal_handler);
  
  // Disable ICANON and ECHO on the hos terminal.
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  tcgetattr(STDIN_FILENO, &tios);
  tios.c_lflag &= ~ICANON;
  tios.c_lflag &= ~(ECHO | ECHONL);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);
  
  int max = -1;
  if(max < STDIN_FILENO) max = STDIN_FILENO;
  if(max < target_pty) max = target_pty;
  if(max < control_pty) max = control_pty;
  if(max < signal_pipe[0]) max = signal_pipe[0];
  if(max < control_pipe_out[0]) max = control_pipe_out[0];

  current = &target; 
  
  for(;;) { // main loop
    
    FD_ZERO(&readfds);

    FD_SET(STDIN_FILENO, &readfds);
    FD_SET(target_pty, &readfds);
    FD_SET(control_pty, &readfds);
    FD_SET(signal_pipe[0], &readfds);
    FD_SET(control_pipe_out[0], &readfds);

    int rc = pselect(max+1, &readfds, NULL, NULL, NULL, &sigmask);
    if( rc == -1 ) {
      if( errno != EINTR) 
        break;
      else
        continue;
    }
    
    if(FD_ISSET(STDIN_FILENO, &readfds)) {
      ret = read(STDIN_FILENO, &input, sizeof(input));
      if(ret != -1) {
        term_write(current, &input, ret); 
      } else {
        break; 
      }
    }
    
    if(FD_ISSET(target_pty, &readfds)) {
      ret = read(target_pty, &output, sizeof(output));
      if(ret != -1) {
        
        if(last != &target){
          term_save(&control);
          term_select(&target); 
          last = &target;
        }
        

        term_update(&target, &output[0], ret);

        // Keeps the cursor on control if active. 
        if(current == &control) { 
          if(last != &control){
            term_save(&target);
            term_select(&control); 
            last = &control;
          }
        }
      
      } else {
        break;
      }
    }

    if(FD_ISSET(control_pty, &readfds)) {
      ret = read(control_pty, &output, sizeof(output));
      if(ret != -1) {
        if(current == &control) { 
          if(last != &control){
            term_save(&target);
            term_select(&control); 
            last = &control;
          }
          term_update(&control, &output[0], ret);
        }
      } else {
        break;
      }
    }
    
    if(FD_ISSET(signal_pipe[0], &readfds)) {
      int opt = fcntl(signal_pipe[0], F_GETFL);
      fcntl(signal_pipe[0], F_SETFL, opt | O_NONBLOCK);
      int sig; 
      ret = read(signal_pipe[0], &sig, 1);
      fcntl(signal_pipe[0], F_SETFL, opt);
      if(ret != -1) {
        switch(sig) {
        case SIGINT:

          open_minibuffer(&target, &control, control_pty, 12); 
          last = NULL; 
          current = &control;
          dprintf(control_pipe_in[1], "s");
          
          break;
        case SIGWINCH:
          // TODO handle resizing. 
          /* ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf); */
          /* ioctl(master, TIOCSWINSZ, &wbuf); */
          /* kill(pid, SIGWINCH); */
          break;
        }
        
      } else {
        break;
      }
    }
    
    if(FD_ISSET(control_pipe_out[0], &readfds)) {
      char c;
      read(control_pipe_out[0], &c, 1);
      switch(c) {
      case 'c':
        close_minibuffer(&target, &control);
        current = &target;
        break;
      }
    }
    
  }

  // Shutdown the control process. 
  close(control_pipe_in[0]); 
  kill(control_pid, SIGTERM);
  waitpid(control_pid, &wstatus, 0);
  
  // Wait for our child, we don't want any zombies.
  kill(pid, SIGTERM); 
  waitpid(pid, &wstatus, 0);

  // Put the term back to working order.
  // Hey, maybe we should save the flags before we change them? 
  tcgetattr(STDIN_FILENO, &tios);
  tios.c_lflag |= ICANON;
  tios.c_lflag |= (ECHO | ECHONL);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);

}

