// Original file is iob_shim.asm
// Rewritten in C because the build system breaks on linux-windows crosscomp with this
#if 0
.code
EXTRN __acrt_iob_func:PROC

__iob_func PROC
    mov     ecx, 0
    jmp     __acrt_iob_func
__iob_func ENDP

END
#endif

extern void* __acrt_iob_func(unsigned int);

void* __iob_func(void)
{
  return __acrt_iob_func(0);
}
