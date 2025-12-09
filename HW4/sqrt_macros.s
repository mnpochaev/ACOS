        .macro READ_INPUTS
        la      a0, prompt_a
        li      a7, 4
        ecall

        li      a7, 6
        ecall
        fmv.s   fs0, fa0

        la      a0, prompt_eps
        li      a7, 4
        ecall

        li      a7, 6
        ecall
        fmv.s   fs1, fa0
        .end_macro


        .macro CHECK_INPUTS
        fmv.s.x ft0, x0

        fle.s   t0, fs0, ft0
        beqz    t0, check_eps

        la      a0, msg_error_a
        li      a7, 4
        ecall

        li      a0, 1
        li      a7, 93
        ecall

check_eps:
        fle.s   t0, fs1, ft0
        beqz    t0, end_check

        la      a0, msg_error_eps
        li      a7, 4
        ecall

        li      a0, 1
        li      a7, 93
        ecall

end_check:
        .end_macro


        .macro CALL_SQRT
        fmv.s   fa0, fs0
        fmv.s   fa1, fs1
        jal     ra, sqrt_iter
        fmv.s   fs2, fa0
        .end_macro


        .macro PRINT_RESULT
        la      a0, msg_result
        li      a7, 4
        ecall

        fmv.s   fa0, fs2
        li      a7, 2
        ecall

        la      a0, msg_newline
        li      a7, 4
        ecall
        .end_macro
