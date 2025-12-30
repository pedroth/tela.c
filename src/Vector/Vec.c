//========================================================================================
/*                                                                                      *
 *                                        VECTOR *
 *                                                                                      */
//========================================================================================

#define _POSIX_C_SOURCE 200112L

#include "Vec.h"
#include "Utils/random.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>

Vec new_vec(size_t n) {
  Vec v;
  v.data = malloc(n * sizeof(double));
  v.n = n;
  v.dim = n;
  return v;
}

Vec from_array(double *array, size_t n) {
  Vec v;
  v.data = array;
  v.n = n;
  v.dim = n;
  return v;
}

double get_vec(Vec *v, size_t i) {
  assert(i < v->n);
  return v->data[i];
}

Vec map_vec(Vec *v, double (*func)(double)) {
  Vec res = new_vec(v->n);
  for (size_t i = 0; i < v->n; i++) {
    res.data[i] = func(v->data[i]);
  }
  return res;
}

Vec op_vec(Vec *u, Vec *v, double (*func)(double, double)) {
  assert(u->n == v->n);
  Vec res = new_vec(u->n);
  for (size_t i = 0; i < u->n; i++) {
    res.data[i] = func(u->data[i], v->data[i]);
  }
  return res;
}

double add_func(double a, double b) {
  return a + b;
}

double sub_func(double a, double b) {
  return a - b;
}

double mul_func(double a, double b) {
  return a * b;
}

double div_func(double a, double b) {
  return a / b;
}

Vec add_vec(Vec *u, Vec *v) {
  return op_vec(u, v, add_func);
}

Vec sub_vec(Vec *u, Vec *v) {
  return op_vec(u, v, sub_func);
}

Vec mul_vec(Vec *u, Vec *v) {
  return op_vec(u, v, mul_func);
}

Vec div_vec(Vec *u, Vec *v) {
  return op_vec(u, v, div_func);
}

Vec scale_vec(Vec *v, double r) {
  Vec res = new_vec(v->n);
  for (size_t i = 0; i < v->n; i++) {
    res.data[i] = v->data[i] * r;
  }
  return res;
}

double dot_vec(Vec *u, Vec *v) {
  assert(u->n == v->n);
  double res = 0.0;
  for (size_t i = 0; i < u->n; i++) {
    res += u->data[i] * v->data[i];
  }
  return res;
}

double square_length_vec(Vec *v) {
  return dot_vec(v, v);
}

double length_vec(Vec *v) {
  return sqrt(square_length_vec(v));
}

Vec random_vec(size_t n) {
  Vec v = new_vec(n);
  for (size_t i = 0; i < n; i++) {
    v.data[i] = random_double();
  }
  return v;
}

void free_vec(Vec *v) {
  if (v->data) {
    free(v->data);
    v->data = NULL;
  }
  v->n = 0;
}