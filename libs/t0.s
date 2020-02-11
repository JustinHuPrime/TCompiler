  .file   "t0.s"
  .text
  .globl  _start

_start:
  mov %rsp, %rbp
  mov 0(%rbp), %rdi
  lea 8(%rbp), %rsi
  lea 16(%rsp, %rdi, 8), %rdx
  call main
  mov %rax, %rdi
  mov $60, %rax
  syscall
