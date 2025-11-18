#!/bin/bash
gdb -ex "run" -ex "bt full" -ex "info locals" -ex "quit" ./bin/card-fifty-two
