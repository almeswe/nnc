.intel_syntax noprefix

.text

.global _start
_start:
    endbr64
    xor  ebp, ebp
    mov  r9, rdx
    pop  rdi
    pop  rsi
    mov  rdx, rsp
    and  rsp, 0xfffffffffffffff0
    push rax
    push rsp
    xor  r8d, r8d
    xor  ecx, ecx
    call main
    mov  rdi, rax
    mov  rax, 60
    syscall
    hlt
