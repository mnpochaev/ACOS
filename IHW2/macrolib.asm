# Набор служебных макросов для RISC-V (RARS) с поддержкой FPU
# Интерфейсы оставлены стабильными для повторного использования в модулях (двойная точность).

# Сохранение регистров, которые используем как локальные
.macro SAVE_REGISTERS
    addi sp, sp, -32
    sw ra, 28(sp)
    sw s0, 24(sp)
    sw s1, 20(sp)
    sw s2, 16(sp)
    sw s3, 12(sp)
    sw s4, 8(sp)
    sw s5, 4(sp)
    sw s6, 0(sp)
.end_macro

# Восстановление сохраненных регистров
.macro RESTORE_REGISTERS
    lw s6, 0(sp)
    lw s5, 4(sp)
    lw s4, 8(sp)
    lw s3, 12(sp)
    lw s2, 16(sp)
    lw s1, 20(sp)
    lw s0, 24(sp)
    lw ra, 28(sp)
    addi sp, sp, 32
.end_macro

# Сохранение временных регистров FPU (double)
.macro SAVE_FLOAT_REGISTERS
    addi sp, sp, -64
    fsd fs0, 56(sp)
    fsd fs1, 48(sp)
    fsd fs2, 40(sp)
    fsd fs3, 32(sp)
    fsd fs4, 24(sp)
    fsd fs5, 16(sp)
    fsd fs6, 8(sp)
    fsd fs7, 0(sp)
.end_macro

# Восстановление регистров FPU (double)
.macro RESTORE_FLOAT_REGISTERS
    fld fs7, 0(sp)
    fld fs6, 8(sp)
    fld fs5, 16(sp)
    fld fs4, 24(sp)
    fld fs3, 32(sp)
    fld fs2, 40(sp)
    fld fs1, 48(sp)
    fld fs0, 56(sp)
    addi sp, sp, 64
.end_macro

# Печать строки с известным на этапе ассемблирования адресом
.macro PRINT_STRING %str_addr
    la a0, %str_addr
    li a7, 4
    ecall
.end_macro

# Печать целого из регистра
.macro PRINT_INT %reg
    mv a0, %reg
    li a7, 1
    ecall
.end_macro

# Печать числа с плавающей точкой (double) из FP-регистра
.macro PRINT_FLOAT %reg
    fmv.d fa0, %reg
    li a7, 3
    ecall
.end_macro

# Ввод числа с плавающей точкой (double) в fa0
.macro INPUT_FLOAT
    li a7, 7
    ecall
.end_macro
