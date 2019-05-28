
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

#define MAX_SAVE_LINES (100)

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

char *term_get_last_line(struct term *t, int n) {

  int p = 0;
     
  TERM_DEBUG(TERM_LINES, t,
             "term_get_last_line: n: %d, START p: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             n, p, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  
  p = t->li.watermark - n;
  TERM_DEBUG(TERM_LINES, t,
             "term_get_last_line: n: %d,  ADJUST_A p: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             n, p, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);

  
  if(t->li.wrap && p < 0) {
    p = MAX_SAVE_LINES + p;
    TERM_DEBUG(TERM_LINES, t,
               "term_get_last_line: m: %d, ADJUST_B p: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               n, p, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  }
  
  if(p > (MAX_SAVE_LINES-1) || p < 0) return NULL;

  TERM_DEBUG(TERM_LINES, t,
             "term_get_last_line: n: %d, END p: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             n, p, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);

  
  return t->lines[p].buf; 
}

int term_line_alloc(struct term *t) {

  struct line *line;

  
  if(!t->lines) return 0;
  if((t->lines + t->li.line)->buf) return 1; 

  line = t->lines + t->li.line;
  line->buf = malloc(t->columns + 1);
  if(!line->buf) {
    free(line); 
    return 0; 
  }

  line->buf[0] = '\0';
  line->size = t->columns;
  return 1; 
}

void term_line_set(struct term *t, int diff, int col) {
  
  int p;

  TERM_DEBUG(TERM_LINES, t,
             "term_line_set: START diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  
  p = t->li.line - diff;

  TERM_DEBUG(TERM_LINES, t,
             "term_line_set: ADJUST_A p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  
  if(!t->li.wrap && p < 0) {
    diff += p;
    p = 0;

    TERM_DEBUG(TERM_LINES, t,
               "term_line_set: ADJUST_B p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  }

  if(t->li.wrap && p < 0) {
    p = MAX_SAVE_LINES + p;

    TERM_DEBUG(TERM_LINES, t,
               "term_line_set: ADJUST_C p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  }

  
  // Handle forward wrap. 
  if(p >= MAX_SAVE_LINES) {
    p = p - MAX_SAVE_LINES;
    t->li.wrap = 1; 

    TERM_DEBUG(TERM_LINES, t,
               "term_line_set: ADJUST_D p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  }
  
  if(p >= MAX_SAVE_LINES || p < 0) return;

  t->li.mark = t->li.mark - (-diff);

  TERM_DEBUG(TERM_LINES, t,
             "term_line_set: ADJUST_E p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);

  if(col < 0) col = 0;
  
  t->li.line = p;
  t->li.col = col;

  TERM_DEBUG(TERM_LINES, t,
             "term_line_set: ADJUST_D p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm :%d\n",
             p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);

  
  if(t->li.mark < 0) {
    t->li.mark = 0;
    t->li.watermark = t->li.line;

    TERM_DEBUG(TERM_LINES, t,
               "term_line_set: ADJUST_F p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  }

  TERM_DEBUG(TERM_LINES, t,
             "term_line_set: END p: %d, diff: %d, row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             p, diff, t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
}


void term_line_next(struct term *t) {

  if(!term_line_alloc(t)) return;

  TERM_DEBUG(TERM_LINES, t,
             "term_line_next: START row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  
  if(t->li.line + 1>= MAX_SAVE_LINES) {
    t->li.wrap = 1;
    t->li.line = 0;
    t->li.col = 0; 

  TERM_DEBUG(TERM_LINES, t,
             "term_line_next: WRAP row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
    
    
  } else {
    t->li.line++; 
    t->li.col = 0;

    TERM_DEBUG(TERM_LINES, t,
               "term_line_next: LINE++ row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
  }

  if(t->li.mark) {
    t->li.mark--;

    TERM_DEBUG(TERM_LINES, t,
               "term_line_next: DEC row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
    
  } else {
    t->li.watermark = t->li.line; 

    TERM_DEBUG(TERM_LINES, t,
               "term_line_next: PUSH row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
               t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);

  }

  TERM_DEBUG(TERM_LINES, t,
             "term_line_next: END row: %d, column: %d, li.line: %d, li.col: %d, li.mark: %d, li.wm: %d\n",
             t->next_row, t->next_column, t->li.line, t->li.col, t->li.mark, t->li.watermark);
}

void term_line_dec(struct term *t) {
  
  struct line *line;

  TERM_DEBUG(TERM_LINES, t,
             "term_line_dec: START row: %d, column: %d, li.line: %d, li.col: %d\n",
             t->next_row, t->next_column, t->li.line, t->li.col);
  
  if(t->li.col ==  0) return; 
  if(!term_line_alloc(t)) return; 
  
  line = t->lines + t->li.line;
  t->li.col--;
  line->buf[t->li.col] = '\0';

  TERM_DEBUG(TERM_LINES, t,
             "term_line_dec: END row: %d, column: %d, li.line: %d, li.col: %d\n",
             t->next_row, t->next_column, t->li.line, t->li.col);
  
}


void term_line_cr(struct term *t) {
  t->li.col = 0; 
  TERM_DEBUG(TERM_LINES, t,
             "term_line_cr: row: %d, column: %d, li.line: %d, li.col: %d\n",
             t->next_row, t->next_column, t->li.line, t->li.col);
}

void term_line_add(struct term *t, char c) {

  struct line *line;

  if(!term_line_alloc(t)) return; 

  line = t->lines + t->li.line;

  // TODO: handle resizing line. 
  if(t->li.col >= line->size) return; 

  line->buf[t->li.col++] = c;
  line->buf[t->li.col] = '\0'; 
  
}

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
  
  //  if(t->state != CASE_PRINT) return;

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
  
  if(t->checkpoint > 0) {
    TERM_DEBUG(TERM, t, "term_flush_A: cp: %d,  top: %d -> %d, bot: %d -> %d, cr: %d -> %d, cc: %d -> %d, ccr: %d -> %d, ccc: %d -> %d\n",
               t->checkpoint, otop, t->top, obot, t->bottom, ocr, t->cursor_row, occ, t->cursor_column, occr, t->check_row, occc, t->check_column);

    term_write_raw(t, (char *) &t->buf[0], t->checkpoint);

    // TODO: check for errors. 
    memmove(&t->buf[0], &t->buf[t->checkpoint], t->pos - t->checkpoint);
            
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
      term_line_dec(t); 
      t->state = CASE_PRINT;
      update_state_buf(t, c);
      return 1; 
      
    case 0xa: // LF
    case 0xb: // VT
      term_line_next(t);
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
      term_line_cr(t); 
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
  term_line_add(t, c); 
  return 0; 
}

int parse_CXX_Param(struct term *t, char stop, int *param) {

  int pos = last_esc(t);
  
  for(; pos < t->pos; pos++) {
    if(t->buf[pos] == stop) break;
    if(t->buf[pos] == '?') continue; 
    // probably should log this? 
    if(!(t->buf[pos] >= '0' && t->buf[pos] <= '9')) return 0;
    *param = *param * 10 + (t->buf[pos] - 48);
  }  

  return 1; 
}

void filter_CUU(struct term *t) {
  
  int param = 0;
  int saved_row;
  
  if(!parse_CXX_Param(t, 'A', &param)) return;
  
  saved_row = t->next_row; 
  t->next_row -= param;

  if(t->next_row < t->top) t->next_row = t->top; 
  if(t->next_row > t->bottom) t->next_row = t->bottom;

  // Update line. 
  term_line_set(t, saved_row - t->next_row, t->next_column); 
}

void filter_CUD(struct term *t) {
  
  int param = 0;
  int saved_row;
  
  if(!parse_CXX_Param(t, 'B', &param)) return;
  
  saved_row = t->next_row; 
  t->next_row += param;

  if(t->next_row < t->top) t->next_row = t->top; 
  if(t->next_row > t->bottom) t->next_row = t->bottom;

  // Update line. 
  term_line_set(t, saved_row - t->next_row, t->next_column); 
}

void filter_EL(struct term *t) {

  int col;
  int param = 0;
  struct line *line;
  
  if(!parse_CXX_Param(t, 'K', &param)) return;

  if(!term_line_alloc(t)) return;

  line = t->lines + t->li.line;
  
  switch(param) {
  case 0:
    line->buf[t->li.col] = '\0';
  case 1:
    for(col = 0; col < t->li.col; col++) {
      line->buf[col] = ' '; 
    }
  case 2:
    line->buf[col] = '\0'; 
  }

}


void filter_CUP(struct term *t) {

  int pos = last_esc(t);
  int row = 0;
  int col = 0;
  int *cur = &row;
  int saved_row;
  
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

  saved_row  = t->next_row; 
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

  //  term_line_set(t, row-1, col-1);
  term_line_set(t, saved_row - row, col-1); 
}

void term_filter_ED(struct term *t) {

  int row, col;
  term_write(t, "\033[6n");
  read_cursor(t->host, &row, &col);

  rewind_esc(t);

  // Only supports to terms, region erase seems to only work on xterm :/ 
  if(t->top == 1) {
    term_write(t, "\033[%d;%dH\033[1J\033[%d;%dH", t->bottom, 1, row, col);
  } else { 
    term_write(t, "\033[%d;%dH\033[0J\033[%d;%dH", t->top, 1, row, col);
  }
}

void filter(struct term *t) {

  switch(t->state) {
  case CSI_CUU_FILTER:
    filter_CUU(t);
    t->state = CASE_PRINT;
    break;
  case CSI_CUD_FILTER:
    filter_CUD(t);
    t->state = CASE_PRINT;
    break;
  case CSI_CUP_FILTER:
    filter_CUP(t);
    t->state = CASE_PRINT;
    break;
  case CASE_ED_FILTER:
    //    rewind_esc(t);
    term_filter_ED(t); 
    t->state = CASE_PRINT;
    // TODO remap, once we have proper cursor info.
  case CASE_EL_FITLER:
    filter_EL(t);
    t->state = CASE_PRINT; 
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

  memset(&t->li, 0, sizeof(t->li)); 
  
  t->lines = malloc(sizeof(struct line) * MAX_SAVE_LINES); 
  memset(t->lines, 0, sizeof(struct line) * MAX_SAVE_LINES); 

}
