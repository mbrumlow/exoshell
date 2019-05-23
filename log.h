
#ifndef EXOTERM_LOG_H
#define EXOTERM_LOG_H

#include <stdint.h>

#define INFO 1
#define UPDATE 2
#define ESCAPE 4
#define CURSOR 8
#define TERM_WRITE 16

#define LOG(fmt, arg...) debug(INFO, fmt, ##arg);
#define TRACE(flag, fmt, arg...) debug(, fmt, ##arg);

int log_init(int argc, char **argv);
void debug(uint64_t tflag, const char *format, ...); 

#endif //EXOTERM_LOG_H
