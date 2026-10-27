#include "linalg.h"

static MathStatus validate_vector(const int64_t *left, const int64_t *right,
                                  int64_t *result, size_t length)
{
    if (length > LINALG_MAX_DIMENSION || (length != 0 &&
        (left == 0 || right == 0 || result == 0))) return MATH_INVALID_ARGUMENT;
    return MATH_OK;
}

static MathStatus validate_matrix(const int64_t *left, const int64_t *right,
                                  int64_t *result, size_t rows, size_t columns)
{
    if (rows > LINALG_MAX_DIMENSION || columns > LINALG_MAX_DIMENSION ||
        (rows != 0 && columns != 0 && (left == 0 || right == 0 || result == 0)))
        return MATH_INVALID_ARGUMENT;
    return MATH_OK;
}

static void copy_values(int64_t *destination, const int64_t *source, size_t count)
{
    size_t index;
    for (index = 0; index < count; ++index) destination[index] = source[index];
}

MathStatus linalg_vector_add(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t length)
{
    size_t index;
    int64_t temporary[LINALG_MAX_DIMENSION];
    MathStatus status = validate_vector(left, right, result, length);
    if (status != MATH_OK) return status;
    for (index = 0; index < length; ++index) {
        status = math_add_i64(left[index], right[index], &temporary[index]);
        if (status != MATH_OK) return status;
    }
    copy_values(result, temporary, length);
    return MATH_OK;
}

MathStatus linalg_vector_sub(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t length)
{
    size_t index;
    int64_t temporary[LINALG_MAX_DIMENSION];
    MathStatus status = validate_vector(left, right, result, length);
    if (status != MATH_OK) return status;
    for (index = 0; index < length; ++index) {
        status = math_sub_i64(left[index], right[index], &temporary[index]);
        if (status != MATH_OK) return status;
    }
    copy_values(result, temporary, length);
    return MATH_OK;
}

MathStatus linalg_vector_scale(const int64_t *vector, int64_t scalar,
                               int64_t *result, size_t length)
{
    size_t index;
    int64_t temporary[LINALG_MAX_DIMENSION];
    if (length > LINALG_MAX_DIMENSION || (length != 0 && (vector == 0 || result == 0)))
        return MATH_INVALID_ARGUMENT;
    for (index = 0; index < length; ++index) {
        MathStatus status = math_mul_i64(vector[index], scalar, &temporary[index]);
        if (status != MATH_OK) return status;
    }
    copy_values(result, temporary, length);
    return MATH_OK;
}

MathStatus linalg_vector_dot(const int64_t *left, const int64_t *right,
                             size_t length, int64_t *result)
{
    size_t index;
    int64_t sum = 0;
    if (length > LINALG_MAX_DIMENSION || result == 0 ||
        (length != 0 && (left == 0 || right == 0))) return MATH_INVALID_ARGUMENT;
    for (index = 0; index < length; ++index) {
        int64_t product;
        MathStatus status = math_mul_i64(left[index], right[index], &product);
        if (status != MATH_OK) return status;
        status = math_add_i64(sum, product, &sum);
        if (status != MATH_OK) return status;
    }
    *result = sum;
    return MATH_OK;
}

MathStatus linalg_vector_norm_squared(const int64_t *vector, size_t length,
                                      int64_t *result)
{
    return linalg_vector_dot(vector, vector, length, result);
}

MathStatus linalg_matrix_identity(int64_t *matrix, size_t dimension)
{
    size_t row;
    size_t column;
    if (dimension > LINALG_MAX_DIMENSION || (dimension != 0 && matrix == 0))
        return MATH_INVALID_ARGUMENT;
    for (row = 0; row < dimension; ++row)
        for (column = 0; column < dimension; ++column)
            matrix[row * dimension + column] = row == column ? 1 : 0;
    return MATH_OK;
}

MathStatus linalg_matrix_add(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t rows, size_t columns)
{
    size_t index;
    int64_t temporary[LINALG_MAX_DIMENSION * LINALG_MAX_DIMENSION];
    MathStatus status = validate_matrix(left, right, result, rows, columns);
    if (status != MATH_OK) return status;
    for (index = 0; index < rows * columns; ++index) {
        status = math_add_i64(left[index], right[index], &temporary[index]);
        if (status != MATH_OK) return status;
    }
    copy_values(result, temporary, rows * columns);
    return MATH_OK;
}

MathStatus linalg_matrix_sub(const int64_t *left, const int64_t *right,
                             int64_t *result, size_t rows, size_t columns)
{
    size_t index;
    int64_t temporary[LINALG_MAX_DIMENSION * LINALG_MAX_DIMENSION];
    MathStatus status = validate_matrix(left, right, result, rows, columns);
    if (status != MATH_OK) return status;
    for (index = 0; index < rows * columns; ++index) {
        status = math_sub_i64(left[index], right[index], &temporary[index]);
        if (status != MATH_OK) return status;
    }
    copy_values(result, temporary, rows * columns);
    return MATH_OK;
}

MathStatus linalg_matrix_scale(const int64_t *matrix, int64_t scalar,
                               int64_t *result, size_t rows, size_t columns)
{
    size_t index;
    int64_t temporary[LINALG_MAX_DIMENSION * LINALG_MAX_DIMENSION];
    if (rows > LINALG_MAX_DIMENSION || columns > LINALG_MAX_DIMENSION ||
        (rows != 0 && columns != 0 && (matrix == 0 || result == 0)))
        return MATH_INVALID_ARGUMENT;
    for (index = 0; index < rows * columns; ++index) {
        MathStatus status = math_mul_i64(matrix[index], scalar, &temporary[index]);
        if (status != MATH_OK) return status;
    }
    copy_values(result, temporary, rows * columns);
    return MATH_OK;
}

MathStatus linalg_matrix_multiply(const int64_t *left, const int64_t *right,
                                  int64_t *result, size_t left_rows,
                                  size_t inner_dimension, size_t right_columns)
{
    size_t row;
    size_t column;
    size_t inner;
    int64_t temporary[LINALG_MAX_DIMENSION * LINALG_MAX_DIMENSION];
    if (left_rows > LINALG_MAX_DIMENSION || inner_dimension > LINALG_MAX_DIMENSION ||
        right_columns > LINALG_MAX_DIMENSION ||
        ((left_rows != 0 && inner_dimension != 0) && left == 0) ||
        ((inner_dimension != 0 && right_columns != 0) && right == 0) ||
        ((left_rows != 0 && right_columns != 0) && result == 0)) return MATH_INVALID_ARGUMENT;
    for (row = 0; row < left_rows; ++row) {
        for (column = 0; column < right_columns; ++column) {
            int64_t sum = 0;
            for (inner = 0; inner < inner_dimension; ++inner) {
                int64_t product;
                MathStatus status = math_mul_i64(left[row * inner_dimension + inner],
                                                  right[inner * right_columns + column], &product);
                if (status != MATH_OK) return status;
                status = math_add_i64(sum, product, &sum);
                if (status != MATH_OK) return status;
            }
            temporary[row * right_columns + column] = sum;
        }
    }
    copy_values(result, temporary, left_rows * right_columns);
    return MATH_OK;
}

MathStatus linalg_matrix_transpose(const int64_t *matrix, int64_t *result,
                                   size_t rows, size_t columns)
{
    size_t row;
    size_t column;
    int64_t temporary[LINALG_MAX_DIMENSION * LINALG_MAX_DIMENSION];
    if (rows > LINALG_MAX_DIMENSION || columns > LINALG_MAX_DIMENSION ||
        (rows != 0 && columns != 0 && (matrix == 0 || result == 0)))
        return MATH_INVALID_ARGUMENT;
    for (row = 0; row < rows; ++row)
        for (column = 0; column < columns; ++column)
            temporary[column * rows + row] = matrix[row * columns + column];
    copy_values(result, temporary, rows * columns);
    return MATH_OK;
}
