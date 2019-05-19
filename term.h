
#ifndef EXOTERM_TERM_H
#define EXOTERM_TERM_H

// Inner terminal state. 
struct term {
  int state;                     // state of output buffer.
  int pos;                       // position in output buffer.
  int cursor_row, cursor_column; // cursor position.
  int top, bottom;               // terminal region.
  int columns;                   // terminal width.
  int pty;                       // terminal pty.
  int host;                      // host terminal pty. 
  char *extra;                   // extra codes to send when selecting. 
  unsigned char buf[4096];       // output buffer. 
};

void term_init(struct term *t, int top, int bottom, int columns, int host, int pty);
void term_update(struct term *t, unsigned char *buf, int len); 
void term_save(struct term *t); 
void term_select(struct term *t);
void term_resize(struct term *t, int top, int bottom, int columns);
void term_write(struct term *t, const void *buf, size_t count);
void term_clear(struct term *t);
void term_set_extra(struct term *t, const char *buf);
void term_unset_extra(struct term *t); 
  
#endif // EXOTERM_TERM_H
