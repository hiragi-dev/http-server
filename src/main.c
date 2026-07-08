#include <stdio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

struct bsem_t {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int v;
};

void
bsem_init(struct bsem_t *bsem)
{
  pthread_mutex_init(&bsem->mutex, NULL);
  pthread_cond_init(&bsem->cond, NULL);
}

void
bsem_notify(struct bsem_t *bsem)
{
  pthread_mutex_lock(&bsem->mutex);
  bsem->v = 1;
  pthread_cond_signal(&bsem->cond);
  pthread_mutex_unlock(&bsem->mutex);
}

void
bsem_wait(struct bsem_t *bsem)
{
  pthread_mutex_lock(&bsem->mutex);
  while (bsem->v == 0) {
    pthread_cond_wait(&bsem->cond, &bsem->mutex);
  }
  bsem->v = 0;
  pthread_mutex_unlock(&bsem->mutex);
}

struct client_queue_entry_t {
  int fd;

  struct client_queue_entry_t *next;
};

struct client_queue_t {
  pthread_mutex_t mutex;
  struct client_queue_entry_t *head;

  struct bsem_t *bsem;
};

int
client_queue_push(struct client_queue_t *self, int fd)
{
  pthread_mutex_lock(&self->mutex);

  struct client_queue_entry_t *new_entry =
      (struct client_queue_entry_t *)calloc(
          1, sizeof(struct client_queue_entry_t));
  if (!new_entry) {
    fprintf(stderr, "calloc(): failure\n");
    return -1;
  }
  new_entry->fd = fd;

  new_entry->next = self->head;
  self->head = new_entry;

  pthread_mutex_unlock(&self->mutex);

  return 0;
}

int
client_queue_pop(struct client_queue_t *self, int *res)
{
  pthread_mutex_lock(&self->mutex);
  
  struct client_queue_entry_t *poped = self->head;
  if (!poped) {
    fprintf(stderr, "client_queue_pop(): empty queue\n");
    return -1;
  }
  self->head = poped->next;

  *res = poped->fd;
  
  pthread_mutex_unlock(&self->mutex);

  return 0;
}

void
client_queue_init(struct client_queue_t *client_queue)
{
  client_queue->head = NULL;
  client_queue->bsem = (struct bsem_t *)calloc(1, sizeof(struct bsem_t));

  bsem_init(client_queue->bsem);
}

struct thread_pool_t {
  struct client_queue_t *client_queue;
};

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
