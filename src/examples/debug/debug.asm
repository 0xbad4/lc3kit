.ORIG x3000
    AND  R0, R0, #0     ; R0 = 0
    ADD  R0, R0, #1     ; R0 = 1
    ADD  R0, R0, R0     ; R0 = 2
    ADD  R0, R0, R0     ; R0 = 4
    ADD  R0, R0, R0     ; R0 = 8
    ADD  R0, R0, R0     ; R0 = 16
    TRAP x25            ; HALT
    .END