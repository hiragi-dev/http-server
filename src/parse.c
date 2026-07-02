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


void
skip_whitespace(char **s)
{
  skip_c(s, ' ');
}

void
skip_c(char **s, char c)
{
  while (**s == c) {
    if (**s != c || **s == '\0')
      return;
    
    *s += 1;
  }
}


bool
is_crlf(char *s, size_t len) {
  if (len < 2)
    return false;

  return s[0] == '\r' && s[1] == '\n';
}
