
#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  void *data;
  u32 length;
  u32 capacity;
  u32 element_size;
} Array;

/* function prototypes */
Array new_array(u32 capacity, u32 element_size);
Array filter_array(Array *a, bool (*func)(void *element, u32 index));
Array slice_array(Array *a, u32 start, u32 end);
void *get_array_element(Array *a, u32 index);
bool push_array(Array *a, const void *element);
void *pop_array(Array *a);
void free_array(Array *array);

#endif // ARRAY_H