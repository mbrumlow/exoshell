
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int
writefd (int fd, void *ptr, int len)
{

  if (len == 0)
    return 0;

  int count = 0;
  unsigned char *buf = ptr;

  do
    {

      do
	{
	  count = write (fd, buf, len);
	}
      while (count < 0 && errno == EINTR);

      if (count == 0)
	break;

      if (count < 0 || count > len)
	{
	  return count;
	}

      len -= count;
      buf += count;

    }
  while (len > 0);

  return (int) (buf - (unsigned char *) ptr);

}

int
writefds (int fd, char *s)
{
  return writefd (fd, (void *) s, strlen (s));
}

int
writefdf (int fd, char *fmt, ...)
{

  int ret;
  int size = 0;
  char *p = NULL;
  va_list ap;

  va_start (ap, fmt);
  size = vsnprintf (p, size, fmt, ap);
  va_end (ap);

  if (size < 0)
    return -1;

  size++;
  p = malloc (size);
  if (p == NULL)
    return -1;

  va_start (ap, fmt);
  size = vsnprintf (p, size, fmt, ap);
  va_end (ap);

  if (size < 0)
    {
      free (p);
      return -1;
    }

  ret = writefd (fd, p, size);
  free (p);

  return ret;
}
