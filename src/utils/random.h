#ifndef RANDOM_H
#define RANDOM_H

#include <stdlib.h>

// Random double between 0.0 and 1.0
static inline double random_double(void) {
  return (double)rand() / RAND_MAX;
}

#endif // RANDOM_H
