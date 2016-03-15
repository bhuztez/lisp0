.text
.global _start
_start:
        xorq %rbp,%rbp
        movq %rsp,%rdi
        movq $(c_stack+2048),%rsp
        call lisp0_start
loop:
        movq %rax,%rdi
        movq $60,%rax
        syscall
        jmp loop

.global syscall
syscall:
        movq %rdi,%rax
        movq %rsi,%rdi
        movq %rdx,%rsi
        movq %rcx,%rdx
        movq %r8,%r10
        movq %r9,%r8
        movq 8(%rsp),%r9
        syscall
        ret
