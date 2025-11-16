
#include "lutils.h"

// Arena implementation

Arena arena_create(size_t capacity)
{
    Arena a = {0};
    panic_if_not((capacity > 0) && "capacity must be larger than zero");
    void *addr = mmap(
        NULL,
        capacity,
        PROT_READ   | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );
    panic_if_not((addr != MAP_FAILED) && "mmap failed");
    a.data = (uint8_t*)addr;
    a.capacity = capacity;
    a.index = 0;
    return a;
}

void arena_release(Arena *a)
{
    munmap(a->data, a->capacity);
    a->data     = NULL;
    a->capacity = 0;
    a->index    = 0;
}

void* arena_push(Arena *a, size_t size, size_t align)
{
    panic_if_not(size > 0 && "size cannot be zero");
    size_t index_pre = align_pow_2(a->index, align);
    size_t index_pst = index_pre + size;
    panic_if_not((a->capacity >= index_pst) && "arena overflow");
    a->index = index_pst;
    return (void *)&a->data[index_pre];
}

void* arena_push_zero(Arena *a, size_t size, size_t align)
{
    void* mem = arena_push(a, size, align);
    panic_if_not(mem != MAP_FAILED);
    memset(mem, 0, size);
    return mem;
}

// String implementation

String string_cstr(Arena *arena, char *cstr)
{
    String s = {0};
    size_t length = strlen(cstr);

    char *mem = arena_push(arena, length, align_of(char));
    panic_if_not(mem != NULL);

    memcpy(mem, cstr, length);
    s.content = (const char *)mem;
    s.length = length;
    return s;
}

String string_new(Arena *arena, size_t length)
{
    String s = {0};

    s.content = arena_push(arena, length, align_of(char));
    panic_if_not(s.content != NULL);

    s.length = length;
    return s;
}

String string_substring(String s1, size_t start, size_t end)
{
    String s2 = {0};
    panic_if_not(end   <= s1.length + 1);
    panic_if_not(start <= s1.length    );
    panic_if_not(start < end           );
    s2.content = &s1.content[start];
    s2.length = end - 1 - start;
    return s2;
}

String string_concat(Arena *arena, String s1, String s2)
{
    size_t i, j;
    String s3 = string_new(arena, s1.length + s2.length);

    char *ptr = (char*)s3.content;
    for(i = 0; i < s1.length; i++) {
        ptr[i] = s1.content[i];
    }
    for(j = 0; j < s2.length; j++) {
        ptr[i] = s2.content[j];
    }

    return s3;
}

bool string_equals(String s1, String s2)
{
    size_t i;
    if(s1.length != s2.length) {
        return false;
    }
    for(i = 0; i < s1.length; i++) {
        if(s1.content[i] != s2.content[i]) {
            return false;
        }
    }
    return true;
}

void string_print_raw(String s)
{
    printf("\"");
    for (size_t i = 0; i <= s.length; i++)
    {
        switch (s.content[i])
        {
        case '\\':
            printf("\\\\");
            break;
        case '\"':
            printf("\\\"");
            break;
        case '\n':
            printf("\\n");
            break;
        case '\t':
            printf("\\t");
            break;
        default:
            printf("%c", s.content[i]);
            break;
        }
    }
    printf("\"");
}