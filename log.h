
#ifndef EXOTERM_LOG_H
#define EXOTERM_LOG_H

#include <stdint.h>

#define INFO       0x000000
#define FIXME      0x000001
#define TERM       0x000002
#define UPDATE     0x000004
#define ESCAPE     0x000008
#define CURSOR     0x000010
#define TERM_WRITE 0x000020

#define LOG(fmt, arg...) debug(INFO, fmt, ##arg);
#define TRACE(flag, fmt, arg...) debug(, fmt, ##arg);

int log_init(int argc, char **argv);
void debug(uint64_t tflag, const char *format, ...); 

#endif //EXOTERM_LOG_H
