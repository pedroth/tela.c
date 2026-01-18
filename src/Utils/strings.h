#ifndef STRINGS_H
#define STRINGS_H

#include "Utils/types.h"
#include "Utils/Array.h"

i32 starts_with(const char *str, const char *prefix);
Array split_str(const char *str, const char *delim);

#endif // STRINGS_H
