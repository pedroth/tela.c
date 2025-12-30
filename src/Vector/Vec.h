#ifndef VEC_H
#define VEC_H

#include <stddef.h>

typedef struct {
  double *data;
  size_t n;
  size_t dim;
} Vec;

Vec new_vec(size_t n);
Vec from_array(double *array, size_t n);
double get_vec(Vec *v, size_t i);

Vec map_vec(Vec *v, double (*func)(double));
Vec op_vec(Vec *u, Vec *v, double (*func)(double, double));

Vec add_vec(Vec *u, Vec *v);
Vec sub_vec(Vec *u, Vec *v);
Vec mul_vec(Vec *u, Vec *v);
Vec div_vec(Vec *u, Vec *v);
Vec scale_vec(Vec *v, double r);

double dot_vec(Vec *u, Vec *v);
double square_length_vec(Vec *v);
double length_vec(Vec *v);

Vec random_vec(size_t n);
void free_vec(Vec *v);

#endif // VEC_H
