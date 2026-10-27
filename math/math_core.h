#ifndef MATH_CORE_H
#define MATH_CORE_H

#include <stdint.h>

typedef enum {
    MATH_OK = 0,
    MATH_INVALID_ARGUMENT,
    MATH_DIVIDE_BY_ZERO,
    MATH_OVERFLOW,
    MATH_NOT_FINITE
} MathStatus;

#define MATH_Q16_SHIFT 16
#define MATH_Q16_ONE   (INT64_C(1) << MATH_Q16_SHIFT)
#define MATH_Q16_PI    INT64_C(205887)
#define MATH_Q16_TWO_PI INT64_C(411775)
#define MATH_Q16_HALF_PI INT64_C(102944)

MathStatus math_add_i64(int64_t left, int64_t right, int64_t *result);
MathStatus math_sub_i64(int64_t left, int64_t right, int64_t *result);
MathStatus math_mul_i64(int64_t left, int64_t right, int64_t *result);
MathStatus math_div_i64(int64_t dividend, int64_t divisor, int64_t *result);
MathStatus math_mod_i64(int64_t dividend, int64_t divisor, int64_t *result);
MathStatus math_pow_i64(int64_t base, uint32_t exponent, int64_t *result);
uint64_t math_gcd_u64(uint64_t left, uint64_t right);
MathStatus math_lcm_u64(uint64_t left, uint64_t right, uint64_t *result);
uint64_t math_isqrt_u64(uint64_t value);
int64_t math_clamp_i64(int64_t value, int64_t minimum, int64_t maximum);

MathStatus math_add_f64(double left, double right, double *result);
MathStatus math_sub_f64(double left, double right, double *result);
MathStatus math_mul_f64(double left, double right, double *result);
MathStatus math_div_f64(double dividend, double divisor, double *result);
MathStatus math_pow_f64(double base, uint32_t exponent, double *result);
MathStatus math_sqrt_f64(double value, double *result);
double math_clamp_f64(double value, double minimum, double maximum);

MathStatus math_sin_cos_q16(int64_t radians, int64_t *sine, int64_t *cosine);
MathStatus math_sin_q16(int64_t radians, int64_t *result);
MathStatus math_cos_q16(int64_t radians, int64_t *result);
MathStatus math_tan_q16(int64_t radians, int64_t *result);

#endif
