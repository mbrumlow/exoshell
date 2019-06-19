
#ifndef EXOTERM_UTIL_H
#define EXOTERM_UTIL_H

int writefd(int fd, void *ptr, int len);
int writefds(int fd, char *s);
int writefdf(int fd, char *fmt, ...);

#endif // EXOTERM_UTIL_H
