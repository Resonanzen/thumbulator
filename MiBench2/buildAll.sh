#!/bin/sh

find . -d  -type d \( ! -name . \) -exec bash -c "cd {} && make clean && make && cp main.bin ../{}.bin && make clean" \;

mv *.bin binaries
