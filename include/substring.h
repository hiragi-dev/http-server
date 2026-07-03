#ifndef __H_SUBSTRING
#define __H_SUBSTRING

typedef struct substring {
  char *from, *to;
} substring;

void
substring_print(substring s);

#endif
