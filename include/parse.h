#ifndef __H_PARSE
#define __H_PARSE

#include <stddef.h>
#include <stdbool.h>

#include "substring.h"

bool
eat_until(char **, char, substring *);

void
skip_whitespace(char **);

void
skip_c(char **s, char c);

bool
is_crlf(char *, size_t);

#endif
