# Библиотека математических функций для вычисления ln(1-x) (double)

.data
.align 3
f1:       .double 1.0
f0:       .double 0.0

.text

# calculate_ln_series (double)
# Вход:
#   fa0 = x (|x| < 1)
#   fa1 = epsilon
# Выход:
#   fa0 = ln(1 - x)
#   a1  = число итераций
.globl calculate_ln_series
calculate_ln_series:
    addi sp, sp, -80
    fsd fs0, 72(sp)           # term
    fsd fs1, 64(sp)           # sum
    fsd fs2, 56(sp)           # x
    fsd fs3, 48(sp)           # epsilon
    fsd fs4, 40(sp)           # x_power
    fsd fs5, 32(sp)           # n как double
    sw ra, 24(sp)
    sw s0, 20(sp)             # n (int)
    sw s1, 16(sp)             # iteration_count

    # Сохраняем входные параметры
    fmv.d fs2, fa0            # x
    fmv.d fs3, fa1            # epsilon

    # Инициализация
    fld fs1, float_0, t0      # sum = 0.0
    fmv.d fs4, fs2            # x_power = x
    li s0, 1                  # n = 1
    li s1, 0                  # iterations = 0

series_loop:
    # term = x_power / n
    fcvt.d.w fs5, s0
    fdiv.d fs0, fs4, fs5

    # sum = sum - term
    fsub.d fs1, fs1, fs0

    # Проверка точности: |term| < epsilon
    jal fabs_inline           # ft0 = |term|
    flt.d t0, ft0, fs3
    bnez t0, series_end

    # Переход к следующему члену: x_power *= x
    fmul.d fs4, fs4, fs2

    addi s0, s0, 1
    addi s1, s1, 1

    # Ограничение числа итераций
    li t0, 1000
    blt s1, t0, series_loop

series_end:
    fmv.d fa0, fs1            # результат
    mv a1, s1                 # итерации

    fld fs0, 72(sp)
    fld fs1, 64(sp)
    fld fs2, 56(sp)
    fld fs3, 48(sp)
    fld fs4, 40(sp)
    fld fs5, 32(sp)
    lw ra, 24(sp)
    lw s0, 20(sp)
    lw s1, 16(sp)
    addi sp, sp, 80
    ret

# |fs0| -> ft0 (double)
fabs_inline:
    fld ft1, float_0, t0
    flt.d t1, fs0, ft1
    beqz t1, fabs_inline_end
    fneg.d ft0, fs0
    ret
fabs_inline_end:
    fmv.d ft0, fs0
    ret
