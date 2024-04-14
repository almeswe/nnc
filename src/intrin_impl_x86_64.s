.intel_syntax noprefix

# Provides intrinsics for few useful x64 assembly instructions.

.text

.global ___rdtsc 
___rdtsc:
    push rdx
    rdtsc
    shl rax, 32
    or  rax, rdx
    pop rdx
    ret
    