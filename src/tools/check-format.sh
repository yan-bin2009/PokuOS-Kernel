#!/usr/bin/env bash

set -u

SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$SRC_DIR" || exit 1

MODE="check"
if [ "${1:-}" = "--fix" ]; then
        MODE="fix"
fi

CFILES=$(find kernel init driver user -name '*.c' -o -name '*.h' | sort)
ALLSRC=$(find kernel init driver user \
        -type f \( -name '*.c' -o -name '*.h' -o -name '*.s' -o -name '*.asm' \) | sort)

FAIL=0

check_trailing_ws()
{
        local f
        for f in $ALLSRC; do
                if grep -qE '[[:blank:]]+$' "$f"; then
                        echo "[trailing-ws] $f"
                        FAIL=1
                fi
        done
}

fix_trailing_ws()
{
        local f
        for f in $ALLSRC; do
                if grep -qE '[[:blank:]]+$' "$f"; then
                        sed -i 's/[[:blank:]]*$//' "$f"
                        echo "[fixed trailing-ws] $f"
                fi
        done
}

check_tabs()
{
        local f
        for f in $CFILES; do
                if grep -qP '\t' "$f"; then
                        echo "[tab] $f"
                        FAIL=1
                fi
        done
}

check_clang_format()
{
        if ! command -v clang-format >/dev/null 2>&1; then
                echo "[skip] clang-format 未安装，跳过 C 文件格式检查" >&2
                return
        fi
        if ! clang-format --dry-run --Werror $CFILES >/dev/null 2>&1; then
                echo "[clang-format] 以下文件不符合 .clang-format："
                clang-format --dry-run $CFILES 2>&1 | grep -oE '^[^:]+' | sort -u
                FAIL=1
        fi
}

fix_clang_format()
{
        if ! command -v clang-format >/dev/null 2>&1; then
                echo "[skip] clang-format 未安装" >&2
                return
        fi
        clang-format -i $CFILES
        echo "[fixed] clang-format -i"
}

if [ "$MODE" = "fix" ]; then
        echo "== 自动修复 =="
        fix_clang_format
        fix_trailing_ws
        echo "== 修复完成，重新检查 =="
        FAIL=0
        check_tabs
        check_clang_format
        check_trailing_ws
else
        echo "== 格式检查 =="
        check_tabs
        check_clang_format
        check_trailing_ws
fi

if [ "$FAIL" -ne 0 ]; then
        echo "== 有格式问题，见上（--fix 可自动修复） =="
        exit 1
fi
echo "== 全部通过 =="
exit 0
