#!/bin/bash

# MXFS Verilog テスト実行スクリプト

set -e

echo "=========================================="
echo "MXFS Verilog Test Suite"
echo "=========================================="
echo ""

PASS=0
FAIL=0

run_test() {
    local name=$1
    echo "--- $name ---"
    if iverilog -s "$name" -o sim.out fsm_fast.v fsm_slow.v fsm_selector.v fsm_tb.v 2>&1; then
        if vvp sim.out 2>&1 | grep -q "ALL PASS"; then
            echo "✓ $name: PASS"
            ((PASS++))
        else
            echo "✗ $name: FAIL"
            ((FAIL++))
        fi
    else
        echo "✗ $name: COMPILE ERROR"
        ((FAIL++))
    fi
    echo ""
}

# テスト実行
run_test "mxfs_fast_tb"
run_test "mxfs_slow_tb"
run_test "mxfs_fast_slow_pattern_a_tb"
run_test "mxfs_fast_slow_pattern_b_tb"

# クリーンアップ
rm -f sim.out

# 結果サマリ
echo "=========================================="
echo "Results: $PASS passed, $FAIL failed"
echo "=========================================="

if [ $FAIL -eq 0 ]; then
    exit 0
else
    exit 1
fi
