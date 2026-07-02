#ifndef __H_LSTRING
#define __H_LSTRING

#include <stddef.h>

typedef struct lstring {
  const char *data;
} lstring;

typedef struct lstring_slice {
  const char *from, *to;
} lstring_slice;

#endif
