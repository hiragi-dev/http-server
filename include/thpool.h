#ifndef __H_THPOOL
#define __H_THPOOL

struct bsem_t {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int v;
};


struct client_queue_entry_t {
  int fd;

  struct client_queue_entry_t *next;
};

struct client_queue_t {
  pthread_mutex_t mutex;
  struct client_queue_entry_t *head;

  struct bsem_t *bsem;
};

struct thread_pool_t {
  struct client_queue_t *client_queue;
};

void bsem_init(struct bsem_t *bsem);
void bsem_notify(struct bsem_t *bsem);
void bsem_wait(struct bsem_t *bsem);
int client_queue_push(struct client_queue_t *self, int fd);
int client_queue_pop(struct client_queue_t *self, int *res);
void client_queue_init(struct client_queue_t *client_queue);

#endif
