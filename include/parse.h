#ifndef __H_PARSE
#define __H_PARSE

#include <stddef.h>
#include <stdbool.h>

#include "substring.h"

bool
eat_until(char **, char, substring *);

#endif
