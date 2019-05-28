
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

void clear() {
  // Clear screen set cursor 1,1
  dprintf(STDOUT_FILENO, "\033[2J\033[;H");
}

void get_cursor(int *row, int *col) {

  dprintf(STDOUT_FILENO, "\033[6n"); 
  read_cursor(STDIN_FILENO, row, col);

}

int test_cursor_wrap_01() {

  int row, col, i, c;
  struct term target;
  struct winsize wbuf;
  
  clear();

  get_cursor(&row, &col); 
  c = row * col * 100000; 
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, STDOUT_FILENO);

  unsigned char buf[] = "0";

  for(i = 1; i < c; i++ ) {

    term_update(&target, &buf[0], 1);
    get_cursor(&row, &col);

    if(target.check_row != row) {
      //      clear();
      printf("cursor_row@%d: %d != %d ws_row=%d, ws_col=%d\n",
             i, target.check_row, row, wbuf.ws_row, wbuf.ws_col);
      return 0;
    }

    if(target.check_column != col) {
      //      clear();
      printf("cursor_column@%d: %d != %d ws_row=%d, ws_col=%d, check_row=%d, check_column=%d state=%d, pos=%d\n",
             i, target.check_column, col, wbuf.ws_row, wbuf.ws_col, target.check_row, target.check_column, target.state, target.pos);
      return 0;
    }

  }

  //  clear(); 
  return 1;
}

int test_cursor_wrap_02() {

  int row, col, i;
  struct term target;
  struct winsize wbuf;
  
  clear();

  get_cursor(&row, &col); 
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, STDOUT_FILENO);


  unsigned char buf[2048];
  memset(&buf[0], 0x30, 2048);

  term_update(&target, &buf[0], sizeof(buf)-1);
  get_cursor(&row, &col);

  i = 0;
  if(target.check_row != row) {
      //      clear();
      printf("cursor_row@%d: %d != %d ws_row=%d, ws_col=%d\n",
             i, target.check_row, row, wbuf.ws_row, wbuf.ws_col);
      return 0;
  }

  if(target.check_column != col) {
      //      clear();
    printf("cursor_column@%d: %d != %d ws_row=%d, ws_col=%d, check_row=%d, check_column=%d state=%d, pos=%d\n",
           i, target.check_column, col, wbuf.ws_row, wbuf.ws_col, target.check_row, target.check_column, target.state, target.pos);
    return 0;
  }

  printf("\n");
  
  //  clear(); 
  return 1;
}

int test_cursor_wrap_03() {

  int row, col, i;
  struct term target;
  struct winsize wbuf;
  
  clear();

  get_cursor(&row, &col); 
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);

  
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, STDOUT_FILENO);

  unsigned char buf[] = "000\n000\n000\n0000\n\n00\n\n0\n\n\n\n\n00000\n\013VT\015CR";

  term_update(&target, &buf[0], sizeof(buf)-1);
  term_flush(&target);
  
  get_cursor(&row, &col);

  i = 0;
  if(target.check_row != row) {
      //      clear();
      printf("\ncursor_row@%d: %d != %d ws_row=%d, ws_col=%d\n",
             i, target.check_row, row, wbuf.ws_row, wbuf.ws_col);
      return 0;
  }

  if(target.check_column != col) {
    printf("\ncursor_column@%d: %d != %d ws_row=%d, ws_col=%d, check_row=%d, check_column=%d state=%d, pos=%d\n",
           i, target.check_column, col, wbuf.ws_row, wbuf.ws_col, target.check_row, target.check_column, target.state, target.pos);
    return 0;
  }

  return 1;
}

int test_cursor_wrap_04() {

  int row, col, i;
  struct term target;
  struct winsize wbuf;
  
  clear();

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
      
  int size = 20;
  term_init(&target, wbuf.ws_row-size+2, wbuf.ws_row, wbuf.ws_col, STDOUT_FILENO, STDOUT_FILENO);
  term_select(&target); 
    
  unsigned char buf[] = "000\n000\n000\n0000\n\n00\n\n0\n\n\n\n\n00000\n\013VT\015CR";
  term_update(&target, &buf[0], sizeof(buf)-1);

  term_save(&target); 
  get_cursor(&row, &col);

  i = 0;
  if(target.check_row != row) {
      //      clear();
      printf("\ncursor_row@%d: %d != %d ws_row=%d, ws_col=%d\n",
             i, target.check_row, row, wbuf.ws_row, wbuf.ws_col);
      return 0;
  }

  if(target.check_column != col) {
    printf("\ncursor_column@%d: %d != %d ws_row=%d, ws_col=%d, check_row=%d, check_column=%d state=%d, pos=%d\n",
           i, target.check_column, col, wbuf.ws_row, wbuf.ws_col, target.check_row, target.check_column, target.state, target.pos);
    return 0;
  }

  return 1;
}

int test_cursor_move_01_subtest(int test, struct term *target, unsigned char *buf, int len, struct winsize *wbuf) {

  int row, col;
  
  term_update(target, buf, len);
  term_flush(target);
  get_cursor(&row, &col);
  
  if(target->check_row != row) {
    printf("\ncursor_row@%d: %d != %d ws_row=%d, ws_col=%d\n",
           test, target->check_row, row, wbuf->ws_row, wbuf->ws_col);
    return 0;
  }

  if(target->check_column != col) {
    printf("\ncursor_column@%d: %d != %d ws_row=%d, ws_col=%d, check_row=%d, check_column=%d state=%d, pos=%d\n",
           test, target->check_column, col, wbuf->ws_row, wbuf->ws_col, target->check_row, target->check_column, target->state, target->pos);
    return 0;
  }

  return 1; 
}

int test_cursor_move_01() {

  int i = 0;
  struct term target;
  struct winsize wbuf;
  
  clear();
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, STDOUT_FILENO);

  
  {
    i++;
    unsigned char buf[] = "0000\e[2;1H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\n\e[H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "000000000\e[H\e[5;1H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\e[1;1H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\e[;201H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\e[20;1H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\e[22;22H";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\e[H\e[K";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  {
    i++;
    unsigned char buf[] = "0000\r\n\r\n000\e[2K";
    if(!test_cursor_move_01_subtest(i, &target, &buf[0], sizeof(buf)-1, &wbuf))
      return 0;
  }

  
  return 1;
}

int test_lines_01_subtest(int n) {

  int max = 100; 
  int i = 0;
  struct term target;
  struct winsize wbuf;
  char line[4096];
  
  clear();
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, STDOUT_FILENO);

  for(i = 0; i < n; i++) {
    sprintf(&line[0], "This is line %d\r\n", i);
    term_update(&target, (unsigned char *) &line[0], strlen(&line[0]));
  }

  term_flush(&target);

  if(n < max) max = n; // TODO: fix me, max saved lines is 100. 
  for(i = 0; i < max ; i++) { 
    sprintf(&line[0], "This is line %d", n-(i+1));
    char *s = term_get_last_line(&target, i+1);
    if(strcmp(s, &line[0]) != 0){
      printf("\nn=%d, test: %d: get: %d, expected '%s' but got '%s'\n", n, i, i+1,  &line[0], s);
      printf("line: %d, watermark: %d, mark: %d, wrap: %d, next_row: %d\n",
             target.li.line, target.li.watermark, target.li.mark, target.li.wrap, target.next_row);
      return 0; 
    }
  }


  return 1;
}

int test_lines_01() {

  int i;
  for(i = 0; i < 1000; i++) {
    if(!test_lines_01_subtest(i)) return 0; 
  }
  return 1; 

}

int test_lines_02_subtest(int n) {

  int max; 
  int i,j;
  struct term target;
  struct winsize wbuf;
  char line[4096];
  
  clear();
  
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &wbuf);
  term_init(&target, 1, wbuf.ws_row, wbuf.ws_col, STDIN_FILENO, STDOUT_FILENO);

  max = wbuf.ws_row - 1; 
  
  for(i = 0; i < n; i++) {
    sprintf(&line[0], "%d ###################\r\n", i);
    term_update(&target, (unsigned char *) &line[0], strlen(&line[0]));
  }

  // move to 1;1
  sprintf(&line[0], "\e[1;1H");
  term_update(&target, (unsigned char *) &line[0], strlen(&line[0]));

  if(n < max) max = n; 
  for(j = 0; j < max; j++) {
    sprintf(&line[0], "\e[?2KThis is line %d\r\n", j);
    term_update(&target, (unsigned char *) &line[0], strlen(&line[0]));



    for(i = 0; i < max ; i++) {

      if(i >= (max - j) -1 ) {
        sprintf(&line[0], "This is line %d", max-(i+1));      
      } else{
        sprintf(&line[0], "%d ###################", n-(i+1));      
      }
      
      char *s = term_get_last_line(&target, i+1);
      if(strcmp(s, &line[0]) != 0){
        printf( "\e[%d;1H", wbuf.ws_row - 10);
        printf("\nn=%d, test: %d: get: %d, j: %d, max: %d, expected '%s' but got '%s'\n", n, i, i+1, j, max,  &line[0], s);
        printf("line: %d, watermark: %d, mark: %d, wrap: %d, next_row: %d\n",
               target.li.line, target.li.watermark, target.li.mark, target.li.wrap, target.next_row);
        return 0; 
      }
    }
    
  }

  term_flush(&target);

  


  return 1;
}


int test_lines_02() {

  int i;

  for(i = 0; i < 1000; i++) {
    if(!test_lines_02_subtest(i)) return 0;
  }
  
  return 1; 
}

int test_run() {

  int pass = 1;
  struct termios tios;
  
  // Disable ICANON and ECHO on the hos terminal.
  tcgetattr(STDIN_FILENO, &tios);
  tios.c_lflag &= ~ICANON;
  tios.c_lflag &= ~(ECHO | ECHONL);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);
  
  if(pass && !test_cursor_wrap_01()) pass = 0;
  if(pass && !test_cursor_wrap_02()) pass = 0;
  if(pass && !test_cursor_wrap_03()) pass = 0;
  if(pass && !test_cursor_wrap_04()) pass = 0;
  if(pass && !test_cursor_move_01()) pass = 0;
  if(pass && !test_lines_01()) pass = 0;
  if(pass && !test_lines_02()) pass = 0;
  
  // Put the term back to working order.
  // Hey, maybe we should save the flags before we change them? 
  tcgetattr(STDIN_FILENO, &tios);
  tios.c_lflag |= ICANON;
  tios.c_lflag |= (ECHO | ECHONL);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tios);

  if(!pass) {
    printf("Some test failed!!!\n");
    return -1;
  } else {
    printf("All test passed!!\n"); 
  }
  
  return 0; 
}

