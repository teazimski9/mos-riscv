#include "rpn_stack.h"

void rpn_stack_init(RpnStack *stack)
{
    if (stack != 0) stack->count = 0;
}

void rpn_stack_clear(RpnStack *stack)
{
    rpn_stack_init(stack);
}

size_t rpn_stack_size(const RpnStack *stack)
{
    return stack == 0 ? 0 : stack->count;
}

RpnStatus rpn_stack_push(RpnStack *stack, int64_t value)
{
    if (stack == 0) return RPN_INVALID_ARGUMENT;
    if (stack->count == RPN_STACK_CAPACITY) return RPN_FULL;
    stack->values[stack->count++] = value;
    return RPN_OK;
}

RpnStatus rpn_stack_pop(RpnStack *stack, int64_t *value)
{
    if (stack == 0 || value == 0) return RPN_INVALID_ARGUMENT;
    if (stack->count == 0) return RPN_EMPTY;
    *value = stack->values[--stack->count];
    return RPN_OK;
}

RpnStatus rpn_stack_peek(const RpnStack *stack, int64_t *value)
{
    if (stack == 0 || value == 0) return RPN_INVALID_ARGUMENT;
    if (stack->count == 0) return RPN_EMPTY;
    *value = stack->values[stack->count - 1];
    return RPN_OK;
}

RpnStatus rpn_stack_swap(RpnStack *stack)
{
    int64_t temporary;
    if (stack == 0) return RPN_INVALID_ARGUMENT;
    if (stack->count < 2) return RPN_EMPTY;
    temporary = stack->values[stack->count - 1];
    stack->values[stack->count - 1] = stack->values[stack->count - 2];
    stack->values[stack->count - 2] = temporary;
    return RPN_OK;
}

RpnStatus rpn_stack_apply(RpnStack *stack, RpnOperation operation)
{
    int64_t left;
    int64_t right;
    int64_t result;
    MathStatus status;
    if (stack == 0) return RPN_INVALID_ARGUMENT;
    if (stack->count < 2) return RPN_EMPTY;
    left = stack->values[stack->count - 2];
    right = stack->values[stack->count - 1];
    switch (operation) {
    case RPN_ADD: status = math_add_i64(left, right, &result); break;
    case RPN_SUBTRACT: status = math_sub_i64(left, right, &result); break;
    case RPN_MULTIPLY: status = math_mul_i64(left, right, &result); break;
    case RPN_DIVIDE: status = math_div_i64(left, right, &result); break;
    case RPN_MODULO: status = math_mod_i64(left, right, &result); break;
    case RPN_POWER:
        if (right < 0 || (uint64_t)right > UINT32_MAX) return RPN_MATH_ERROR;
        status = math_pow_i64(left, (uint32_t)right, &result);
        break;
    default: return RPN_INVALID_ARGUMENT;
    }
    if (status != MATH_OK) return RPN_MATH_ERROR;
    stack->values[stack->count - 2] = result;
    --stack->count;
    return RPN_OK;
}
