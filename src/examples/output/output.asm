.ORIG x3000
LEA   R0, MSG       ; R0 = address of string
TRAP  x22           ; PUTS
TRAP  x25           ; HALT
MSG   .STRINGZ "Every fall is a chance to rise.\n"
.END
