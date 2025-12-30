#include <stdlib.h>

// Random double between 0.0 and 1.0
double random_double() {
  return (double)rand() / RAND_MAX;
}