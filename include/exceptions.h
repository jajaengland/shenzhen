#pragma once
#include <stdint.h>

void exceptions_init(void);

/* Interrupt frame pushed by CPU + our stubs */
typedef struct {
    /* pushed by exception stubs */
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    /* pushed by CPU (or stub for exceptions without error code) */
    uint32_t int_no, err_code;
    /* pushed by CPU automatically */
    uint32_t eip, cs, eflags;
} int_frame_t;
