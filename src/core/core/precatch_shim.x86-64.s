# This file is generated from C++:
#   using F = void(void* context);
#   extern "C" void precatch_shim(F f, void* ctx) { f(ctx); }
# And then modified to use custom personality function.

    .section    __TEXT,__text,regular,pure_instructions
    .globl    _precatch_shim
    .align    4, 0x90
_precatch_shim:
    .cfi_startproc
    .cfi_personality 0x9b, _precatch_shim_personality # <-- Added
## BB#0:
    pushq    %rbp
Ltmp0:
    .cfi_def_cfa_offset 16
Ltmp1:
    .cfi_offset %rbp, -16
    movq    %rsp, %rbp
Ltmp2:
    .cfi_def_cfa_register %rbp
    movq    %rdi, %rax
    movq    %rsi, %rdi
    popq    %rbp
    jmpq    *%rax                   ## TAILCALL
    .cfi_endproc

.subsections_via_symbols
