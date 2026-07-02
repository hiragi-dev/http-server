#ifndef __H_HTTP
#define __H_HTTP

#include "substring.h"

typedef struct http_field {
  substring name;
  substring value;

  struct http_field *next;
} http_field;

typedef struct http_request_line {
  substring method, request_target, protocol;
} http_request_line;

typedef struct http_request {
  http_request_line request_line;
  http_field *http_fields;
  substring message_body;
} http_request;

http_request *
parse_http_request(char **);

int
parse_http_request_start_line(char **, http_request_line *);

http_field *
parse_http_field_line(char **req);

void
http_request_print(http_request);

#endif
