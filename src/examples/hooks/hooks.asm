.ORIG x3000
    AND  R0, R0, #0
    ADD  R0, R0, #7
    ST   R0, VAL        ; memory write -- hook fires
    LD   R1, VAL        ; memory read  -- hook fires
    TRAP x25
    VAL  .BLKW #1
.END