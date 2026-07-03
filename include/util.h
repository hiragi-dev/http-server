#ifndef __H_UTIL
#define __H_UTIL

#include <stddef.h>

typedef struct file_entry {
  char *path;

  struct file_entry *next;
} file_entry;

typedef struct resource_entry {
  char *url;
  char *blob;
  size_t len;

  struct resource_entry *next;
} resource_entry;

file_entry *
list_files_recursive(const char *dir_path);

void
file_entry_free(file_entry *entries);

resource_entry *
resource_entry_new(char *url, char *blob, size_t len);

#endif
