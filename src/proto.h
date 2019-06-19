
#ifndef EXOTERM_PROTO_H
#define EXOTERM_PROTO_H

#include <stdint.h>

#define PROTO_OK    0x000000
#define PROTO_ERROR 0x000001

// One shot operations, requiring no reply.
#define PROTO_SHELL_START 0x000100
#define PROTO_SHELL_END   0x000101

// Request for data, expecting multi byte replies that can be interrupted.
#define PROTO_REQ_LINE 0x001000

// Types
#define PROTO_NONE   0x001000
#define PROTO_STRING 0x001001

struct message
{
  uint32_t type;
  uint32_t size;
  unsigned char buf[512];
};

uint32_t proto_send_uint32 (int fd, uint32_t s);
uint32_t proto_read_uint32 (int fd, uint32_t * p);
uint32_t proto_send_eof (int fd);
uint32_t proto_read_req (int fd);
uint32_t proto_send_req (int fd, uint32_t req);
uint32_t proto_send_string (int fd, char *s, int len);
uint32_t proto_read_string (int fd, char **p);

#endif //EXOTERM_PROTO_H
