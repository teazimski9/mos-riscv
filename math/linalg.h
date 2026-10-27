#ifndef LINALG_H
#define LINALG_H

#include <stddef.h>
#include <stdint.h>
#include "math_core.h"

#define LINALG_MAX_DIMENSION 8U

MathStatus linalg_vector_add(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t length);
MathStatus linalg_vector_sub(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t length);
MathStatus linalg_vector_scale(const int64_t *vector, int64_t scalar,
                               int64_t *result, size_t length);
MathStatus linalg_vector_dot(const int64_t *left, const int64_t *right,
                             size_t length, int64_t *result);
MathStatus linalg_vector_norm_squared(const int64_t *vector, size_t length,
                                      int64_t *result);

MathStatus linalg_matrix_identity(int64_t *matrix, size_t dimension);
MathStatus linalg_matrix_add(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t rows, size_t columns);
MathStatus linalg_matrix_sub(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t rows, size_t columns);
MathStatus linalg_matrix_scale(const int64_t *matrix, int64_t scalar,
                               int64_t *result, size_t rows, size_t columns);
MathStatus linalg_matrix_multiply(const int64_t *left, const int64_t *right,
                                  int64_t *result, size_t left_rows,
                                  size_t inner_dimension, size_t right_columns);
MathStatus linalg_matrix_transpose(const int64_t *matrix, int64_t *result,
                                   size_t rows, size_t columns);

#endif
