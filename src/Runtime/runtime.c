// =============================================================================
// mellis/Runtime/runtime.c
//
// Minimal C runtime for mellis MVP.
// =============================================================================

#include <stdio.h>

void print(int val) {
    printf("%d\n", val);
}

#include <stdbool.h>

extern bool mellis_coro_is_done(void* handle);
extern void mellis_coro_resume(void* handle);

void mellis_block_on(void* handle) {
    while (!mellis_coro_is_done(handle)) {
        mellis_coro_resume(handle);
    }
}
