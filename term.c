
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pty.h>

#include "term.h"
#include "term_table.h"

// Reads cursor escape from host terminal, throwing out any other input.
void read_cursor(int fd, int *row, int *col) {

  int step = 0;
  int *cur = row;
  int t = 0;
  int i = 0; 
  int p = 0;
  unsigned char c;
  
  //"\33[5;28R"
  for(;;)  {
    read(fd, &c, 1); 
    switch(step) {
    case 0:
      if(c == 0x1b) step = 1;
      break;
    case 1:
      if(c == '[') step = 2;
      break;
    case 2:
    case 3:
      if(c >= '0' && c <= '9') {
        i = c - 48; 
        t = *cur;
        t = t * (p * 10) + i;
        p = 1;
        *cur = t;
      } else if (c == ';') {
        p = 0;
        cur = col;
        step = 3;
      } else if ( c == 'R') {
        return; 
      }
    }
  }
}

// Rewinds to the start of the last escape code. 
int rewind_esc(struct term *t) {
  int ret; 
  int pos = t->pos - 1;
  if(pos < 0) return 0; 
  
  while(pos >= 0) {
    switch(t->buf[pos]) {
    case 0x1b: // ESC
    case 0x90: // DCS
    case 0x96: // SPA
    case 0x97: // EPA
    case 0x98: // SOS
    case 0x9a: // DECID
    case 0x9c: // ST
    case 0x9d: // OCS
    case 0x9e: // PM
    case 0x9f: // APC
    case 0x9b: // CSI
      t->buf[t->pos] = '\0'; 
      ret = t->pos - pos; 
      t->pos = pos; 
      return ret; 
    default:
      pos--; 
    }
  }
  return 0; 
}

// Saves current term state. 
void term_save(struct term *t) {
  dprintf(t->host, "\033[6n"); 
  read_cursor(t->host, &t->cursor_row, &t->cursor_column);
}

void term_set_extra(struct term *t, const char *buf) {
  term_unset_extra(t); 
  t->extra = strdup(buf); 
}

void term_unset_extra(struct term *t) {
  if(t->extra) {
    free(t->extra);
    t->extra =NULL;
  }
}

// Sets up the host terminal to receive output. 
void term_select(struct term *t) {
  if(t->extra){
    write(t->host, t->extra, strlen(t->extra));
  }
  dprintf(t->host, "\033[%d;%dr\033[?1;9l\033[%d;%dH",
          t->top, t->bottom, t->cursor_row, t->cursor_column);
}

// Clears terminal region. 
void term_clear(struct term *t) {
  // Only supports to terms, region erase seems to only work on xterm :/ 
  if(t->top == 1) {
    dprintf(t->host, "\033[%d;%dH\033[1J", t->bottom, 1);
  } else { 
    dprintf(t->host, "\033[%d;%dH\033[0J", t->top, 1);
  }
}

void csi_map(struct term *t) {

  // TODO rewrind, then replace. 
  switch(t->state) {
  case CASE_ED_MAP:
    rewind_esc(t);
    // TODO replace with proper clear up from bottom/top; 
  }
  t->state = CASE_PRINT;
  

}

void term_flush(struct term *t) {
  if(t->state == CASE_PRINT && t->pos > 0) { 
    write(t->host, &t->buf[0], t->pos);
    t->pos = 0;
  }
}

void update_state_buf(struct term *t, unsigned char c) {
  t->buf[t->pos++] = c;
}

void update(struct term *t, unsigned char c) {

  switch(t->state) {

  case CASE_TODO:
    t->state = CASE_PRINT;
    update_state_buf(t, c); 
    return;
    
  case CASE_ESC:
    term_flush(t);
    t->state = esc_table[c];
    update_state_buf(t, c); 
    return;

  case CASE_7BIT:
    t->state = CASE_PRINT;
    update_state_buf(t, c); 
    return;
    
  case CASE_CSI:
    t->state = csi_table[c];
    update_state_buf(t, c);
    csi_map(t);
    return;

  case CASE_OSC:
    switch(c) {
    case 0x0:
    case 0x7: // BELL
      t->state = CASE_PRINT;
      update_state_buf(t, c); 
      return;
    default:
      update_state_buf(t, c); 
    }
    return; 

  case CASE_ED_MAP:
    t->state = CASE_PRINT;
    update_state_buf(t, c);
    return; 
    
  default: // C1 check. 

    switch(c) {
    
    case 0x1b: // ESC
      term_flush(t);
      t->state = CASE_ESC;
      update_state_buf(t, c); 
      return;

      // TODO 
    case 0x90: // DCS
    case 0x96: // SPA
    case 0x97: // EPA
    case 0x98: // SOS
    case 0x9a: // DECID
    case 0x9c: // ST
    case 0x9d: // OCS
    case 0x9e: // PM
    case 0x9f: // APC
      t->state = CASE_TODO;
      update_state_buf(t, c); 
      return;
    
    case 0x9b: // CSI
      term_flush(t);
      t->state = CASE_CSI;
      update_state_buf(t, c); 
      return;
      
    }
  }

  t->state = CASE_PRINT;
  update_state_buf(t, c); 
    
}

void term_update(struct term *t, unsigned char *buf, int len) {
  int i;
  for(i = 0; i < len; i++, buf++) {
    update(t, *buf); 
  }
  term_flush(t);
}

void term_resize(struct term *t, int top, int bottom, int columns) {

  int diff = t->bottom - bottom;
  struct winsize wbuf;
  
  term_save(t);

  if(t->cursor_row > bottom) {
    t->cursor_row -= diff;
    dprintf(t->host, "\033[%d;%dr\033[%dS\033[%d;%dH",
            t->top, t->bottom, diff,
            t->cursor_row, t->cursor_column);
  } // else clear region? 

  t->top = top;
  t->bottom = bottom;
  
  wbuf.ws_row = bottom - top - 1; 
  wbuf.ws_col = columns;
  ioctl(t->pty, TIOCSWINSZ, &wbuf);
}

void term_write(struct term *t, const void *buf, size_t count) {
  write(t->pty, buf, count); 
}

void term_init(struct term *t, int top, int bottom, int columns, int host, int pty) {

  struct winsize wbuf;
  
  t->state = CASE_PRINT;
  t->pos = 0;
  t->top = top;
  t->bottom = bottom;
  t->columns = columns;
  t->pty = pty;
  t->host = host;
  t->extra = NULL;
  
  wbuf.ws_row = bottom - top; 
  wbuf.ws_col = columns;
  ioctl(pty, TIOCSWINSZ, &wbuf);

}
