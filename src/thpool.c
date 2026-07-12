#include <stdio.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "thpool.h"

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
