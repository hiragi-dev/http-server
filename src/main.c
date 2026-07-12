#include <stdio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "thpool.h"


static void *
thread_loop(void *arg)
{
  struct thread_pool_t *tp = (struct thread_pool_t *)arg;

  while (1) {
    printf("wait for cond: thread %ld\n", (unsigned long)pthread_self());

    bsem_wait(tp->client_queue->bsem);
    int fd = 0;
    int rv = client_queue_pop(tp->client_queue, &fd);

    printf("poped %d\n", fd);

    printf("get cond: thread %ld\n", (unsigned long)pthread_self());

  }
  
  return NULL;
}

int
main(int argc, char *argv[])
{
  int n_threads = 1;
  if (argc > 1) 
    n_threads = atoi(argv[1]);

  pthread_t *threads = (pthread_t *)calloc(n_threads, sizeof(pthread_t));
  if (!threads) {
    fprintf(stderr, "calloc(): failure");
    return 1;
  }

  // struct bsem_t bsem;
  // bsem_init(&bsem);
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
  
  sleep(1);

  for (int i = 0; i < 4; i++) {
    client_queue_push(tp.client_queue, i);
    bsem_notify(tp.client_queue->bsem);
    sleep(1);
  }

  sleep(5);

  return 0;
}
