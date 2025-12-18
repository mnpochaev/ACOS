# Унифицированные подпрограммы ввода-вывода для RARS с поддержкой double

.data
.align 3
.globl float_0, float_1, float_m1
space:          .asciz " "
newline_ch:     .asciz "\n"
error_m:        .asciz "Error: Invalid input!\n"

# Константы с плавающей точкой (double), общие для модулей
float_0:        .double 0.0
float_1:        .double 1.0
float_m1:       .double -1.0

.text

# Ввод целого числа с консоли -> a0
.globl input_int
input_int:
    addi sp, sp, -8
    sw ra, 4(sp)

    li a7, 5                  # системный вызов read_int
    ecall

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

# Ввод double -> fa0
.globl input_float
input_float:
    addi sp, sp, -8
    sw ra, 4(sp)

    li a7, 7                  # системный вызов read_double
    ecall

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

# Печать целого из a0
.globl print_int
print_int:
    addi sp, sp, -8
    sw ra, 4(sp)

    li a7, 1                  # системный вызов print_int
    ecall

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

# Печать double из fa0
.globl print_float
print_float:
    addi sp, sp, -8
    sw ra, 4(sp)

    li a7, 3                  # системный вызов print_double
    ecall

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

# Печать строки по адресу a0
.globl print_string
print_string:
    addi sp, sp, -8
    sw ra, 4(sp)

    li a7, 4                  # системный вызов print_string
    ecall

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

# Вывод перевода строки
.globl newline
newline:
    addi sp, sp, -8
    sw ra, 4(sp)

    la a0, newline_ch
    li a7, 4
    ecall

    lw ra, 4(sp)
    addi sp, sp, 8
    ret

# Проверка: fa0 внутри диапазона [fa1, fa2]; результат a0=1 если да, иначе 0
.globl check_float_range
check_float_range:
    addi sp, sp, -32
    sw ra, 24(sp)
    fsd fs0, 16(sp)
    fsd fs1, 8(sp)

    fmv.d fs0, fa0            # проверяемое значение
    fmv.d fs1, fa1            # нижняя граница

    fle.d t0, fa1, fa0        # нижняя граница <= значение
    fmv.d fa0, fs0
    fle.d t1, fa0, fa2        # значение <= верхняя граница

    and a0, t0, t1            # 1, если обе проверки истинны

    fld fs0, 16(sp)
    fld fs1, 8(sp)
    lw ra, 24(sp)
    addi sp, sp, 32
    ret

# Модуль числа с плавающей точкой (double): |fa0| -> fa0
.globl fabs
fabs:
    addi sp, sp, -8
    sw ra, 4(sp)

    fld ft0, float_0, t0
    flt.d t0, fa0, ft0
    beqz t0, fabs_end
    fneg.d fa0, fa0

fabs_end:
    lw ra, 4(sp)
    addi sp, sp, 8
    ret
