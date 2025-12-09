.data
msg_n:    .asciz "Максимальное n, для которого n! помещается в 32-битном слове: "
msg_fact: .asciz "\nЗначение факториала этого n: "
newline:  .asciz "\n"

.text
.globl main

.macro FACT(%n_reg, %res_reg)
    mv   a0, %n_reg
    jal  factorial
    mv   %res_reg, a0
.end_macro

main:
    li   s0, 1
    li   s1, 1

find_max_loop:
    addi t0, s0, 1
    mulhu t1, s1, t0
    bnez  t1, found_max
    mul   s1, s1, t0
    addi  s0, s0, 1
    j     find_max_loop

found_max:
    mv   t2, s0
    FACT(t2, t3)

    la   a0, msg_n
    li   a7, 4
    ecall

    mv   a0, t2
    li   a7, 1
    ecall

    la   a0, msg_fact
    li   a7, 4
    ecall

    mv   a0, t3
    li   a7, 1
    ecall

    la   a0, newline
    li   a7, 4
    ecall

    li   a7, 10
    ecall

factorial:
    addi sp, sp, -8
    sw   ra, 4(sp)
    sw   a0, 0(sp)

    li   t0, 1
    ble  a0, t0, fact_base_case

    addi a0, a0, -1
    jal  factorial
    lw   t1, 0(sp)
    mul  a0, a0, t1
    j    fact_end

fact_base_case:
    li   a0, 1

fact_end:
    lw   ra, 4(sp)
    addi sp, sp, 8
    ret