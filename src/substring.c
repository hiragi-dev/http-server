#include <stdio.h>

#include "substring.h"

void
substring_print(substring s)
{
  printf("substring_print\n");
  
  for (char *c = s.from; c != s.to; c++)
    printf("%c", *c);

  printf("\n");
}
