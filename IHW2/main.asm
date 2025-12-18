.include "macrolib.asm"

.data
.align 3
.globl result_msg, iterations_msg, close_bracket, epsilon
menu_prompt:    .asciz "\n=== МЕНЮ ===\n1. Вычислить ln(1-x)\n2. Запустить тесты\n3. Выход\nВыберите пункт (1-3): "
invalid_option: .asciz "Некорректный выбор. Введите 1, 2 или 3.\n"
x_prompt:       .asciz "Введите x (-1 < x < 1): "
error_msg:      .asciz "x должен удовлетворять -1 < x < 1.\n"
result_msg:     .asciz "ln(1-x) = "
manual_header:  .asciz "\n=== РУЧНОЙ РЕЖИМ ===\n"
iterations_msg: .asciz " (итерации: "
close_bracket:  .asciz ")\n"

epsilon:        .double 0.001   # граница точности 0.1%

.text
.globl main

main:
menu_loop:
    # Вывод меню и чтение выбора как целого числа
    PRINT_STRING menu_prompt
    jal input_int              # a0 <- выбранный пункт меню

    li t0, 1
    beq a0, t0, manual_mode
    li t0, 2
    beq a0, t0, test_mode
    li t0, 3
    beq a0, t0, exit_program

    PRINT_STRING invalid_option
    j menu_loop

manual_mode:
    PRINT_STRING manual_header
    PRINT_STRING x_prompt
    jal input_float            # fa0 <- введенное x

    # Проверка диапазона: -1 < x < 1
    fld fa1, float_m1, t0      # нижняя граница
    fld fa2, float_1, t0       # верхняя граница
    jal check_float_range      # a0 = 1 если в диапазоне
    beqz a0, invalid_x

    # Подготовка вызова ряда: fa0 = x, fa1 = epsilon
    fld fa1, epsilon, t0
    jal calculate_ln_series    # fa0 -> ln(1-x), a1 -> количество итераций

    fmv.d fs0, fa0             # сохраняем результат
    mv s0, a1                  # сохраняем число итераций

    PRINT_STRING result_msg
    fmv.d fa0, fs0
    jal print_float

    PRINT_STRING iterations_msg
    mv a0, s0
    jal print_int
    PRINT_STRING close_bracket

    j menu_loop

test_mode:
    PRINT_STRING test_header
    jal run_automated_tests
    j menu_loop

exit_program:
    li a7, 10
    ecall

invalid_x:
    PRINT_STRING error_msg
    j menu_loop
