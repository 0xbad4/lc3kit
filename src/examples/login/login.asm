.ORIG x3000

; print username prompt
        LEA  R0, UN_PROMPT
        TRAP x22            ; PUTS

; read username into UN_BUF, one char at a time via IN (echoed)
        LEA  R1, UN_BUF     ; R1 = write pointer into buffer
UN_LOOP
        TRAP x23            ; IN  - reads char, echoes it, result in R0
        ADD  R2, R0, #0
        LD   R3, ENTER
        NOT  R3, R3
        ADD  R3, R3, #1     ; R3 = -ENTER
        ADD  R3, R2, R3     ; R3 = char - ENTER
        BRz  UN_DONE        ; if char == Enter, end username
        STR  R0, R1, #0     ; store char into buffer
        ADD  R1, R1, #1     ; advance pointer
        BR   UN_LOOP
UN_DONE
        AND  R0, R0, #0
        STR  R0, R1, #0     ; null-terminate username

; print password prompt
        LEA  R0, PW_PROMPT
        TRAP x22            ; PUTS

; read password into PW_BUF via GETC (not echoed)
        LEA  R1, PW_BUF     ; R1 = write pointer into buffer
PW_LOOP
        TRAP x20            ; GETC - reads char, NO echo, result in R0
        ADD  R2, R0, #0
        LD   R3, ENTER
        NOT  R3, R3
        ADD  R3, R3, #1     ; R3 = -ENTER
        ADD  R3, R2, R3     ; R3 = char - ENTER
        BRz  PW_DONE        ; if char == Enter, end password
        STR  R0, R1, #0     ; store char into buffer
        ADD  R1, R1, #1     ; advance pointer
        BR   PW_LOOP
PW_DONE
        AND  R0, R0, #0
        STR  R0, R1, #0     ; null-terminate password

; newline after password entry
        LEA  R0, NEW_LINE
        TRAP x22            ; PUTS

; print result: username:password
        LEA  R0, UN_BUF
        TRAP x22            ; PUTS username

        LEA  R0, SEPARATOR
        TRAP x22            ; PUTS ":"

        LEA  R0, PW_BUF
        TRAP x22            ; PUTS password

        LEA  R0, NEW_LINE
        TRAP x22            ; PUTS newline

        TRAP x25            ; HALT

ENTER      .FILL x000A      ; newline character (LF)
UN_PROMPT  .STRINGZ "username: "
PW_PROMPT  .STRINGZ "password: "
SEPARATOR  .STRINGZ ":"
NEW_LINE   .STRINGZ "\n"

; buffers -- 32 chars each, more than enough for a demo
UN_BUF     .BLKW #32
PW_BUF     .BLKW #32

.END