#ifndef __H_SUBSTRING
#define __H_SUBSTRING

typedef struct substring {
  const char *from, *to;
} substring;

void
substring_print(substring s);

#endif
