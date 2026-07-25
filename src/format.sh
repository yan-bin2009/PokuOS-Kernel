#!/bin/bash
# 批量将 .c .h .asm 文件的行首缩进统一为 8 格制表符

find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.asm" \) -exec sh -c '
    for f do
        unexpand --first-only -t 8 "$f" > "$f.tmp" && mv "$f.tmp" "$f"
    done
' sh {} +
