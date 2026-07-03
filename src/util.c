#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

#include "util.h"

file_entry *
list_files_recursive(const char *dir_path)
{
  DIR *dir = opendir(dir_path);
  if (!dir) {
    perror("failed to open directory");
    return NULL;
  }

  file_entry *files = NULL;

  struct dirent *entry;
  while ((entry = readdir(dir))) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

    struct stat st;
    if (stat(path, &st) != 0) {
      perror("failed to stat file");
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      file_entry *sub_files = list_files_recursive(path);

      file_entry *last = sub_files;
      while (last && last->next)
        last = last->next;

      if (last) {
        last->next = files;
        files = sub_files;
      }
    } else {
      file_entry *file = (file_entry *)malloc(sizeof(file_entry));
      file->path = strdup(path);
      file->next = files;
      files = file;
    }
  }

  closedir(dir);

  return files;
}

void
file_entry_free(file_entry *entries)
{
  while (entries) {
    file_entry *next = entries->next;
    free(entries->path);
    free(entries);
    entries = next;
  }
}


resource_entry *resource_entry_new(char *url, char *blob, size_t len)
{
  resource_entry *entry = (resource_entry *)malloc(sizeof(resource_entry));

  entry->url = url;
  entry->blob = blob;
  entry->len = len;
  entry->next = NULL;

  return entry;
}

resource_entry *
resource_entry_find_with_url(resource_entry *head, char *url)
{
  for (resource_entry *res = head; res; res = res->next) {
    if (strncmp(res->url, url, strlen(res->url)) == 0)
      return res;
  }

  return NULL;
}