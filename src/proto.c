
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "proto.h"
#include "util.h"

uint32_t
proto_read_uint32 (int fd, uint32_t * p)
{
  int ret;

  ret = read (fd, p, sizeof (uint32_t));
  if (ret != sizeof (uint32_t))
    {
      return PROTO_ERROR;
    }
  return PROTO_OK;
}

uint32_t
proto_send_uint32 (int fd, uint32_t s)
{
  int ret;

  ret = writefd (fd, &s, sizeof (s));
  if (ret != sizeof (s))
    {
      return PROTO_ERROR;
    }
  return PROTO_OK;
}

uint32_t
proto_read_req (int fd)
{
  uint32_t req;
  if (proto_read_uint32 (fd, &req) != PROTO_OK)
    return PROTO_ERROR;
  return req;
}

uint32_t
proto_send_req (int fd, uint32_t req)
{

  int ret = writefd (fd, &req, sizeof (req));
  if (ret < sizeof (req))
    {
      return PROTO_ERROR;
    }

  return PROTO_OK;
}

uint32_t
proto_read_string (int fd, char **p)
{

  int ret;
  char *buf = NULL;
  uint32_t size;
  uint32_t req = proto_read_req (fd);

  switch (req)
    {
    case PROTO_STRING:

      size = proto_read_req (fd);

      buf = malloc (size + 1);
      if (!buf)
	{
	  return PROTO_ERROR;
	}

      ret = read (fd, buf, size);
      if (ret != size)
	{
	  return PROTO_ERROR;
	}

      buf[size] = '\0';
      *p = buf;
      return PROTO_OK;

    case PROTO_NONE:
      return PROTO_NONE;

    }

  return PROTO_ERROR;
}

uint32_t
proto_send_eof (int fd)
{
  return proto_send_req (fd, PROTO_NONE);
}

uint32_t
proto_send_string (int fd, char *s, int len)
{

  uint32_t l = (uint32_t) len;

  if (proto_send_req (fd, PROTO_STRING) != PROTO_OK)
    return PROTO_ERROR;
  if (proto_send_uint32 (fd, l) != PROTO_OK)
    return PROTO_ERROR;
  if (writefd (fd, s, len) != len)
    return PROTO_ERROR;
  return PROTO_OK;
}
