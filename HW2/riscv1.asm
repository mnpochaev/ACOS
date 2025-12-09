.data
msg_dividend: .asciz "Enter dividend: "
msg_divisor:  .asciz "Enter divisor: "
msg_err:      .asciz "Error: division by zero\n"
msg_q:        .asciz "Quotient = "
msg_r:        .asciz "Remainder = "
newline:      .asciz "\n"

.text
.globl main

main:
    la a0, msg_dividend
    li a7, 4
    ecall

    li a7, 5
    ecall
    mv s0, a0

    la a0, msg_divisor
    li a7, 4
    ecall

    li a7, 5
    ecall
    mv s1, a0

    beq s1, zero, div_zero

    mv s2, s0
    li s7, 0
    blt s2, zero, div_neg
    j div_sign_done

div_neg:
    li s7, 1
    sub s2, zero, s2

div_sign_done:
    mv s3, s1
    li s8, 0
    blt s3, zero, divisor_neg
    j divisor_sign_done

divisor_neg:
    li s8, 1
    sub s3, zero, s3

divisor_sign_done:
    li s4, 0
    beq s7, s8, sign_done
    li s4, 1

sign_done:
    mv s5, s2
    mv s6, zero

div_loop:
    blt s5, s3, div_end
    sub s5, s5, s3
    addi s6, s6, 1
    j div_loop

div_end:
    beqz s4, q_ok
    sub s6, zero, s6

q_ok:
    beqz s7, r_ok
    sub s5, zero, s5

r_ok:
    la a0, msg_q
    li a7, 4
    ecall

    mv a0, s6
    li a7, 1
    ecall

    la a0, newline
    li a7, 4
    ecall

    la a0, msg_r
    li a7, 4
    ecall

    mv a0, s5
    li a7, 1
    ecall

    la a0, newline
    li a7, 4
    ecall

    li a0, 0
    li a7, 10
    ecall

div_zero:
    la a0, msg_err
    li a7, 4
    ecall
    li a0, 1
    li a7, 10
    ecall
