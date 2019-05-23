
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h> 
#include <pty.h>

#include "util.h"
#include "log.h"
#include "term.h"
#include "term_table.h"

#define TERM_DEBUG(flag, term, fmt, arg...) term_debug(flag, term, fmt, ##arg); 

void term_debug(int flag, struct term *t, char *fmt, ...) {

  int size = 0; 
  char *p = NULL; 
  va_list ap;
  
  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if(size < 0)
    return;

  size++;
  p = malloc(size);
  if (p == NULL)
    return;

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if(size < 0){
    free(p);
    return;
  }

  debug(flag, "TERM[%d]: %s", t->id, p);
  free(p); 

}

void term_write_raw(struct term *t, char *buf, int size) {
  TERM_DEBUG(TERM_WRITE, t, "WRITE(%d): '%s'\n", size, buf);
  writefd(t->host, buf, size);
}

void term_write(struct term *t, char *fmt, ...) {

  int size = 0; 
  char *p = NULL; 
  va_list ap;
  
  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if(size < 0)
    return;

  size++;
  p = malloc(size);
  if (p == NULL)
    return;

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if(size < 0){
    free(p);
    return;
  }

  TERM_DEBUG(TERM_WRITE, t, "WRITE(%d): '%s'\n", size, p); 
  term_write_raw(t, p, size); 
  free(p); 
}

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

// TODO: make lookup table for faster lockups.
int last_esc(struct term *t) {
  
  int pos = t->pos - 1;
  if(pos < 0) return 0; 
  
  while(pos >= 0) {
    if(esc_start[t->buf[pos]])
      return pos;
    pos--;
  }
  return -1;
  
}

// Rewinds to the start of the last escape code. 
int rewind_esc(struct term *t) {
  int ret; 
  int pos = t->pos - 1;
  if(pos < 0) return 0; 

  pos = last_esc(t);
  if(pos < 0) return -1;

  t->buf[t->pos] = '\0'; 
  ret = t->pos - pos; 
  t->pos = pos; 
  return ret; 
}

void checkpoint(struct term *t) {
  t->checkpoint = t->pos; 
}

// Saves current term state. 
void term_save(struct term *t) {
  
  if(!t->init) return;
  
  term_write(t, "\033[6n");
  read_cursor(t->host, &t->cursor_row, &t->cursor_column);

  TERM_DEBUG(TERM, t, "term_save: top: %d, bot: %d, cr: %d, cc: %d\n",
             t->top, t->bottom, t->cursor_row, t->cursor_column);
  
  /* t->cursor_row = t->check_row; */
  /* t->cursor_column = t->check_column; */
  
  if(t->check_row != t->cursor_row || t->check_column != t->cursor_column) {
    TERM_DEBUG(CURSOR|FIXME|INFO, t, "MISMATCH: row_want: %d, row_have: %d, column_want: %d, column_have: %d\n",
               t->cursor_row, t->check_row, t->cursor_column, t->check_column);

    // Going to re-sync them.
    // TODO log: and produce a buffer containing the term changes this to
    // happen. 
    t->check_row = t->cursor_row;
    t->check_column = t->cursor_column;
    
  }
}

void term_save_cursor(struct term *t) {
  term_write(t, "\033[6n");
  read_cursor(t->host, &t->check_row, &t->check_column);
  TERM_DEBUG(CURSOR, t, "term_save_cursor: top: %d, bottom: %d, row: %d, col: %d || nrow: %d, ncol: %d\n",
             t->top, t->bottom, t->cursor_row, t->cursor_column,
             t->next_row, t->next_column);

}


void term_set_extra(struct term *t, const char *buf) {
  term_unset_extra(t); 
  t->extra = strdup(buf); 
}

void term_unset_extra(struct term *t) {
  if(t->extra) {
    free(t->extra);
    t->extra = NULL;
  }
}

// Sets up the host terminal to receive output. 
void term_select(struct term *t) {
  if(t->extra){
    write(t->host, t->extra, strlen(t->extra));
  }

  TERM_DEBUG(TERM, t, "term_select: top: %d, bot: %d, cr: %d, cc: %d\n",
             t->top, t->bottom, t->cursor_row, t->cursor_column);

  term_write(t, "\033[%d;%dH\033[%d;%dr\033[?1;9l\033[%d;%dH",
             t->cursor_row, t->cursor_column, t->top, t->bottom, t->cursor_row, t->cursor_column);
}

// Clears terminal region. 
void term_clear(struct term *t) {

  int row, col;
  
  term_write(t, "\033[6n");
  read_cursor(t->host, &row, &col);
  
  // Only supports to terms, region erase seems to only work on xterm :/ 
  if(t->top == 1) {
    term_write(t, "\033[%d;%dH\033[1J\033[%d;%dH", t->bottom, 1, row, col);
  } else { 
    term_write(t, "\033[%d;%dH\033[0J\033[%d;%dH", t->top, 1, row, col);
  }
}

static inline void debug_esc(struct term *t) {

  int pos, size; 
  char *p = NULL;
  
  if(t->state != CASE_PRINT) return;

  pos = last_esc(t);
  if(pos < 0) return;

  size = (t->pos - pos) + 1; 
  p = malloc(size);
  if(!p) return; // Who do we tell ? 

  strncpy(p, (char *)&t->buf[pos], size);
  p[size-1] = '\0'; 
  
  TERM_DEBUG(ESCAPE, t, "ESCAPE: %s\n", &p[0]);
  free(p);

}

void term_bs(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->needs_wrap = 0;
  if(t->next_column) t->next_column--; 
  
  TERM_DEBUG(CURSOR, t, "term_cr: %d:%d:%d -> %d:%d:%d\n",
             row,col,wrap, t->next_row, t->next_column, t->needs_wrap);
}

void term_cr(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->needs_wrap = 0;
  t->next_column = 1; 
  
  TERM_DEBUG(CURSOR, t, "term_cr: %d:%d:%d -> %d:%d:%d\n",
             row, col, wrap, t->next_row, t->next_column, t->needs_wrap);
}

void term_ff(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->needs_wrap = 0;
  t->next_row = t->top;
  t->next_column = 1;
  
  TERM_DEBUG(CURSOR, t, "term_ff: %d:%d:%d -> %d:%d:%d\n",
             row,col,wrap, t->next_row, t->next_column, t->needs_wrap);
}

void term_lfvt(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->next_row++; 
  if(t->next_row > t->bottom) {
    t->next_row = t->bottom;
  }
  t->needs_wrap = 0;
  
  TERM_DEBUG(CURSOR, t, "term_lf: %d:%d:%d -> %d:%d:%d\n",
             row,col,wrap, t->next_row, t->next_column, t->needs_wrap);
}

void term_cursor_wrap(struct term *t) {

  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;
  
  // FUCK terminals are weird. 
  if(t->needs_wrap) {
    t->next_column = 1;
    if(t->next_row < t->bottom ) {
      t->next_row += t->needs_wrap;
    }
    if(t->next_row > t->bottom) {
      t->next_row = t->bottom;
    }
    t->needs_wrap = 0;
  }

  TERM_DEBUG(CURSOR, t, "cursor_wrap: %d:%d:%d -> %d:%d:%d\n",
        row,col,wrap, t->next_row, t->next_column, t->needs_wrap);  
}

void term_cursor_next(struct term *t) {

  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  term_cursor_wrap(t);
  
  if(t->next_column == t->columns) {
    t->needs_wrap++; 
  } else if(t->next_column < t->columns){
    t->next_column++;
  }

  TERM_DEBUG(CURSOR, t, "cursor_next: %d:%d:%d -> %d:%d:%d\n",
        row,col,wrap, t->next_row, t->next_column, t->needs_wrap);
}

void term_flush(struct term *t) {
  
  int otop = t->top;
  int obot = t->bottom;
  int ocr = t->cursor_row;
  int occ = t->cursor_column;
  int occr = t->check_row;
  int occc = t->check_column; 
  
  int i,z; 
  if(t->checkpoint > 0) {
    TERM_DEBUG(TERM, t, "term_flush_A: cp: %d,  top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d, ccr: %d -> %d, ccc: %d -> %d\n",
               t->checkpoint, otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column, occr, t->check_row, occc, t->check_column);

    /* write(t->host, &t->buf[0], t->checkpoint); */
    term_write_raw(t, (char *) &t->buf[0], t->checkpoint);

    memmove(&t->buf[0], &t->buf[t->checkpoint], t->pos - t->checkpoint);
    
    /* z = 0; */
    /* for(i = t->checkpoint; i < t->pos; i++, z++) { */
    /*   t->buf[z] = t->buf[i]; */
    /* } */
    
    t->pos = t->pos - t->checkpoint;
    t->checkpoint = 0;
    t->check_row = t->next_row;
    t->check_column = t->next_column;

    TERM_DEBUG(TERM, t, "term_flush_B: cp: %d,  top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d, ccr: %d -> %d, ccc: %d -> %d\n",
               t->checkpoint, otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column, occr, t->check_row, occc, t->check_column);

  }
}

void update_state_buf(struct term *t, unsigned char c) {
  t->buf[t->pos++] = c;
  t->buf[t->pos] = '\0'; 
}

static inline int esc_check(struct term *t, int state) {
  if(t->state != state) return 1;
  return 0; 
}

int update(struct term *t, unsigned char c) {

  TERM_DEBUG(UPDATE, t, "'%c':%d state: %d\n", c, c, t->state); 
  
  switch(t->state) {
    
  case CASE_TODO:
    t->state = CASE_PRINT;
    update_state_buf(t, c); 
    return esc_check(t, CASE_TODO);
    
  case CASE_ESC:
    t->state = esc_table[c];
    update_state_buf(t, c); 
    return esc_check(t, CASE_ESC); 

  case CASE_7BIT:
    t->state = CASE_PRINT;
    update_state_buf(t, c); 
    return esc_check(t, CASE_7BIT);
    
  case CASE_CSI:
    t->state = csi_table[c];
    update_state_buf(t, c);
    return esc_check(t, CASE_CSI); 

  case CASE_OSC:
    switch(c) {
    case 0x7:  // BELL
    case 0x9c: // ST;
      t->state = CASE_PRINT;
      update_state_buf(t, c);
      return 1; 
    }
    update_state_buf(t, c);
    return 0;
    
  default: // C1 check. 
    
    switch(c) {
    case 0x8: // BS
      term_bs(t); 
      t->state = CASE_PRINT;
      update_state_buf(t, c);
      return 1; 
      
    case 0xa: // LF
    case 0xb: // VT
      term_lfvt(t); 
      t->state = CASE_PRINT;
      update_state_buf(t, c);
      return 1; 

    case 0xc: // FF
      term_ff(t); 
      t->state = CASE_PRINT;
      update_state_buf(t, c);
      return 1; 
      
    case 0xd: // CR
      term_cr(t); 
      t->state = CASE_PRINT;
      update_state_buf(t, c);
      return 1; 
    
    case 0x1b: // ESC
      t->state = CASE_ESC;
      update_state_buf(t, c); 
      return 0; // Start of a escape sequence.

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
      return 0; // Start of a escape sequence.
    
    case 0x9b: // CSI
      t->state = CASE_CSI;
      update_state_buf(t, c); 
      return 0; // Start of a escape sequence. 
      
    }
  }

  t->state = CASE_PRINT;
  term_cursor_next(t); 
  update_state_buf(t, c);
  return 0; 
}

void filter_CUP(struct term *t) {

  int pos = last_esc(t);
  int row = 0;
  int col = 0;
  int *cur = &row;

  // CSI P s ; P s H
  pos++; // moves to -> or first parameter. 
  for(; pos < t->pos; pos++) {

    if(t->buf[pos] == '[') continue;
    if(t->buf[pos] == ';') {
      cur = &col;
      continue;
    }
    if(t->buf[pos] == 'H') break;
    
    // probably should log this? 
    if(!(t->buf[pos] >= '0' && t->buf[pos] <= '9')) return;
    
    *cur = *cur * 10 + (t->buf[pos] - 48);
  }

  if(!row) row = t->top;
  if(!col) col = 1;

  if(row < t->top) row = t->top; 
  if(row > t->bottom) row = t->bottom;
  if(col> t->columns) col = t->columns;
  
  t->next_row = row;
  t->next_column = col;
  t->needs_wrap = 0; 
  
  // Rewrite the cursor move update.
  // It seems programs like top like to request the cursor to be moved one
  // outside of the region / terminal.
  rewind_esc(t);
  int len = sprintf((char *)&t->buf[t->pos], "\e[%d;%dH", row, col);
  t->pos += len;
  
  checkpoint(t); // move our checkpoint up. 
}

void filter(struct term *t) {

  switch(t->state) {
  case CSI_CUP_FILTER:
    filter_CUP(t);
    t->state = CASE_PRINT;
    break;
  case CASE_ED_FILTER:
    rewind_esc(t);
    t->state = CASE_PRINT;
    // TODO remap, once we have proper cursor info.
  case CASE_LOG:
    t->state = CASE_PRINT;
    break; 
  }

}

void term_update(struct term *t, unsigned char *buf, int len) {
  int i; 
  for(i = 0; i < len; i++, buf++) {
    if(update(t, *buf)) {
      debug_esc(t); 
    }
    filter(t);
    if(t->state == CASE_PRINT)
      checkpoint(t); 
  }
  term_flush(t);
}

void term_resize(struct term *t, int top, int bottom, int columns) {

  int otop = t->top;
  int obot = t->bottom;
  int ocr = t->cursor_row;
  int occ = t->cursor_column;
  int diff = t->bottom - bottom;
  struct winsize wbuf;

  TERM_DEBUG(TERM, t, "term_resize_A: top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d\n",
             otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column);
    
  term_save(t);

  TERM_DEBUG(TERM, t, "term_resize_B: top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d\n",
             otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column);


  
  if(t->cursor_row > bottom) {
    TERM_DEBUG(TERM|CURSOR, t, "term_resize_C: diff: %d, top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d\n",
               diff, otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column);
  
    t->cursor_row -= diff;
    t->next_row -= diff; // TODO: do we need to bump next? 
    t->check_row -= diff;
    
    term_write(t, "\033[%d;%dr\033[%dS\033[%d;%dH",
               t->top, t->bottom, diff,
               t->cursor_row, t->cursor_column);
  } // TODO: else clear region? 

  t->top = top;
  t->bottom = bottom;
  
  memset(&wbuf, 0, sizeof(wbuf));
  wbuf.ws_row = bottom - top + 1;
  wbuf.ws_col = columns;
  
  ioctl(t->pty, TIOCSWINSZ, &wbuf);

  TERM_DEBUG(TERM, t, "term_resize_D: top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d\n",
             otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column);

}

void term_send(struct term *t, const void *buf, size_t count) {
  write(t->pty, buf, count); 
}

void term_init(struct term *t, int top, int bottom, int columns, int host, int pty) {

  struct winsize wbuf;

  t->init = 1; 
  t->state = CASE_PRINT;
  t->pos = 0;
  t->top = top;
  t->bottom = bottom;
  t->columns = columns;
  t->pty = pty;
  t->host = host;
  t->extra = NULL;
  t->cursor_row = top;
  t->cursor_column = 1;
  t->check_row = top;
  t->check_column = 1; 
  t->next_row = top;
  t->next_column = 1;
  t->needs_wrap = 0;
  t->checkpoint = 0;
  
  memset(&wbuf, 0, sizeof(wbuf));
  wbuf.ws_row = bottom - top + 1;
  wbuf.ws_col = columns;
  ioctl(pty, TIOCSWINSZ, &wbuf);

  /* term_debug(t, "term_init: top: %d, bottom: %d, crow: %d, ccol: %d || nrow: %d, ncol: %d\n", */
  /*            t->top, t->bottom, t->cursor_row, t->cursor_column, */
  /*            t->next_row, t->next_column); */

}
