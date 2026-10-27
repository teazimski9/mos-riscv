#include "forth.h"

#include <stddef.h>
#include <stdint.h>

#include "../../math/math_core.h"
#include "../../uart.h"

#define FORTH_STACK_CAPACITY 64U
#define FORTH_TOKEN_CAPACITY 32U
#define FORTH_LINE_CAPACITY 128U
#define FORTH_DEFINITION_CAPACITY 16U
#define FORTH_DEFINITION_TOKEN_CAPACITY 32U
#define FORTH_EXECUTION_DEPTH 16U

typedef struct {
    char name[FORTH_TOKEN_CAPACITY];
    char tokens[FORTH_DEFINITION_TOKEN_CAPACITY][FORTH_TOKEN_CAPACITY];
    size_t token_count;
} ForthDefinition;

typedef struct {
    int is_float;
    int64_t integer;
    double floating;
} ForthValue;

typedef struct {
    ForthValue values[FORTH_STACK_CAPACITY];
    size_t count;
    ForthDefinition definitions[FORTH_DEFINITION_CAPACITY];
    size_t definition_count;
    ForthDefinition *compiling;
    int running;
} Forth;

static int strings_equal(const char *left, const char *right)
{
    for (;;) {
        char left_character = *left++;
        char right_character = *right++;
        if (left_character >= 'A' && left_character <= 'Z')
            left_character = (char)(left_character + ('a' - 'A'));
        if (right_character >= 'A' && right_character <= 'Z')
            right_character = (char)(right_character + ('a' - 'A'));
        if (left_character != right_character) return 0;
        if (left_character == '\0') return 1;
    }
}

static void copy_string(char *destination, const char *source, size_t capacity)
{
    size_t index = 0;
    while (index + 1 < capacity && source[index] != '\0') {
        destination[index] = source[index];
        ++index;
    }
    destination[index] = '\0';
}

static void write_i64(int64_t value)
{
    char digits[20];
    uint64_t magnitude;
    size_t count = 0;

    if (value < 0) {
        uart_putc('-');
        magnitude = (uint64_t)(-(value + 1)) + 1U;
    } else {
        magnitude = (uint64_t)value;
    }

    do {
        digits[count++] = (char)('0' + magnitude % 10U);
        magnitude /= 10U;
    } while (magnitude != 0);

    while (count != 0) uart_putc(digits[--count]);
}

static void write_f64(double value)
{
    uint64_t whole;
    unsigned int digits[6];
    size_t count = 6;
    size_t index;

    if (value < 0.0) {
        uart_putc('-');
        value = -value;
    }
    if (value > (double)INT64_MAX) {
        uart_write("float-overflow");
        return;
    }
    value += 0.0000005;
    whole = (uint64_t)value;
    write_i64((int64_t)whole);
    value -= (double)whole;
    for (index = 0; index < count; ++index) {
        value *= 10.0;
        digits[index] = (unsigned int)value;
        value -= (double)digits[index];
    }
    while (count > 1 && digits[count - 1] == 0U) --count;
    uart_putc('.');
    for (index = 0; index < count; ++index) uart_putc((char)('0' + digits[index]));
}

static int push_value(Forth *forth, ForthValue value)
{
    if (forth->count == FORTH_STACK_CAPACITY) {
        uart_write(" stack overflow\n");
        return 0;
    }
    forth->values[forth->count++] = value;
    return 1;
}

static int push_i64(Forth *forth, int64_t value)
{
    ForthValue forth_value = { 0, value, 0.0 };
    return push_value(forth, forth_value);
}

static int push_f64(Forth *forth, double value)
{
    ForthValue forth_value = { 1, 0, value };
    return push_value(forth, forth_value);
}

static int pop_value(Forth *forth, ForthValue *value)
{
    if (forth->count == 0) {
        uart_write(" stack underflow\n");
        return 0;
    }
    *value = forth->values[--forth->count];
    return 1;
}

static double as_f64(ForthValue value)
{
    return value.is_float ? value.floating : (double)value.integer;
}

static int parse_i64(const char *text, int64_t *value)
{
    uint64_t result = 0;
    uint64_t limit;
    int negative = 0;

    if (*text == '-') {
        negative = 1;
        ++text;
    }
    if (*text == '\0') return 0;
    limit = negative ? UINT64_C(9223372036854775808) : INT64_MAX;
    while (*text != '\0') {
        unsigned int digit;
        if (*text < '0' || *text > '9') return 0;
        digit = (unsigned int)(*text++ - '0');
        if (result > (limit - digit) / 10U) return 0;
        result = result * 10U + digit;
    }
    if (negative) {
        *value = result == UINT64_C(9223372036854775808) ? INT64_MIN : -(int64_t)result;
    } else {
        *value = (int64_t)result;
    }
    return 1;
}

static int parse_f64(const char *text, double *value)
{
    double result = 0.0;
    double scale = 0.1;
    int negative = 0;
    int has_digits = 0;
    int exponent = 0;
    int exponent_negative = 0;

    if (*text == '-') {
        negative = 1;
        ++text;
    } else if (*text == '+') {
        ++text;
    }
    while (*text >= '0' && *text <= '9') {
        result = result * 10.0 + (double)(*text++ - '0');
        has_digits = 1;
    }
    if (*text == '.') {
        ++text;
        while (*text >= '0' && *text <= '9') {
            result += (double)(*text++ - '0') * scale;
            scale *= 0.1;
            has_digits = 1;
        }
    }
    if (!has_digits) return 0;
    if (*text == 'e' || *text == 'E') {
        ++text;
        if (*text == '-' || *text == '+') {
            exponent_negative = *text == '-';
            ++text;
        }
        if (*text < '0' || *text > '9') return 0;
        while (*text >= '0' && *text <= '9') {
            if (exponent > 308) return 0;
            exponent = exponent * 10 + (*text++ - '0');
        }
        if (*text != '\0' || exponent > 308) return 0;
        while (exponent-- != 0) result = exponent_negative ? result / 10.0 : result * 10.0;
    } else if (*text != '\0') {
        return 0;
    }
    *value = negative ? -result : result;
    return 1;
}

static ForthDefinition *find_definition(Forth *forth, const char *name)
{
    size_t index;
    for (index = 0; index < forth->definition_count; ++index) {
        if (strings_equal(forth->definitions[index].name, name))
            return &forth->definitions[index];
    }
    return 0;
}

static int begin_definition(Forth *forth, const char *name)
{
    if (*name == '\0' || forth->definition_count == FORTH_DEFINITION_CAPACITY) {
        uart_write(" cannot define word\n");
        return 0;
    }
    forth->compiling = &forth->definitions[forth->definition_count++];
    forth->compiling->token_count = 0;
    copy_string(forth->compiling->name, name, FORTH_TOKEN_CAPACITY);
    return 1;
}

static int compile_string(Forth *forth, const char *text)
{
    ForthDefinition *definition = forth->compiling;
    size_t length = 0;

    while (text[length] != '\0') ++length;
    if (definition->token_count == FORTH_DEFINITION_TOKEN_CAPACITY ||
        length + 2 > FORTH_TOKEN_CAPACITY) {
        uart_write(" string is too long\n");
        return 0;
    }
    definition->tokens[definition->token_count][0] = '\1';
    copy_string(&definition->tokens[definition->token_count][1], text,
                FORTH_TOKEN_CAPACITY - 1);
    ++definition->token_count;
    return 1;
}

static int execute_token(Forth *forth, const char *token, size_t depth);

static int execute_definition(Forth *forth, ForthDefinition *definition, size_t depth)
{
    size_t index;
    if (depth == FORTH_EXECUTION_DEPTH) {
        uart_write(" definition nesting too deep\n");
        return 0;
    }
    for (index = 0; index < definition->token_count; ++index) {
        if (!execute_token(forth, definition->tokens[index], depth + 1)) return 0;
    }
    return 1;
}

static int binary_operation(Forth *forth, char operation)
{
    ForthValue left;
    ForthValue right;
    MathStatus status;
    if (!pop_value(forth, &right) || !pop_value(forth, &left)) return 0;
    if (left.is_float || right.is_float) {
        double result;
        if (operation == '%') {
            uart_write(" modulo requires integers\n");
            return 0;
        }
        switch (operation) {
        case '+': status = math_add_f64(as_f64(left), as_f64(right), &result); break;
        case '-': status = math_sub_f64(as_f64(left), as_f64(right), &result); break;
        case '*': status = math_mul_f64(as_f64(left), as_f64(right), &result); break;
        default: status = math_div_f64(as_f64(left), as_f64(right), &result); break;
        }
        if (status != MATH_OK) {
            uart_write(status == MATH_DIVIDE_BY_ZERO ? " division by zero\n" : " arithmetic error\n");
            return 0;
        }
        return push_f64(forth, result);
    }
    switch (operation) {
    case '+': status = math_add_i64(left.integer, right.integer, &left.integer); break;
    case '-': status = math_sub_i64(left.integer, right.integer, &left.integer); break;
    case '*': status = math_mul_i64(left.integer, right.integer, &left.integer); break;
    case '/': status = math_div_i64(left.integer, right.integer, &left.integer); break;
    default: status = math_mod_i64(left.integer, right.integer, &left.integer); break;
    }
    if (status != MATH_OK) {
        uart_write(status == MATH_DIVIDE_BY_ZERO ? " division by zero\n" : " arithmetic error\n");
        return 0;
    }
    return push_value(forth, left);
}

static int execute_token(Forth *forth, const char *token, size_t depth)
{
    ForthValue value;
    int64_t integer;
    double floating;
    ForthDefinition *definition;

    if (token[0] == '\1') {
        uart_write(token + 1);
        return 1;
    }
    if (parse_i64(token, &integer)) return push_i64(forth, integer);
    if (parse_f64(token, &floating)) return push_f64(forth, floating);
    if (strings_equal(token, "+")) return binary_operation(forth, '+');
    if (strings_equal(token, "-")) return binary_operation(forth, '-');
    if (strings_equal(token, "*")) return binary_operation(forth, '*');
    if (strings_equal(token, "/")) return binary_operation(forth, '/');
    if (strings_equal(token, "mod")) return binary_operation(forth, '%');
    if (strings_equal(token, "f+")) return binary_operation(forth, '+');
    if (strings_equal(token, "f-")) return binary_operation(forth, '-');
    if (strings_equal(token, "f*")) return binary_operation(forth, '*');
    if (strings_equal(token, "f/")) return binary_operation(forth, '/');
    if (strings_equal(token, "fsqrt")) {
        if (!pop_value(forth, &value)) return 0;
        if (math_sqrt_f64(as_f64(value), &floating) != MATH_OK) {
            uart_write(" arithmetic error\n");
            return 0;
        }
        return push_f64(forth, floating);
    }
    if (strings_equal(token, "fpow")) {
        ForthValue exponent;
        if (!pop_value(forth, &exponent) || !pop_value(forth, &value)) return 0;
        if (exponent.is_float || exponent.integer < 0 || (uint64_t)exponent.integer > UINT32_MAX ||
            math_pow_f64(as_f64(value), (uint32_t)exponent.integer, &floating) != MATH_OK) {
            uart_write(" arithmetic error\n");
            return 0;
        }
        return push_f64(forth, floating);
    }
    if (strings_equal(token, "fclamp")) {
        ForthValue maximum;
        ForthValue minimum;
        if (!pop_value(forth, &maximum) || !pop_value(forth, &minimum) || !pop_value(forth, &value))
            return 0;
        return push_f64(forth, math_clamp_f64(as_f64(value), as_f64(minimum), as_f64(maximum)));
    }
    if (strings_equal(token, "dup") || strings_equal(token, "fdup")) {
        if (forth->count == 0) {
            uart_write(" stack underflow\n");
            return 0;
        }
        return push_value(forth, forth->values[forth->count - 1]);
    }
    if (strings_equal(token, "drop") || strings_equal(token, "fdrop")) return pop_value(forth, &value);
    if (strings_equal(token, "swap") || strings_equal(token, "fswap")) {
        if (forth->count < 2) {
            uart_write(" stack underflow\n");
            return 0;
        }
        value = forth->values[forth->count - 1];
        forth->values[forth->count - 1] = forth->values[forth->count - 2];
        forth->values[forth->count - 2] = value;
        return 1;
    }
    if (strings_equal(token, "over") || strings_equal(token, "fover")) {
        if (forth->count < 2) {
            uart_write(" stack underflow\n");
            return 0;
        }
        return push_value(forth, forth->values[forth->count - 2]);
    }
    if (strings_equal(token, ".") || strings_equal(token, "f.")) {
        if (!pop_value(forth, &value)) return 0;
        if (value.is_float) write_f64(value.floating);
        else write_i64(value.integer);
        uart_putc(' ');
        return 1;
    }
    if (strings_equal(token, ".s")) {
        size_t index;
        uart_putc('<');
        write_i64((int64_t)forth->count);
        uart_write("> ");
        for (index = 0; index < forth->count; ++index) {
            if (forth->values[index].is_float) write_f64(forth->values[index].floating);
            else write_i64(forth->values[index].integer);
            uart_putc(' ');
        }
        return 1;
    }
    if (strings_equal(token, "clear")) {
        forth->count = 0;
        return 1;
    }
    if (strings_equal(token, "cr")) {
        uart_putc('\n');
        return 1;
    }
    if (strings_equal(token, "bye")) {
        forth->running = 0;
        return 1;
    }
    definition = find_definition(forth, token);
    if (definition != 0) return execute_definition(forth, definition, depth);
    uart_write(" unknown word: ");
    uart_write(token);
    uart_putc('\n');
    return 0;
}

static void execute_line(Forth *forth, char *line)
{
    char *token = line;
    while (*token != '\0') {
        char *end;
        while (*token == ' ' || *token == '\t') ++token;
        if (*token == '\0') return;

        if (token[0] == '.' && token[1] == '(') {
            end = token + 2;
            while (*end != '\0' && *end != ')') ++end;
            if (*end == '\0') {
                uart_write(" unterminated comment\n");
                return;
            }
            *end++ = '\0';
            uart_write(token + 2);
            token = end;
            continue;
        }
        if (token[0] == '.' && token[1] == '"') {
            end = token + 2;
            while (*end != '\0' && *end != '"') ++end;
            if (*end == '\0') {
                uart_write(" unterminated string\n");
                return;
            }
            *end++ = '\0';
            if (forth->compiling != 0) {
                if (!compile_string(forth, token + 2)) return;
            } else {
                uart_write(token + 2);
            }
            token = end;
            continue;
        }
        end = token;
        while (*end != '\0' && *end != ' ' && *end != '\t') ++end;
        if (*end != '\0') *end++ = '\0';

        if (forth->compiling != 0) {
            if (strings_equal(token, ";")) {
                forth->compiling = 0;
                uart_write(" ok\n");
            } else if (forth->compiling->token_count == FORTH_DEFINITION_TOKEN_CAPACITY) {
                uart_write(" definition too long\n");
                forth->compiling = 0;
            } else {
                copy_string(forth->compiling->tokens[forth->compiling->token_count++], token,
                            FORTH_TOKEN_CAPACITY);
            }
        } else if (token[0] == ':' && token[1] != '\0') {
            if (!begin_definition(forth, token + 1)) return;
        } else if (strings_equal(token, ":")) {
            while (*end == ' ' || *end == '\t') ++end;
            token = end;
            while (*end != '\0' && *end != ' ' && *end != '\t') ++end;
            if (*end == '\0' && token == end) {
                uart_write(" cannot define word\n");
                return;
            }
            if (*end != '\0') *end++ = '\0';
            if (!begin_definition(forth, token)) return;
            token = end;
            continue;
        } else {
            execute_token(forth, token, 0);
        }
        token = end;
    }
}

static void read_line(char *line)
{
    size_t length = 0;
    for (;;) {
        char character = uart_getc();
        if (character == '\r' || character == '\n') {
            uart_putc('\n');
            line[length] = '\0';
            return;
        }
        if (character == '\b' || character == 127) {
            if (length != 0) {
                --length;
                uart_write("\b \b");
            }
        } else if (character >= ' ' && character <= '~' && length + 1 < FORTH_LINE_CAPACITY) {
            line[length++] = character;
            uart_putc(character);
        }
    }
}

void forth_run(void)
{
    static Forth forth;
    char line[FORTH_LINE_CAPACITY];

    forth.count = 0;
    forth.definition_count = 0;
    forth.compiling = 0;
    forth.running = 1;
    while (forth.running) {
        uart_write("ok> ");
        read_line(line);
        execute_line(&forth, line);
    }
    uart_write("bye\n");
}
