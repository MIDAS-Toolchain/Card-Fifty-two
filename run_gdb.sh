#!/bin/bash
# Run the program in gdb and wait for crash
# Press D to deal a card and trigger the crash
# This will show the backtrace

gdb -ex "run" -ex "bt full" -ex "info locals" -ex "quit" ./bin/card-fifty-two
