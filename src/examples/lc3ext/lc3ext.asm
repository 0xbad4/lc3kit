.ORIG x3000
    AND  R0, R0, #0
    ADD  R0, R0, #4
    SHL  R1, R0, #2
    SHR  R2, R1, #1
    MUL  R3, R0, R2
    DIV  R4, R3, R0
    TRAP x25         ; HALT
.END
