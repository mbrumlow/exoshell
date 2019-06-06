
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

static void term_debug(int flag, struct term *t, char *fmt, ...) {

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

static void term_write_raw(struct term *t, char *buf, int size) {
  TERM_DEBUG(TERM_WRITE, t, "WRITE(%d): '%s'\n", size, buf);
  writefd(t->host, buf, size);
}

static void term_write(struct term *t, char *fmt, ...) {

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

static struct line *term_get_line(struct term *t, int n) {

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

  
  return &t->lines[p];
}

char *term_get_last_line(struct term *t, int n) {

  struct line *line;

  line = term_get_line(t, n);
  if(!line) return NULL;

  return line->buf; 
  
}

static int term_line_alloc(struct term *t) {

  struct line *line;

  
  if(!t->lines) return 0;
  if((t->lines + t->li.line)->buf) return 1; 

  line = t->lines + t->li.line;
  line->buf = malloc(t->columns + 1);
  line->sgr = malloc(sizeof(uint64_t) * (t->columns +1)); 
  if(!line->buf) {
    free(line); 
    return 0; 
  }

  line->buf[0] = '\0';
  line->size = t->columns;
  return 1; 
}

static void term_line_set(struct term *t, int diff, int col) {
  
  int p;
  
  p = t->li.line - diff;
  
  if(!t->li.wrap && p < 0) {
    diff += p;
    p = 0;
  }

  if(t->li.wrap && p < 0) {
    p = MAX_SAVE_LINES + p;
  }
  
  // Handle forward wrap. 
  if(p >= MAX_SAVE_LINES) {
    p = p - MAX_SAVE_LINES;
    t->li.wrap = 1; 
  }
  
  if(p >= MAX_SAVE_LINES || p < 0) return;

  t->li.mark = t->li.mark - (-diff);

  if(col < 0) col = 0;
  
  t->li.line = p;
  t->li.col = col;
  
  if(t->li.mark < 0) {
    t->li.mark = 0;
    t->li.watermark = t->li.line;
  }

}

static void term_line_next(struct term *t) {

  if(!term_line_alloc(t)) return;
  
  if(t->li.line + 1>= MAX_SAVE_LINES) {
    t->li.wrap = 1;
    t->li.line = 0;
    t->li.col = 0; 
  } else {
    t->li.line++; 
    t->li.col = 0;
  }

  if(t->li.mark) {
    t->li.mark--;
  } else {
    t->li.watermark = t->li.line; 
  }

}

static void term_line_dec(struct term *t) {
  
  struct line *line;
  
  if(t->li.col ==  0) return; 
  if(!term_line_alloc(t)) return; 
  
  line = t->lines + t->li.line;
  t->li.col--;
  line->buf[t->li.col] = '\0';

}

static void term_line_cr(struct term *t) {
  t->li.col = 0; 
}

static void term_line_add(struct term *t, char c) {

  char *buf = NULL;
  uint64_t *sgr_buf = NULL;
  struct line *line;

  if(!term_line_alloc(t)) return; 

  line = t->lines + t->li.line;

  // resize if needed. 
  if(t->li.col >= line->size) {
    buf = realloc(line->buf, (line->size * 2) + 1);
    if(!buf) return; 
    line->buf = buf;

    sgr_buf = realloc(line->sgr, sizeof(uint64_t) * ((line->size * 2) + 1));
    if(!sgr_buf) return;
    line->sgr = sgr_buf; 
    
    line->size *= 2; 
  }

  line->sgr[t->li.col] = t->sgr; 
  line->buf[t->li.col++] = c;
  line->buf[t->li.col] = '\0'; 
  
}

static int last_esc(struct term *t) {
  
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
static int rewind_esc(struct term *t) {
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

static void checkpoint(struct term *t) {
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

  int i;

  // TODO restore SGR for each side. 
  if(t->host_decset_low) { 
    for(i = 0; i < 34; i++) {
      if(t->host_decset_low[i] != t->decset_low[i] ) {
        switch(i) {
        case DECSET_DECTCEM:
          if(t->decset_low[i] == 1 || t->host_decset_low[i] == 2) {
            term_write(t, "\033[?25h");
            t->decset_low[i] = 1;
          } else if (t->decset_low[i] == 2 || t->host_decset_low[i] == 1){
            term_write(t, "\033[?25l");
            t->decset_low[i] = 2;
          }
          t->host_decset_low[i] = t->decset_low[i];
          break; 
        }
      
      }
    }
  }
  
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
    term_write(t, "\033[%d;%dH\033[K\033[1J\033[%d;%dH", t->bottom, 1, row, col);
  } else { 
    term_write(t, "\033[%d;%dH\033[K\033[0J\033[%d;%dH", t->top, 1, row, col);
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

static void term_bs(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->needs_wrap = 0;
  if(t->next_column) t->next_column--; 
  
  TERM_DEBUG(CURSOR, t, "term_cr: %d:%d:%d -> %d:%d:%d\n",
             row,col,wrap, t->next_row, t->next_column, t->needs_wrap);
}

static void term_cr(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->needs_wrap = 0;
  t->next_column = 1; 
  
  TERM_DEBUG(CURSOR, t, "term_cr: %d:%d:%d -> %d:%d:%d\n",
             row, col, wrap, t->next_row, t->next_column, t->needs_wrap);
}

static void term_ff(struct term *t) {
  int row = t->next_row;
  int col = t->next_column;
  int wrap = t->needs_wrap;

  t->needs_wrap = 0;
  t->next_row = t->top;
  t->next_column = 1;
  
  TERM_DEBUG(CURSOR, t, "term_ff: %d:%d:%d -> %d:%d:%d\n",
             row,col,wrap, t->next_row, t->next_column, t->needs_wrap);
}

static void term_lfvt(struct term *t) {
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

static void term_cursor_wrap(struct term *t) {

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

static void term_cursor_next(struct term *t) {

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
  
  if(t->checkpoint > 0) {

    term_write_raw(t, (char *) &t->buf[0], t->checkpoint);

    // TODO: check for errors. 
    memmove(&t->buf[0], &t->buf[t->checkpoint], t->pos - t->checkpoint);
            
    t->pos = t->pos - t->checkpoint;
    t->checkpoint = 0;
    t->check_row = t->next_row;
    t->check_column = t->next_column;

  }
}

static void update_state_buf(struct term *t, unsigned char c) {
  t->buf[t->pos++] = c;
  t->buf[t->pos] = '\0'; 
}

static inline int esc_check(struct term *t, int state) {
  if(t->state != state) return 1;
  return 0; 
}

static int update(struct term *t, unsigned char c) {

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

static int parse_CXX_Param(struct term *t, char stop, int *param) {

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

static void filter_CUU(struct term *t) {
  
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

static void filter_CUD(struct term *t) {
  
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

static void filter_EL(struct term *t) {

  int col;
  int param = 0;
  struct line *line;
  
  if(!parse_CXX_Param(t, 'K', &param)) return;

  if(!term_line_alloc(t)) return;

  line = t->lines + t->li.line;
  
  switch(param) {
  case 0:
    line->buf[t->next_column] = '\0';
  case 1:
    for(col = 0; col < t->li.col; col++) {
      line->buf[col] = ' '; 
    }
  case 2:
    line->buf[t->next_column] = '\0'; 
  }

}

static void filter_CUP(struct term *t) {

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

static void term_filter_ED(struct term *t) {

  int row, col;
  term_write(t, "\033[6n");
  read_cursor(t->host, &row, &col);

  rewind_esc(t);

  // Only supports two terms, region erase seems to only work on xterm :/ 
  if(t->top == 1) {
    term_write(t, "\033[%d;%dH\033[1J\033[%d;%dH", t->bottom, 1, row, col);
  } else { 
    term_write(t, "\033[%d;%dH\033[0J\033[%d;%dH", t->top, 1, row, col);
  }
}

static void term_filter_DECSET(struct term *t) {

  int pos = last_esc(t);
  int reset = 0; 
  int num = 0;
  int map = 0;

  TERM_DEBUG(TERM_DECSET, t, "term_filter_DECSET: start\n");
  
  // CSI ? P m h
  // CSI ? P m l
  pos++; // moves to -> or first parameter. 
  for(; pos < t->pos; pos++) {

    if(t->buf[pos] == '[') continue;
    if(t->buf[pos] == '?') continue;
    if(t->buf[pos] == 'h') break;
    if(t->buf[pos] == 'l') {
      reset = 1;
      break;
    }
    
    // probably should log this? 
    if(!(t->buf[pos] >= '0' && t->buf[pos] <= '9')) return;
    
    num = num * 10 + (t->buf[pos] - 48);
  }

  TERM_DEBUG(TERM_DECSET, t, "num: %d\n", num);
  
  if(num >= DECSET_MAX) return;
  map = decset_low_table[num]; 

  if(reset) {
    TERM_DEBUG(TERM_DECSET, t, "DECRSET: %d\n", map);
    t->decset_low[map] = 2;
    t->host_decset_low[map] = 2;
  } else {
    TERM_DEBUG(TERM_DECSET, t, "DECSET: %d\n", map);
    t->decset_low[map] = 1;
    t->host_decset_low[map] = 1;
  }
  
}

static uint8_t term_sgr_read(struct term *t, int *pos) {

  int _pos = *pos; 
  uint8_t num = 0;
  
  for(; _pos < t->pos; _pos++) {
    TERM_DEBUG(TERM_SGR, t, "term_sgr_read: pos: %d, max: %d  c: '%c'\n",
               _pos, t->pos, t->buf[_pos]);

    if(t->buf[_pos] == '[') continue;
    if(t->buf[_pos] == 'm') break;
    if(t->buf[_pos] == ';') break; 
    if(!(t->buf[_pos] >= '0' && t->buf[_pos] <= '9')) break;
    num = num * 10 + (t->buf[_pos] - 48);
  }

  _pos++; 
  *pos = _pos; 
  return num;
}

static void term_set_state_color(struct term *t,
                          uint8_t ps1, uint8_t ps2,
                          uint8_t ps3, uint8_t ps4, uint8_t ps5) {

  uint64_t sgr_old = t->sgr;
  uint8_t *sgr = (uint8_t *) &t->sgr;
  
  TERM_DEBUG(TERM_SGR, t, "term_set_state_color: ps1: %d, ps2: %d, ps3: %d, ps4: %d; ps5: %d\n",
             ps1, ps2, ps3, ps4, ps5); 

  if(ps1 == 38) {

    sgr[7] = ps3;
    sgr[6] = ps4;
    sgr[5] = ps5;

    sgr[1] &= ~ 0x3;
    if(ps2 == 5) {
      sgr[1] |= 0x1;
      TERM_DEBUG(TERM_SGR, t, "term_set_state_color: fg(256): %d\n", ps4); 
    } else if (ps2 == 2) {
      sgr[1] |= 0x3;
      TERM_DEBUG(TERM_SGR, t, "term_set_state_color: fg(24bit): %d,%d,%d\n", ps3, ps4, ps5); 
    }

    goto out; 
  }

  if(ps1 == 48) {

    sgr[4] = ps3;
    sgr[3] = ps4;
    sgr[2] = ps5;

    sgr[1] &= ~ (0x3 << 4);
    if(ps2 == 5) {
      sgr[1] |= (0x1 << 4);
      TERM_DEBUG(TERM_SGR, t, "term_set_state_color: bg(256): %d\n", ps4); 
    } else if (ps2 == 2) {
      sgr[1] |= (0x3 << 4);
      TERM_DEBUG(TERM_SGR, t, "term_set_state_color: bg(24bit): %d,%d,%d\n", ps3, ps4, ps5); 
    }

    goto out; 
  }
  
  // 8 and 16 color fg.
  if((ps1 >= 30 && ps1 <= 38) || (ps1 >=90 && ps1 <= 97)) {
    sgr[7] = ps1;
    sgr[6] = 0;
    sgr[5] = 0;
    sgr[1] &= ~ 0x3;
    TERM_DEBUG(TERM_SGR, t, "term_set_state_color: fg(8/16): %d\n", ps1); 
    goto out; 
  }

  if((ps1 >= 40 && ps1 <= 49) || (ps1 >=100 && ps1 <= 107)) {
    sgr[4] = ps1;
    sgr[3] = 0;
    sgr[2] = 0;
    sgr[1] &= ~ (0x3 << 4);
    TERM_DEBUG(TERM_SGR, t, "term_set_state_color: bg(8/16): %d\n", ps1); 
    goto out; 
  }
  
 out:
  TERM_DEBUG(TERM_SGR, t, "term_set_state_color: 0x%lx -> 0x%lx\n", sgr_old, t->sgr); 
}

static void term_filter_SGR(struct term *t) {
  
  int pos = last_esc(t) + 1;
  uint8_t ps1 = 0;
  uint8_t ps2 = 0;
  uint8_t ps3 = 0;
  uint8_t ps4 = 0;
  uint8_t ps5 = 0;
 
  /* 8   7   6   5   4   3   2   1          */
  /* |fgr|fgb|fgb|bgr|bgb|bgg|   |II BU  B| */
  /*                              76543210  */
  
  // CSI P m m
  do {

    ps1 = term_sgr_read(t, &pos);
    
    TERM_DEBUG(TERM_SGR, t, "term_filter_SGR: ps1: %d, pos: %d, max: %d\n",
               ps1, pos, t->pos);
    
    if(ps1 == 0) {
      TERM_DEBUG(TERM_SGR, t, "term_filter_SGR: (reset) ps1: %d, pos: %d, max: %d\n",
                 ps1, pos, t->pos);
      t->sgr = 0;
      continue;
    } else if (ps1 <= 8) {
      TERM_DEBUG(TERM_SGR, t, "term_filter_SGR: (set:%d) ps1: %d, pos: %d, max: %d\n",
                 ps1 - 1, ps1, pos, t->pos);
      t->sgr |= 1 << (ps1 - 1);
      continue;
    } else if (ps1 <= 28) {
      TERM_DEBUG(TERM_SGR, t, "term_filter_SGR: (unset:%d) ps1: %d, pos: %d, max: %d\n",
                 ps1 - 21, ps1, pos, t->pos);
      t->sgr &= ~(ps1 - 21);
      continue; 
    }
    
    // 255/24bit color cases. 
    if(ps1 == 38 || ps1 == 48) {
      ps2 = term_sgr_read(t, &pos);
      ps3 = term_sgr_read(t, &pos); // 256 color or red. 

      if ( ps2 == 2 ) {
        ps4 = term_sgr_read(t, &pos); // blue
        ps5 = term_sgr_read(t, &pos); // green
      }
    } else {
      ps2 = 0;
      ps3 = 0;
      ps4 = 0;
      ps5 = 0; 
    }

    term_set_state_color(t, ps1, ps2, ps3, ps4, ps5);
  } while (pos < t->pos); 
  
}

static void filter(struct term *t) {

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
    term_filter_ED(t); 
    t->state = CASE_PRINT;
    // TODO remap, once we have proper cursor info.
    break;
  case CASE_EL_FITLER:
    filter_EL(t);
    t->state = CASE_PRINT;
    break;
  case CASE_DECSET_FILTER:
    term_filter_DECSET(t);
    t->state = CASE_PRINT;
    break;
  case CASE_SGR_FILTER:
    term_filter_SGR(t);
    t->state = CASE_PRINT;
    break;
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

/* static void term_updatef(struct term *t, char *fmt, ...) { */

/*   int size = 0;  */
/*   char *p = NULL;  */
/*   va_list ap; */
  
/*   va_start(ap, fmt); */
/*   size = vsnprintf(p, size, fmt, ap); */
/*   va_end(ap); */

/*   if(size < 0) */
/*     return; */

/*   size++; */
/*   p = malloc(size); */
/*   if (p == NULL) */
/*     return; */

/*   va_start(ap, fmt); */
/*   size = vsnprintf(p, size, fmt, ap); */
/*   va_end(ap); */

/*   if(size < 0){ */
/*     free(p); */
/*     return; */
/*   } */

/*   term_update(t, (unsigned char *) p, size);  */
/*   free(p);  */
/* } */

static void term_draw_sgr(struct term *t, uint64_t new, uint64_t old) {

  uint8_t attr_new, attr_old;
  uint8_t *o, *n;

  term_debug(TERM_SGR, t, "term_draw_sgr: 0x%lx\n", new); 
  
  // No change at all!
  if(new == old) return; 

  attr_new = (uint8_t)(new & 0x00000000000000FF);
  attr_old = (uint8_t)(old & 0x00000000000000FF);

  if(0 && attr_new != attr_old) { 
    // Suggestions on how to make this better? 
    if(attr_new & (1 << 0) && !(attr_old & (1 << 0))) {
      term_write(t, "\033[1m"); 
    }
    if(!(attr_new & (1 << 0)) && (attr_old & (1 << 0))) {
      term_write(t, "\033[22m"); 
    }
  
    if(attr_new & (1 << 3) && !(attr_old & (1 << 3))) {
      term_write(t, "\033[4m"); 
    }
    if(!(attr_new & (1 << 3)) && (attr_old & (1 << 3))) {
      term_write(t, "\033[24m"); 
    }
  
    if(attr_new & (1 << 4) && !(attr_old & (1 << 4))) {
      term_write(t, "\033[5m"); 
    }
    if(!(attr_new & (1 << 4)) && (attr_old & (1 << 4))) {
      term_write(t, "\033[25m"); 
    }
  
    if(attr_new & (1 << 6) && !(attr_old & (1 << 6))) {
      term_write(t, "\033[7m"); 
    }
    if(!(attr_new & (1 << 6)) && (attr_old & (1 << 6))) {
      term_write(t, "\033[27m"); 
    }
  
    if(attr_new & (1 << 7) && !(attr_old & (1 << 7))) {
      term_write(t, "\033[8m"); 
    }
    if(!(attr_new & (1 << 7)) && (attr_old & (1 << 7))) {
      term_write(t, "\033[28m"); 
    }
  }


  // Color did not change. 
  if(new >> 8 == old >> 8) return;

  o = (uint8_t *)&old;
  n = (uint8_t *)&new;
  
  if(!(n[7] == o[7]) ||
     !(n[6] == o[6]) ||
     !(n[5] == o[6]) ||
     !((n[1] & 0x3) == (o[1] & 0x3))) {
    // FG color changed.

    switch(n[1]) {
    case 0x0:
      term_write(t, "\033[%dm", n[7]); 
      break;

    case 0x1:
      term_write(t, "\033[38;5;%dm", n[7]); 
      break;

    case 0x3:
      term_write(t, "\033[38;2;%d;%d;%dm", n[7], n[6], n[5]); 
      break;
    }
    
  }

  if(!(n[4] == o[4]) ||
     !(n[3] == o[3]) ||
     !(n[2] == o[2]) ||
     !(((n[1] >> 4) & 0x3) == ((o[1] >> 4) & 0x3))) {
    // BG color changed.

    switch(n[1] >> 4) {
    case 0x0:
      term_write(t, "\033[%dm", n[7]); 
      break;

    case 0x1:
      term_write(t, "\033[48;5;%dm", n[7]); 
      break;

    case 0x3:
      term_write(t, "\033[48;2;%d;%d;%dm", n[7], n[6], n[5]); 
      break;
    }
    
  }

}

static int term_draw_line(struct term *t, int n) {

  int i; 
  uint64_t last = 0;
  struct line *line; 
  
  line = term_get_line(t, n);
  if(!line) return 0;

  if(t->extra){
    write(t->host, t->extra, strlen(t->extra));
    term_write(t, "\033[2K"); 
  }  else {
    term_write(t, "\033[0m\033[2K"); 
  }

  for(i = 0; i < line->size; i++) {
    if(line->buf[i] == '\0') break;
    term_draw_sgr(t, line->sgr[i], last);
    term_write(t, "%c", line->buf[i]);
    last = line->sgr[i]; 
  }

  return 1; 
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

  // Hack for redraw on resize. Need to make this more generic. 
  if(t->top == 1) { 
    term_write(t, "\033[%d;%dH\033[K\033[1J\033[%d;%dH\033[%d;%dr\033[?1;9l\033[%d;%dH",
               t->bottom, 1, t->top, 1, t->top, t->bottom, t->top, 1);
  } else {
    term_write(t, "\033[%d;%dH\033[K\033[0J\033[%d;%dH\033[%d;%dr\033[?1;9l\033[%d;%dH",
               t->top, 1, t->top, 1, t->top, t->bottom, t->top, 1);
  }
  
  int n = (t->bottom - t->top);
  for(int i = 0; i <= n; i++) {
    if(term_draw_line(t, n-i)) {
      if(i + 1 <= n) term_write(t, "\r\n");
    }
  }

  term_save(t);

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
  t->host_decset_low = NULL;

  TERM_DEBUG(TERM, t, "term_init: top: %d, bot: %d, cr: %d, cc: %d\n",
             t->top, t->bottom, t->cursor_row,t->cursor_column);

  
  memset(&wbuf, 0, sizeof(wbuf));
  wbuf.ws_row = bottom - top + 1;
  wbuf.ws_col = columns;
  ioctl(pty, TIOCSWINSZ, &wbuf);
  
  memset(&t->li, 0, sizeof(t->li)); 
  memset(&t->decset_low, 0, sizeof(t->decset_low));
  
  t->lines = malloc(sizeof(struct line) * MAX_SAVE_LINES); 
  memset(t->lines, 0, sizeof(struct line) * MAX_SAVE_LINES); 

}
