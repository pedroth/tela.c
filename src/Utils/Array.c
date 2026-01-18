#include "Utils/Array.h"
#include <stdlib.h>
#include <string.h>

Array new_array(u32 capacity, u32 element_size) {
  Array array;
  array.data = malloc(capacity * element_size);
  array.length = 0;
  array.capacity = capacity;
  array.element_size = element_size;
  return array;
}

Array filter_array(Array *a, bool (*func)(void *element, u32 index)) {
  Array ans = new_array(a->capacity, a->element_size);
  for (u32 i = 0; i < a->length; i++) {
    void *element = (char *)a->data + (i * a->element_size);
    if (func(element, i)) {
      push_array(&ans, element);
    }
  }
  return ans;
}

Array slice_array(Array *a, u32 start, u32 end) {
  Array ans = new_array(end - start, a->element_size);
  for (u32 i = start; i < end && i < a->length; i++) {
    void *element = (char *)a->data + (i * a->element_size);
    push_array(&ans, element);
  }
  return ans;
}

void *get_array_element(Array *a, u32 index) {
  if (index >= a->length) {
    return NULL;
  }
  return (char *)a->data + (index * a->element_size);
}

bool push_array(Array *a, const void *element) {
  if (a->length >= a->capacity) {
    size_t new_cap = a->capacity == 0 ? 4 : a->capacity * 2;
    void *new_data = realloc(a->data, new_cap * a->element_size);
    if (!new_data)
      return false;

    a->data = new_data;
    a->capacity = new_cap;
  }

  memcpy((char *)a->data + (a->length * a->element_size), element,
         a->element_size);
  a->length++;
  return true;
}

void *pop_array(Array *a) {
  if (a->length == 0) {
    return NULL;
  }

  a->length--;
  if (a->length > 0 && a->length <= a->capacity / 4) {
    u32 new_cap = a->capacity / 2;
    void *new_data = realloc(a->data, new_cap * a->element_size);
    if (new_data) {
      a->data = new_data;
      a->capacity = new_cap;
    }
  }
  return (char *)a->data + (a->length * a->element_size);
}

void free_array(Array *array) {
  free(array->data);
  array->data = NULL;
  array->length = 0;
  array->capacity = 0;
}
