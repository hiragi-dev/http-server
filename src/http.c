#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "http.h"
#include "parse.h"
#include "substring.h"

http_request *
parse_http_request(char **req)
{
  http_request_line request_line;
  if (parse_http_request_start_line(req, &request_line) != 0) {
    fprintf(stderr, "failed to parse http request start line");
    return NULL;
  }

  /* skip crlf */
  *req += 2;

  /* field */
  http_field *http_fields = NULL;
  
  while (!is_crlf(*req, strlen(*req))) {
    http_field *http_field = parse_http_field_line(req);
    if (http_field == NULL) {
      fprintf(stderr, "failed to parse http field line");
      /* free http_fields ? */
      return NULL;
    }

    http_field->next = http_fields;
    http_fields = http_field;
  }

  /* skip crlf */
  *req += 2;

  /* message-body */
  substring message_body;
  message_body.from = *req;
  
  while (1) {
    if (**req == '\0') {
      message_body.to = *req;
      break;
    }

    *req += 1;
  }

  /* no necessary for heap allocation */
  http_request *http_req = (http_request *)malloc(sizeof(http_request));
  http_req->request_line = request_line;
  http_req->http_fields = http_fields;
  http_req->message_body = message_body;

  return http_req;
}

int
parse_http_request_start_line(char **req, http_request_line *res)
{
  /* method */
  substring req_method;
  if(!eat_until(req, ' ', &req_method)) {
    fprintf(stderr, "failed to parse http request method");
    return -1;
  }
  skip_whitespace(req);

  /* request-target */
  substring req_target;
  if(!eat_until(req, ' ', &req_target)) {
    fprintf(stderr, "failed to parse http request target");
    return -1;
  }
  skip_whitespace(req);

  /* protocol */
  substring req_protocol;
  if (!eat_until(req, '\r', &req_protocol)) {
    fprintf(stderr, "failed to parse http protocol");
    return -1;
  }
  skip_whitespace(req);

  /* substring_print(req_method); */
  /* substring_print(req_target); */
  /* substring_print(req_protocol); */

  res->method = req_method;
  res->request_target = req_target;
  res->protocol = req_protocol;

  return 0;
}

http_field *
parse_http_field_line(char **req)
{
  substring field_name;
  if (!eat_until(req, ':', &field_name)) {
    fprintf(stderr, "failed to parse filed name");
    return NULL;
  }

  skip_c(req, ':');

  substring field_value;
  if (!eat_until(req, '\r', &field_value)) {
    fprintf(stderr, "failed to parse filed value");
    return NULL;
  }

  /* skip crlf */
  *req += 2;

  /* substring_print(field_name); */
  /* substring_print(field_value); */

  http_field *field = (http_field *)malloc(sizeof(http_field));

  field->name = field_name;
  field->value = field_value;
  field->next = NULL;
  
  return field;
}

void
http_request_print(http_request target)
{
  printf("HTTP Request:\n");
  
  printf("\tmethod:\t");          substring_print(target.request_line.method);
  printf("\trequest-target:\t");  substring_print(target.request_line.request_target);
  printf("\tprotocol:\t");        substring_print(target.request_line.protocol);

  printf("\tHedaers:\n");
  for (http_field *h = target.http_fields; h; h = h->next) {
    printf("\t\t%.*s:\t%.*s\n", (int)((unsigned long)(h->name.to) - (unsigned long)(h->name.from)), h->name.from, (int)((unsigned long)(h->value.to) - (unsigned long)(h->value.from)), h->value.from);
  }
}
