
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


#include "log.h"
#include "term.h"
#include "lisp.h"
#include "test.h"

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

struct term *close_minibuffer(struct term *target, struct term *control, struct term *current, struct term *last) {
  
  struct winsize wbuf;
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);

  if(control != last) { 
    term_set_extra(control, "\033[m"); 
    term_select(control);
    term_unset_extra(control);
    //term_clear(control);
    control->init = 0; 
  }

  term_select(target);
  term_resize(target, 1, wbuf.ws_row, wbuf.ws_col);
  term_unset_extra(target);
  term_select(target);
  return target;
  
}

void open_minibuffer(struct term *target, struct term *control, int control_pty, int size) {
    
    struct winsize wbuf;
    
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);

    // Prep top terminal.
    term_resize(target, 1, wbuf.ws_row - size, wbuf.ws_col); 
    term_set_extra(target, "\033[m");
    term_save(target); 
    
    // Initialize bottom terminal.
    term_init(control, wbuf.ws_row-size+2, wbuf.ws_row, wbuf.ws_col,
              STDIN_FILENO, control_pty);
    control->id = 2; 
    term_set_extra(control, "\033[32m\033[40m\033[1m");  

    term_select(control);
    term_clear(control); 

}

struct term *resize(struct term *target, struct term *control, struct term *current, struct term *last, int size) {
    
    struct winsize wbuf;
    int bottom;
    
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
    bottom = wbuf.ws_row; 

    if(target != last) { 

      // control is active, minibuffer open.
      if(current == control) { 
        term_save(control);
        bottom = wbuf.ws_row-size; 
      }
      

    }

    term_select(target);
    term_resize(target, 1, bottom, wbuf.ws_col); 
    term_select(target);
    
    if(control != current)
      return target;

    term_select(control);
    term_resize(control, bottom+2, wbuf.ws_row, wbuf.ws_col); 
    term_select(control);

    return control; 
}

int exoshell_main(int argc, char **argv) { 

  int ret; 
  int target_pid;
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
  
  target.init = 0;
  control.init = 0; 

  log_init(argc, argv); 

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  
  target_pid = forkpty(&target_pty, NULL, NULL, &wbuf); 
  if(!target_pid){ // child
    char *cmd = "/bin/bash";
    char *argv[2];
    argv[0] = "bash";
    argv[1] = NULL;
    execvp(cmd, argv);
  }

  pipe(control_pipe_in);
  pipe(control_pipe_out);

  control_pid = forkpty(&control_pty, NULL, NULL, &wbuf);
  if(!control_pid) { // child

    close(control_pipe_in[1]);
    close(control_pipe_out[0]);

    exoshell(argc, argv, control_pipe_in[0], control_pipe_out[1], target_pty); 
     _exit(0);
  }
  
  // Initialize target terminal. 
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, target_pty);
  target.id = 1;
  
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
        term_send(current, &input, ret); 
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
            term_save(&target);
            term_select(&control); 
            last = &control;
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

          // This is not likely to happen.
          // But we should ensure target term is selected before opening the minibuffer.
          if(last != &target){
            term_save(&control);
            term_select(&target); 
            last = &target;
          }
          
          open_minibuffer(&target, &control, control_pty, 12); 
          last = &control;
          current = &control;
          kill(target_pid, SIGWINCH);
          dprintf(control_pipe_in[1], "s");
          
          break;
        case SIGWINCH:

          last = resize(&target, &control, current, last, 12); 
          kill(target_pid, SIGWINCH);
          kill(control_pid, SIGWINCH);
                
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

        last = close_minibuffer(&target, &control, current, last);
        kill(target_pid, SIGWINCH);
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
  kill(target_pid, SIGTERM); 
  waitpid(target_pid, &wstatus, 0);

  // Put the term back to working order.
  // Hey, maybe we should save the flags before we change them? 
  tcgetattr(STDIN_FILENO, &tios);
  tios.c_lflag |= ICANON;
  tios.c_lflag |= (ECHO | ECHONL);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);

  return 0; 
}

int exoshell_test(int argc, char **argv) {
  return test_run(); 
}

int main(int argc, char **argv) {

  int i; 
  for(i = 0; i < argc; i++) {
    if(strcmp(argv[i], "-test") == 0) {
      return exoshell_test(argc, argv); 
    }
  }
  
  return exoshell_main(argc, argv); 
}
