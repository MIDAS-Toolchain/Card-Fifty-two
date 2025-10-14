set pagination off
set logging file gdb_output.txt
set logging on
run
bt full
info locals
info registers
quit
