#!/bin/bash
# Brick Performance Benchmarks
# Mede tempo de compilação, alocação, reset, etc.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
BRICK="$BUILD_DIR/brick"

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m'

mkdir -p "$BUILD_DIR"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Brick Benchmarks${NC}"
echo -e "${CYAN}========================================${NC}"

if [ ! -f "$BRICK" ]; then
    echo -e "${RED}Compiler not found. Run 'scons' first.${NC}"
    exit 1
fi

echo ""
echo -e "${YELLOW}Benchmark 1: Compilation time${NC}"

# Generate a .brc file with N structs/functions
generate_large_mc() {
    local n=$1
    local file=$2
    echo "package BENCH" > "$file"
    echo "block global = 64MB" >> "$file"
    for ((i=0; i<n; i++)); do
        echo "struct Struct$i {" >> "$file"
        echo "    int field$i" >> "$file"
        echo "    fn Struct$i(int v) { field$i = v; }" >> "$file"
        echo "    fn get_$i() -> int { return field$i; }" >> "$file"
        echo "}" >> "$file"
        echo "" >> "$file"
    done
    echo "fn main() {" >> "$file"
    echo "    global.reset()" >> "$file"
    echo "}" >> "$file"
}

generate_large_mc 100 "$BUILD_DIR/bench_100.brc"

echo -n "  Compiling 100 structs... "
start=$(date +%s%N)
$BRICK "$BUILD_DIR/bench_100.brc" -o "$BUILD_DIR/bench_100.c" 2>/dev/null
end=$(date +%s%N)
elapsed=$(( (end - start) / 1000000 ))
echo -e "${GREEN}${elapsed}ms${NC}"

echo ""
echo -e "${YELLOW}Benchmark 2: Runtime allocator${NC}"

# Create benchmark C program
cat > "$BUILD_DIR/bench_alloc.c" << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "block_memory.h"

#define ALLOC_COUNT 1000000
#define ALLOC_SIZE  64

int main() {
    clock_t start, end;
    double cpu_time;

    // Benchmark block allocator
    BlockCtx* block = block_create(256);
    start = clock();
    for (int i = 0; i < ALLOC_COUNT; i++) {
        void* p = block_alloc(block, ALLOC_SIZE);
        (void)p;
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  Block alloc: %d allocs of %dB in %.3fs\n", ALLOC_COUNT, ALLOC_SIZE, cpu_time);

    // Benchmark malloc
    void** ptrs = malloc(ALLOC_COUNT * sizeof(void*));
    start = clock();
    for (int i = 0; i < ALLOC_COUNT; i++) {
        ptrs[i] = malloc(ALLOC_SIZE);
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  malloc:      %d allocs of %dB in %.3fs\n", ALLOC_COUNT, ALLOC_SIZE, cpu_time);

    // Benchmark block reset
    start = clock();
    for (int i = 0; i < 100000; i++) {
        block_reset(block);
        block_alloc(block, ALLOC_SIZE);
    }
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("  Block reset: 100k resets + allocs in %.3fs\n", cpu_time);

    free(ptrs);
    block_destroy(block);
    return 0;
}
EOF

gcc -O3 "$BUILD_DIR/bench_alloc.c" "$PROJECT_DIR/runtime/block_memory.c" -o "$BUILD_DIR/bench_alloc" -I"$PROJECT_DIR/runtime"

if [ -f "$BUILD_DIR/bench_alloc" ]; then
    "$BUILD_DIR/bench_alloc"
else
    echo -e "${RED}  Could not compile benchmark${NC}"
fi

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${GREEN}Benchmarks done!${NC}"
echo -e "${CYAN}========================================${NC}"
