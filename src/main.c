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
#include <dirent.h>

#include "substring.h"
#include "parse.h"
#include "http.h"

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

bool
into_http_method(substring target, enum HttpMethod *res)
{
  for (int i = 0; i < sizeof(http_methods) / sizeof(struct HttpMethodTuple); i++) {
    if (strncmp(http_methods[i].str, target.from, strlen(http_methods[i].str)) == 0) {
      *res = http_methods[i].method;
      return true;
    }
  }

  return false;
}

const char http_response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n\
<!DOCTYPE html>\
<html lang=\"en\">\
<p>This Site is Hosted by Handwritten HTTP Server for Study.</p>\
<p>The Content is Preparing Now...</p>\
</html>\
";

void
handle_http_request(http_request *req, int client_fd)
{
  enum HttpMethod method;
  if (into_http_method(req->request_line.method, &method)) {
    switch (method) {
    case GET:
      send(client_fd, http_response, sizeof(http_response), 0);
      
      break;
    default:
      fprintf(stderr, "unsupported HTTP Method");
      break;
    }
  }
}

int
main(int argc, char *argv[])
{
  char *mounting_dir = "./";
  
  for (int i = 0; i < argc; i++) {
    if (argv[i][0] != '-')
      continue;

    if (i + 1 >= argc) {
      fprintf(stderr, "invalid usage of option '%c' \n", argv[i][1]);
      return 1;
    }
    
    switch (argv[i][1]) {
    case 'm':
      mounting_dir = argv[++i];
      break;
    default:
      fprintf(stderr, "unsupported argment '%c'", *argv[i]);
      return 1;
    }
  }

  DIR *res_dir = opendir(mounting_dir);
  if (!res_dir) {
    perror("failed to open resource directory");
    return 1;
  }

  // struct dirent *entry;
  // while ((entry = readdir(res_dir))) {
  //   printf("%s\n", entry->d_name);
  //   entry->d_type
  // }
  
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

  char recv_buffer[256*10] = {0};

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
    recv_buffer[recv_len] = '\0';
    
    char *req = recv_buffer;
    http_request *http_request = parse_http_request(&req);
    if (http_request) {
      /* http_request_print(*http_request); */
      handle_http_request(http_request, client_fd);
    }
    
    close(client_fd);
  }

  return 0;
}
