#ifndef RPN_STACK_H
#define RPN_STACK_H

#include <stddef.h>
#include <stdint.h>
#include "math_core.h"

#define RPN_STACK_CAPACITY 64U

typedef struct {
    int64_t values[RPN_STACK_CAPACITY];
    size_t count;
} RpnStack;

typedef enum {
    RPN_ADD,
    RPN_SUBTRACT,
    RPN_MULTIPLY,
    RPN_DIVIDE,
    RPN_MODULO,
    RPN_POWER
} RpnOperation;

typedef enum {
    RPN_OK = 0,
    RPN_INVALID_ARGUMENT,
    RPN_EMPTY,
    RPN_FULL,
    RPN_MATH_ERROR
} RpnStatus;

void rpn_stack_init(RpnStack *stack);
void rpn_stack_clear(RpnStack *stack);
size_t rpn_stack_size(const RpnStack *stack);
RpnStatus rpn_stack_push(RpnStack *stack, int64_t value);
RpnStatus rpn_stack_pop(RpnStack *stack, int64_t *value);
RpnStatus rpn_stack_peek(const RpnStack *stack, int64_t *value);
RpnStatus rpn_stack_swap(RpnStack *stack);
RpnStatus rpn_stack_apply(RpnStack *stack, RpnOperation operation);

#endif
