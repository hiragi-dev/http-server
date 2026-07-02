#include <stddef.h>
#include <stdbool.h>

#include "parse.h"
#include "substring.h"

bool
eat_until(char **src, char d, substring *res)
{
  char *_src = *src;

  for (char *s = *src; s != '\0'; s++) {
    if (*s == d) {
      *res = (substring){ .from = _src, .to = s };
      *src = s;
      return true;
    }
  }

  *src = _src;

  return false;
 }
