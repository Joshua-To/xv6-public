#!/bin/bash
# AI Daemon Patch Testing Script
# Demonstrates patch generation and application
#
# Usage: run this AFTER xv6 starts and is at a shell prompt
# Example xv6 session:
#   make qemu
#   (wait for xv6 shell)
#   monitor start 180 telemetry
#   test_patches
#   (watch output for patches being generated and applied)

echo "=========================================="
echo "AI Daemon Patch Testing Suite"
echo "=========================================="
echo ""
echo "Prerequisites:"
echo "  1. make qemu should be running"
echo "  2. You should be at xv6 shell prompt"
echo "  3. Run: monitor start 180 telemetry"
echo "  4. Run: test_patches"
echo ""
echo "This script will run 3 tests to trigger patch generation:"
echo ""

echo "Test 1: Excessive Syscalls (should trigger after 10 seconds)"
echo "  Expected: AI daemon detects 500+ syscalls → generates patch"
test_excessive_syscalls &
EXCESS_PID=$!

echo ""
sleep 5

echo "Test 2: Slow Syscalls (should trigger after 20 seconds)"
echo "  Expected: AI daemon detects slow calls (>2000µs) → generates patch"
test_slow_syscalls &
SLOW_PID=$!

echo ""
sleep 5

echo "Test 3: Memory Spike (should trigger after 30 seconds)"
echo "  Expected: AI daemon detects 256KB+ allocation → generates patch"
test_memory_spike &
MEM_PID=$!

echo ""
echo "=========================================="
echo "Tests running in background..."
echo "Watch the AI daemon output for:"
echo "  [TELEMETRY] - samples being collected"
echo "  [PATTERN DETECTED] - anomalies found"
echo "  [PATCH ELIGIBLE] - patch will be generated"
echo "  [PATCH GENERATED] - patch created"
echo "  [SUCCESS] - patch applied to kernel"
echo "=========================================="
echo ""

# Wait for all tests to complete
wait $EXCESS_PID
wait $SLOW_PID
wait $MEM_PID

echo ""
echo "All tests completed!"
echo ""
echo "Next steps:"
echo "  1. monitor stop  - to stop monitoring and see final stats"
echo "  2. Reboot xv6 to see patches persist"
echo "     (patches.db file should contain applied patches)"
