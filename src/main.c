#include <stdio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "thpool.h"

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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "substring.h"
#include "parse.h"
#include "http.h"
#include "util.h"

#define PORT "8080"

static void *
thread_loop(void *arg);

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
<p>This Site is Hosted by HandWritten HTTP Server for Study.</p>\
<p>The Content is Preparing Now...</p>\
</html>\
";

const char http_response_ok[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

const char http_response_404[] =
    "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n\
<!doctype html>\
<head>\
    <title>404 not found</title>\
</head>\
<html lang=\"en\">\
<h>404 Not Found</h>\
<p>Oops! Something went wrong!</p>\
</html>\
";

resource_entry *resources = NULL;

void
handle_http_request(http_request *req, int client_fd)
{
  enum HttpMethod method;

  if (into_http_method(req->request_line.method, &method)) {
    switch (method) {
    case GET:
      resource_entry *target = resource_entry_find_with_url(resources, req->request_line.request_target.from);
      if (target) {
        send(client_fd, http_response_ok, sizeof http_response_ok, 0);
        send(client_fd, target->blob, target->len, 0);
      } else {
        send(client_fd, http_response_404, sizeof http_response_404, 0);
      }

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

  file_entry *files = list_files_recursive(mounting_dir);

  for (file_entry *file = files; file; file = file->next) {
    char *url = file->path;
    while (url[0] != '/')
      url++;

    int fd = open(file->path, O_RDONLY);
    if (fd < 0) {
      perror("failed to open file");
      return 1;
    }

    struct stat finfo;
    if (fstat(fd, &finfo) == -1) {
      perror("failed to stat");
      return 1;
    }

    size_t file_len = finfo.st_size;
    char *buf = (char *)malloc(sizeof(char) * file_len);
    read(fd, buf, file_len);

    resource_entry *new_entry = resource_entry_new(url, buf, file_len);
    new_entry->next = resources;
    resources = new_entry;
  }

  // for (resource_entry *entry = resources; entry; entry = entry->next) {
  //   printf("%s -> '%.*s\n'", entry->url, entry->len, entry->blob);
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
  socklen_t sin_size = sizeof(client_addr);
  char client_addr_str[INET6_ADDRSTRLEN+1];

  char recv_buffer[256*10] = {0};

  printf("listening for port '%s'\n", PORT);
  printf("waiting for connections...\n");

  int n_threads = 4;

  pthread_t *threads = (pthread_t *)calloc(n_threads, sizeof(pthread_t));
  if (!threads) {
    fprintf(stderr, "calloc(): failure");
    return 1;
  }

  struct client_queue_t client_queue;
  client_queue_init(&client_queue);
  struct thread_pool_t tp = { .client_queue = &client_queue };

  for (int i = 0; i < n_threads; i++) {
    int rv = pthread_create(&threads[i], NULL, thread_loop, (void *)&tp);
    if (rv != 0) {
      fprintf(stderr, "pthread_create(): failure");
      return 1;
    }
  }

  while (1) {
    int client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (client_fd == -1) {
      perror("accept(): failure");
      continue;
    }

    inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr), client_addr_str, sizeof(client_addr_str));
    printf("got connection from %s\n", client_addr_str);

    client_queue_push(tp.client_queue, client_fd);
    bsem_notify(tp.client_queue->bsem);

  }

  return 0;
}

static void *
thread_loop(void *arg)
{
  struct thread_pool_t *tp = (struct thread_pool_t *)arg;
  char recv_buffer[256*10] = {0};

  while (1) {
    printf("wait for cond: thread %ld\n", (unsigned long)pthread_self());

    bsem_wait(tp->client_queue->bsem);

    printf("get bsem\n");
    int client_fd = 0;
    int rv = client_queue_pop(tp->client_queue, &client_fd);

    printf("poped %d\n", client_fd);
    printf("get cond: thread %ld\n", (unsigned long)pthread_self());

    int recv_len = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
    recv_buffer[recv_len] = '\0';
    char *req = recv_buffer;
    http_request *http_request = parse_http_request(&req);
    if (http_request) {
      //http_request_print(*http_request);
      handle_http_request(http_request, client_fd);
    }

    close(client_fd);
  }
  
  return NULL;
}
