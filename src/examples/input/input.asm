.ORIG x3000

TRAP  x23           ; IN  - prompts, reads char, echoes it into R0
PUT
LEA   R0, MSG
TRAP  x22           ; PUTS - print confirmation
TRAP  x25           ; HALT
MSG   .STRINGZ " <- you typed that!\n"
.END