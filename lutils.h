#ifndef _LUTILS_H_
#define _LUTILS_H_

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Toggle assertions on and off here
#define DEBUG 1
#if DEBUG
#define panic_if_not(cond)                        \
    do                                            \
    {                                             \
        if (!(cond))                              \
        {                                         \
            fprintf(                              \
                stderr,                           \
                "%s:%d: Assertion '%s' failed\n", \
                __FILE__,                         \
                __LINE__,                         \
                #cond);                           \
            exit(1);                              \
        }                                         \
    } while (0)
#else
#define panic_if_not(...) (void)0
#endif

// Arena
typedef struct arena Arena;
struct arena
{
    uint8_t *data;
    size_t capacity;
    size_t index;
};

#define GIBIBYTE 1ULL << 30
#define ARENA_DEFAULT_CAPACITY 4ULL * GIBIBYTE

// Calulates the alignment of a given type
#define align_of(type) offsetof(struct { char c; type member; }, member)

// Rounds 'x' up to the nearest multiple of 'b', where 'b' must be a power of two.
#define align_pow_2(x, b) (((x) + (b) - 1) & (~((b) - 1)))

Arena arena_create(size_t capacity);
void arena_release(Arena *a);
void *arena_push(Arena *a, size_t size, size_t align);
void *arena_push_zero(Arena *a, size_t size, size_t align);

// String
typedef struct string String;
struct string
{
    const char *content;
    size_t length;
};

#define str(s) \
    {.content = s, .length = sizeof(s) - 1}

String string_cstr(Arena *arena, char *cstr);
String string_new(Arena *arena, size_t length);
String string_substring(String s, size_t start, size_t end);
String string_concat(Arena *arena, String s1, String s2);
bool string_equals(String s1, String s2);
void string_print_raw(String s);

#endif