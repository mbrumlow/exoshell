
#ifndef EXOTERM_TERM_H
#define EXOTERM_TERM_H

#include <stdint.h>

struct line
{
  int size;
  char *buf;
  uint64_t *sgr;
};

struct line_info
{
  int line;
  int col;
  int wrap;
  int mark;
  int watermark;
};

// Inner terminal state. 
struct term
{
  int id;
  int init;
  int state;			// state of output buffer.
  int pos;			// position in output buffer.
  int needs_wrap;
  int cursor_row, cursor_column;	// cursor position.
  int check_row, check_column;	// cursor check for debugging cursor tracking.
  int next_row, next_column;	// Where the cursor should be after flush.
  int top, bottom;		// terminal region.
  int columns;			// terminal width.
  int pty;			// terminal pty.
  int host;			// host terminal pty.
  int checkpoint;		// The last safe spot to flush to the terminal.
  char *extra;			// extra codes to send when selecting.
  uint64_t sgr;
  struct line *lines;
  struct line_info li;
  int *host_decset_low;
  int decset_low[34];
  unsigned char buf[4096];	// output buffer.

};

void read_cursor (int fd, int *row, int *col);
void term_init (struct term *t, int top, int bottom, int columns, int host,
		int pty);
void term_update (struct term *t, unsigned char *buf, int len);
void term_save (struct term *t);
void term_select (struct term *t);
void term_resize (struct term *t, int top, int bottom, int columns);
void term_send (struct term *t, const void *buf, size_t count);
void term_clear (struct term *t);
void term_set_extra (struct term *t, const char *buf);
void term_unset_extra (struct term *t);
void term_save_cursor (struct term *t);
void term_flush (struct term *t);
char *term_get_last_line (struct term *t, int n);


#endif // EXOTERM_TERM_H
