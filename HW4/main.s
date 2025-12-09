        .include "sqrt_macros.s"

        .data
prompt_a:      .asciz "Введите положительное число a: "
prompt_eps:    .asciz "Введите требуемую точность eps (> 0): "
msg_result:    .asciz "Приближённый квадратный корень: "
msg_newline:   .asciz "\n"
msg_error_a:   .asciz "Ошибка: число a должно быть строго больше 0.\n"
msg_error_eps: .asciz "Ошибка: точность eps должна быть строго больше 0.\n"

        .text
        .globl main
main:
        READ_INPUTS
        CHECK_INPUTS
        CALL_SQRT
        PRINT_RESULT

        li      a0, 0
        li      a7, 93
        ecall
