#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#define PORT "8080"

void *
get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  } else {
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }
}

bool is_crlf(char *s, size_t len) {
  if (len < 2)
    return false;

  return s[0] == '\r' && s[1] == '\n';
}

bool
eat(char **src, char *target, size_t len)
{ 
  if (strncmp(*src, target, len) == 0) {
    *src += len;
    return true;
  }

  return false;
}

enum HttpMethod {
  POST,
  GET,
};

struct HttpMethodTuple {
  char *str;
  enum HttpMethod method;
};

struct HttpMethodTuple http_methods[] = {
  { .str = "POST", .method = POST },
  { .str = "GET", .method = GET },
};

int
parse_http_method(char *target, size_t len, enum HttpMethod *res)
{
  for (int i = 0; i < sizeof(http_methods) / sizeof(struct HttpMethodTuple); i++) {
    size_t ll = strlen(http_methods[i].str);
    if (eat(&target, http_methods[i].str, strlen(http_methods[i].str))) {
	*res = http_methods[i].method;
	return 0;
    }
  }

  return -1;
}

int
parse_http_request(char *req, size_t len)
{  
  int rv;

  enum HttpMethod http_method;
  parse_http_method(req, len, &http_method);

  //  printf("parsed http method is %02d, %02d\n",int(http_method), GET);
  /* for (size_t i = 0; i < len; i++) { */
  /*   if (is_crlf(&req[i], len-i)) { */
      
  /*   } */
}

int
main(int argc, char *argv[])
{
  struct addrinfo hints, *servinfo;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;	   /* IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_flags = AI_PASSIVE;	   /* to bind, accept */

  int rv;
  rv = getaddrinfo(NULL, PORT, &hints, &servinfo);

  if (rv != 0) {
    fprintf(stderr, "getaddrinfo(): failure\n");
    return 1;
  }

  int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (sockfd == -1) {
    perror("socket(): failure");
    return 1;
  }

  int yes = 1;
  rv = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (rv == -1) {
    perror("setsocketopt(): failure");
    return 1;
  }

  rv = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
  if (rv == -1) {
    close(sockfd);
    perror("bind(): failure");
    return 1;
  }

  rv = listen(sockfd, 10);
  if (rv == -1) {
    perror("listen() failure");
    return 1;
  }

  struct sockaddr_storage client_addr;
  socklen_t sin_size;
  char client_addr_str[INET6_ADDRSTRLEN];

  char recv_buffer[256] = {0};

  printf("listening for port '%s'\n", PORT);
  printf("waiting for connections...\n");
  
  while (1) {
    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (client_fd == -1) {
      perror("accept(): failure");
      continue;
    }

    inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_addr_str, sizeof(client_addr_str));
    printf("got connection from %s\n", client_addr_str);

    int recv_len = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
    for (int i = 0; i < recv_len; i++) {
      printf("%c", recv_buffer[i]);
    }
    printf("\n");

    parse_http_request(recv_buffer, recv_len);
    
    close(client_fd);
  }

  return 0;
}
