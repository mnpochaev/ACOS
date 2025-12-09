        .data
one:    .float 1.0
half:   .float 0.5

        .text
        .globl sqrt_iter
sqrt_iter:
        la      t0, one
        flw     ft0, 0(t0)

        la      t0, half
        flw     ft1, 0(t0)

        fmv.s   ft2, fa0
        flt.s   t1, fa0, ft0
        beqz    t1, init_done
        fmv.s   ft2, ft0

init_done:
        fmul.s  ft4, fa1, fa1

iter_loop:
        fdiv.s  ft3, fa0, ft2
        fadd.s  ft3, ft3, ft2
        fmul.s  ft3, ft3, ft1

        fsub.s  ft5, ft3, ft2
        fmul.s  ft5, ft5, ft5

        fle.s   t2, ft5, ft4
        bnez    t2, done

        fmv.s   ft2, ft3
        j       iter_loop

done:
        fmv.s   fa0, ft3
        ret
