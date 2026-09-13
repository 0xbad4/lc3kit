.ORIG x3000
    AND  R0, R0, #0  ; R0 = 0
    ADD  R0, R0, #4  ; R0 = 4
    SHL  R1, R0, #2  ; R1 = 16
    SHR  R2, R1, #1  ; R2 = 8
    MUL  R3, R0, R2  ; R3 = 32
    DIV  R4, R3, R0  ; R4 = 8
    TRAP x25         ; HALT
.END
