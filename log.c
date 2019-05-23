
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

#include "log.h"

static int stdlog = -1; 
static uint64_t flags = 0;

int log_set_flag(char *flag, int len) {

  if(strncmp(flag, "escape", len) == 0){
    debug(INFO, "+debug: escape\n");
    flags |= ESCAPE;
  }
  
  if(strncmp(flag, "update", len) == 0){
    debug(INFO, "+debug: update\n");
    flags |= UPDATE;
  }
  
  if(strncmp(flag, "cursor", len) == 0){
    debug(INFO, "+debug: cursor\n");
    flags |= CURSOR;
  }

  if(strncmp(flag, "term_write", len) == 0){
    debug(INFO, "+debug: term_write\n");
    flags |= TERM_WRITE;
  }
    
  return 1; 
}

int log_flags(char *flags) {

  int len;
  char *cur = flags; 
  char *next = cur;

  for(;next && *cur != '\0'; cur = next + 1){

    next = strchr(cur, ',');
    if(next) 
      len = next - cur;
    else
      len = strlen(cur); 

    log_set_flag(cur, len);
    
  }
  
  return 1; 
}

int log_open(char *path) {
  int fd = open(path, O_CREAT|O_APPEND|O_WRONLY);
  if(fd < 0){
    return 0; 
  }

  stdlog = fd;
  flags |= INFO;
  return 1; 
}

int log_init(int argc, char **argv) {

  int i;

  for(i = 0; i < argc; i++) {

    // Handle log file option.
    if(strcmp(argv[i], "-log") == 0){
      if(i+1 < argc) {
        if(!log_open(argv[++i])) {
          fprintf(stderr, "%s: failed to open log with error %d\n", argv[0], errno);
          return 0; 
        }
      } else {
        fprintf(stderr, "%s: -log must be followed by a path\n", argv[0]);
        return 0; 
      }
    }

    if(strcmp(argv[i], "-debug") == 0) {
      if(i+1 < argc){
        if(!log_flags(argv[++i])){
          fprintf(stderr, "%s: failed to process debug flags\n", argv[0]);
          return 0; 
        }
      } else {
        fprintf(stderr, "%s: -log must be followed comma separated list of flags\n", argv[0]);
        return 0; 
      }
    }
    
  }

  return 1; 
}

void debug(uint64_t flag, const char *fmt, ...) {

  int i, j;
  int size = 0;
  int extra = 0;
  char *p = NULL;
  char *p2 = NULL;
  va_list ap;

  if(stdlog < 0) return; 
  if(!(flags & flag)) return; 
  
  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if(size < 0)
    return;

  size++;
  p = malloc(size);
  if(p == NULL)
    return;

  va_start(ap, fmt);
  size = vsnprintf(p, size, fmt, ap);
  va_end(ap);

  if(size < 0) {
    free(p);
    return; 
  }

  // count how much extra space we need for translating escape codes.
  for(i = 0; i < size; i++) {
    if(p[i] < 32) extra += 4;
  }

  p2 = malloc(size+extra);
  if(p2 == NULL){
    free(p);
    return; 
  }

  j = 0; 
  for(i = 0; i < size; i++) {

    if(i+1 == size && p[i] == '\n') {
      p2[j++] = '\n';
      p2[j++] = '\0';
      continue;
    }

    switch(p[i]){
    case 0x7: // Bell
      j += sprintf(&p2[j], "\\a");
      break;
    case 0x8: // Backspace
      j += sprintf(&p2[j], "\\b");
      break;
    case 0x1b: // Escape
      j += sprintf(&p2[j], "\\e");
      break;
    case 0x0c: // ff
      j += sprintf(&p2[j], "\\f");
      break;
    case 0x0a: // Newline
      j += sprintf(&p2[j], "\\n");
      break;
    case 0x0d: // CR
      j += sprintf(&p2[j], "\\r");
      break;
    case 0x09: // HT
      j += sprintf(&p2[j], "\\t");
      break;
    case 0x0b: // VT
      j += sprintf(&p2[j], "\\v");
      break;
    case 0x5c: // VT
      j += sprintf(&p2[j], "%s", "\\\\");      
      break;
    default:
      if(p[i] < 32 || p[i] == 0x7f) { 
        j += sprintf(&p2[j], "\\%03o", p[i]);
      } else {
        p2[j++] = p[i]; 
      }
    }
  }

  dprintf(stdlog, "%s", p2);

  free(p);
  free(p2);
  
}
