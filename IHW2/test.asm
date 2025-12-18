
.include "macrolib.asm"

.data
.align 3
.globl run_automated_tests
.globl test_header
test_header:    .asciz "\n=== ПРОХОЖДЕНИЕ АВТОТЕСТОВ ===\n"
test1_msg:      .asciz "\nТест 1: x = 0.1\n"
test2_msg:      .asciz "\nТест 2: x = 0.5\n"
test3_msg:      .asciz "\nТест 3: x = -0.3\n"
expected_msg:   .asciz "Ожидается: "
passed_msg:     .asciz "ПРОЙДЕНО\n"
failed_msg:     .asciz "ОШИБКА\n"

# Тестовые значения
test1_x:        .double 0.1
test2_x:        .double 0.5
test3_x:        .double -0.3

# Ожидаемые значения (для проверки)
expected1:      .double -0.105360  # ln(0.9)
expected2:      .double -0.693147  # ln(0.5)
expected3:      .double 0.262364   # ln(1.3)

.text

run_automated_tests:
    addi sp, sp, -8
    sw ra, 4(sp)

    PRINT_STRING test_header

    # Тесты
    jal test1
    jal test2
    jal test3

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

test1:
    addi sp, sp, -48
    fsd fs0, 40(sp)
    sw ra, 32(sp)
    sw s0, 28(sp)
    sw s1, 24(sp)

    PRINT_STRING test1_msg

    fld fa0, test1_x, t0
    fld fa1, epsilon, t0
    jal calculate_ln_series

    fmv.d fs0, fa0            # результат
    mv s0, a1                 # итерации

    PRINT_STRING result_msg
    fmv.d fa0, fs0
    jal print_float

    PRINT_STRING iterations_msg
    mv a0, s0
    jal print_int
    PRINT_STRING close_bracket

    # Проверка точности
    fld ft0, expected1, t0    # ожидаемое
    fsub.d ft1, fs0, ft0      # разность

    fld ft2, float_0, t0
    flt.d t1, ft1, ft2
    beqz t1, diff_positive1
    fneg.d ft1, ft1
 diff_positive1:
    fld ft2, epsilon, t0
    flt.d t0, ft1, ft2
    bnez t0, test1_pass

    PRINT_STRING failed_msg
    j test1_end

test1_pass:
    PRINT_STRING passed_msg

test1_end:
    fld fs0, 40(sp)
    lw ra, 32(sp)
    lw s0, 28(sp)
    lw s1, 24(sp)
    addi sp, sp, 48
    ret

test2:
    addi sp, sp, -48
    fsd fs0, 40(sp)
    sw ra, 32(sp)
    sw s0, 28(sp)
    sw s1, 24(sp)

    PRINT_STRING test2_msg

    fld fa0, test2_x, t0
    fld fa1, epsilon, t0
    jal calculate_ln_series

    fmv.d fs0, fa0
    mv s0, a1

    PRINT_STRING result_msg
    fmv.d fa0, fs0
    jal print_float

    PRINT_STRING iterations_msg
    mv a0, s0
    jal print_int
    PRINT_STRING close_bracket

    fld ft0, expected2, t0
    fsub.d ft1, fs0, ft0

    fld ft2, float_0, t0
    flt.d t1, ft1, ft2
    beqz t1, diff_positive2
    fneg.d ft1, ft1
 diff_positive2:
    fld ft2, epsilon, t0
    flt.d t0, ft1, ft2
    bnez t0, test2_pass

    PRINT_STRING failed_msg
    j test2_end

test2_pass:
    PRINT_STRING passed_msg

test2_end:
    fld fs0, 40(sp)
    lw ra, 32(sp)
    lw s0, 28(sp)
    lw s1, 24(sp)
    addi sp, sp, 48
    ret

test3:
    addi sp, sp, -48
    fsd fs0, 40(sp)
    sw ra, 32(sp)
    sw s0, 28(sp)
    sw s1, 24(sp)

    PRINT_STRING test3_msg

    fld fa0, test3_x, t0
    fld fa1, epsilon, t0
    jal calculate_ln_series

    fmv.d fs0, fa0
    mv s0, a1

    PRINT_STRING result_msg
    fmv.d fa0, fs0
    jal print_float

    PRINT_STRING iterations_msg
    mv a0, s0
    jal print_int
    PRINT_STRING close_bracket

    fld ft0, expected3, t0
    fsub.d ft1, fs0, ft0

    fld ft2, float_0, t0
    flt.d t1, ft1, ft2
    beqz t1, diff_positive3
    fneg.d ft1, ft1
 diff_positive3:
    fld ft2, epsilon, t0
    flt.d t0, ft1, ft2
    bnez t0, test3_pass

    PRINT_STRING failed_msg
    j test3_end

test3_pass:
    PRINT_STRING passed_msg

test3_end:
    fld fs0, 40(sp)
    lw ra, 32(sp)
    lw s0, 28(sp)
    lw s1, 24(sp)
    addi sp, sp, 48
    ret
