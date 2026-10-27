#include "math_core.h"

#include <float.h>

#define I64_MAX_VALUE INT64_MAX
#define I64_MIN_VALUE INT64_MIN

#define CORDIC_ITERATIONS 16U
#define CORDIC_GAIN_INVERSE_Q16 INT64_C(39797)

static const int64_t cordic_atan_q16[CORDIC_ITERATIONS] = {
    51472, 30386, 16055, 8150, 4091, 2047, 1024, 512,
    256, 128, 64, 32, 16, 8, 4, 2
};

MathStatus math_add_i64(int64_t left, int64_t right, int64_t *result)
{
    if (result == 0) return MATH_INVALID_ARGUMENT;
    if ((right > 0 && left > I64_MAX_VALUE - right) ||
        (right < 0 && left < I64_MIN_VALUE - right)) return MATH_OVERFLOW;
    *result = left + right;
    return MATH_OK;
}

MathStatus math_sub_i64(int64_t left, int64_t right, int64_t *result)
{
    if (result == 0) return MATH_INVALID_ARGUMENT;
    if ((right > 0 && left < I64_MIN_VALUE + right) ||
        (right < 0 && left > I64_MAX_VALUE + right)) return MATH_OVERFLOW;
    *result = left - right;
    return MATH_OK;
}

MathStatus math_mul_i64(int64_t left, int64_t right, int64_t *result)
{
    if (result == 0) return MATH_INVALID_ARGUMENT;
    if (left == 0 || right == 0) {
        *result = 0;
        return MATH_OK;
    }
    if ((left == I64_MIN_VALUE && right == -1) ||
        (right == I64_MIN_VALUE && left == -1)) return MATH_OVERFLOW;
    if ((left > 0 && right > 0 && left > I64_MAX_VALUE / right) ||
        (left > 0 && right < 0 && right < I64_MIN_VALUE / left) ||
        (left < 0 && right > 0 && left < I64_MIN_VALUE / right) ||
        (left < 0 && right < 0 && left < I64_MAX_VALUE / right)) return MATH_OVERFLOW;
    *result = left * right;
    return MATH_OK;
}

MathStatus math_div_i64(int64_t dividend, int64_t divisor, int64_t *result)
{
    if (result == 0) return MATH_INVALID_ARGUMENT;
    if (divisor == 0) return MATH_DIVIDE_BY_ZERO;
    if (dividend == I64_MIN_VALUE && divisor == -1) return MATH_OVERFLOW;
    *result = dividend / divisor;
    return MATH_OK;
}

MathStatus math_mod_i64(int64_t dividend, int64_t divisor, int64_t *result)
{
    if (result == 0) return MATH_INVALID_ARGUMENT;
    if (divisor == 0) return MATH_DIVIDE_BY_ZERO;
    if (dividend == I64_MIN_VALUE && divisor == -1) {
        *result = 0;
        return MATH_OK;
    }
    *result = dividend % divisor;
    return MATH_OK;
}

MathStatus math_pow_i64(int64_t base, uint32_t exponent, int64_t *result)
{
    int64_t power = 1;
    if (result == 0) return MATH_INVALID_ARGUMENT;
    while (exponent != 0) {
        if ((exponent & 1U) != 0 && math_mul_i64(power, base, &power) != MATH_OK)
            return MATH_OVERFLOW;
        exponent >>= 1U;
        if (exponent != 0 && math_mul_i64(base, base, &base) != MATH_OK)
            return MATH_OVERFLOW;
    }
    *result = power;
    return MATH_OK;
}

uint64_t math_gcd_u64(uint64_t left, uint64_t right)
{
    while (right != 0) {
        uint64_t remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

MathStatus math_lcm_u64(uint64_t left, uint64_t right, uint64_t *result)
{
    uint64_t reduced;
    if (result == 0) return MATH_INVALID_ARGUMENT;
    if (left == 0 || right == 0) {
        *result = 0;
        return MATH_OK;
    }
    reduced = left / math_gcd_u64(left, right);
    if (reduced > UINT64_MAX / right) return MATH_OVERFLOW;
    *result = reduced * right;
    return MATH_OK;
}

uint64_t math_isqrt_u64(uint64_t value)
{
    uint64_t result = 0;
    uint64_t bit = UINT64_C(1) << 62;
    while (bit > value) bit >>= 2;
    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }
        bit >>= 2;
    }
    return result;
}

int64_t math_clamp_i64(int64_t value, int64_t minimum, int64_t maximum)
{
    if (minimum > maximum) return minimum;
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static int f64_is_finite(double value)
{
    return value == value && value <= DBL_MAX && value >= -DBL_MAX;
}

static MathStatus validate_f64_result(double *result)
{
    return result == 0 ? MATH_INVALID_ARGUMENT : MATH_OK;
}

MathStatus math_add_f64(double left, double right, double *result)
{
    if (validate_f64_result(result) != MATH_OK) return MATH_INVALID_ARGUMENT;
    if (!f64_is_finite(left) || !f64_is_finite(right)) return MATH_NOT_FINITE;
    *result = left + right;
    return f64_is_finite(*result) ? MATH_OK : MATH_NOT_FINITE;
}

MathStatus math_sub_f64(double left, double right, double *result)
{
    if (validate_f64_result(result) != MATH_OK) return MATH_INVALID_ARGUMENT;
    if (!f64_is_finite(left) || !f64_is_finite(right)) return MATH_NOT_FINITE;
    *result = left - right;
    return f64_is_finite(*result) ? MATH_OK : MATH_NOT_FINITE;
}

MathStatus math_mul_f64(double left, double right, double *result)
{
    if (validate_f64_result(result) != MATH_OK) return MATH_INVALID_ARGUMENT;
    if (!f64_is_finite(left) || !f64_is_finite(right)) return MATH_NOT_FINITE;
    *result = left * right;
    return f64_is_finite(*result) ? MATH_OK : MATH_NOT_FINITE;
}

MathStatus math_div_f64(double dividend, double divisor, double *result)
{
    if (validate_f64_result(result) != MATH_OK) return MATH_INVALID_ARGUMENT;
    if (!f64_is_finite(dividend) || !f64_is_finite(divisor)) return MATH_NOT_FINITE;
    if (divisor == 0.0) return MATH_DIVIDE_BY_ZERO;
    *result = dividend / divisor;
    return f64_is_finite(*result) ? MATH_OK : MATH_NOT_FINITE;
}

MathStatus math_pow_f64(double base, uint32_t exponent, double *result)
{
    double power = 1.0;
    if (validate_f64_result(result) != MATH_OK) return MATH_INVALID_ARGUMENT;
    if (!f64_is_finite(base)) return MATH_NOT_FINITE;
    while (exponent != 0U) {
        if ((exponent & 1U) != 0U) {
            MathStatus status = math_mul_f64(power, base, &power);
            if (status != MATH_OK) return status;
        }
        exponent >>= 1U;
        if (exponent != 0U) {
            MathStatus status = math_mul_f64(base, base, &base);
            if (status != MATH_OK) return status;
        }
    }
    *result = power;
    return MATH_OK;
}

MathStatus math_sqrt_f64(double value, double *result)
{
    double estimate;
    double next;
    unsigned int iteration;
    if (validate_f64_result(result) != MATH_OK) return MATH_INVALID_ARGUMENT;
    if (!f64_is_finite(value)) return MATH_NOT_FINITE;
    if (value < 0.0) return MATH_INVALID_ARGUMENT;
    if (value == 0.0) {
        *result = 0.0;
        return MATH_OK;
    }
    estimate = value < 1.0 ? 1.0 : value;
    for (iteration = 0; iteration < 1024U; ++iteration) {
        next = 0.5 * (estimate + value / estimate);
        if (next == estimate) break;
        estimate = next;
    }
    *result = estimate;
    return f64_is_finite(*result) ? MATH_OK : MATH_NOT_FINITE;
}

double math_clamp_f64(double value, double minimum, double maximum)
{
    if (minimum > maximum) return minimum;
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

MathStatus math_sin_cos_q16(int64_t radians, int64_t *sine, int64_t *cosine)
{
    int64_t x = CORDIC_GAIN_INVERSE_Q16;
    int64_t y = 0;
    int64_t angle;
    int cosine_sign = 1;
    unsigned int iteration;

    if (sine == 0 || cosine == 0) return MATH_INVALID_ARGUMENT;

    angle = radians % MATH_Q16_TWO_PI;
    if (angle > MATH_Q16_PI) angle -= MATH_Q16_TWO_PI;
    if (angle < -MATH_Q16_PI) angle += MATH_Q16_TWO_PI;

    if (angle > MATH_Q16_HALF_PI) {
        angle = MATH_Q16_PI - angle;
        cosine_sign = -1;
    } else if (angle < -MATH_Q16_HALF_PI) {
        angle = -MATH_Q16_PI - angle;
        cosine_sign = -1;
    }

    for (iteration = 0; iteration < CORDIC_ITERATIONS; ++iteration) {
        int64_t shifted_x = x >> iteration;
        int64_t shifted_y = y >> iteration;

        if (angle >= 0) {
            x -= shifted_y;
            y += shifted_x;
            angle -= cordic_atan_q16[iteration];
        } else {
            x += shifted_y;
            y -= shifted_x;
            angle += cordic_atan_q16[iteration];
        }
    }

    *sine = y;
    *cosine = cosine_sign < 0 ? -x : x;
    return MATH_OK;
}

MathStatus math_sin_q16(int64_t radians, int64_t *result)
{
    int64_t cosine;
    if (result == 0) return MATH_INVALID_ARGUMENT;
    return math_sin_cos_q16(radians, result, &cosine);
}

MathStatus math_cos_q16(int64_t radians, int64_t *result)
{
    int64_t sine;
    if (result == 0) return MATH_INVALID_ARGUMENT;
    return math_sin_cos_q16(radians, &sine, result);
}

MathStatus math_tan_q16(int64_t radians, int64_t *result)
{
    int64_t sine;
    int64_t cosine;
    int64_t scaled_sine;
    MathStatus status;

    if (result == 0) return MATH_INVALID_ARGUMENT;
    status = math_sin_cos_q16(radians, &sine, &cosine);
    if (status != MATH_OK) return status;
    if (cosine >= -1 && cosine <= 1) return MATH_DIVIDE_BY_ZERO;
    status = math_mul_i64(sine, MATH_Q16_ONE, &scaled_sine);
    if (status != MATH_OK) return status;
    return math_div_i64(scaled_sine, cosine, result);
}
