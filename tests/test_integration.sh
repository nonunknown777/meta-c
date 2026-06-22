#!/bin/bash
# Brick Integration Test
# Teste de Integracao Brick
# Compila um .brc → .c → gcc → executa binário e verifica saída
# Compila um .brc → .c → gcc → executa binario e verifica saida

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
BRICK="$BUILD_DIR/brick"
RUNTIME="$PROJECT_DIR/runtime/block_memory.c $PROJECT_DIR/runtime/hot_reload.c $PROJECT_DIR/runtime/io.c"
PASS=0
FAIL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

mkdir -p "$BUILD_DIR"

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}  Brick Integration Tests${NC}"
echo -e "${CYAN}  Testes de Integracao Brick${NC}"
echo -e "${CYAN}========================================${NC}"

# Check if compiler exists
# Verifica se o compilador existe
if [ ! -f "$BRICK" ]; then
    echo -e "${RED}Compiler not found. Run 'scons' first.${NC}"
    exit 1
fi

test_compile_and_run() {
    local name="$1"
    local brc_file="$2"
    local c_file="$BUILD_DIR/${name}.c"
    local bin_file="$BUILD_DIR/${name}"

    echo -ne "  ${name}... "

    # Compile .brc → .c
    # Compila .brc → .c
    $BRICK "$brc_file" -o "$c_file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (compiler error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Compile .c → binary
    # Compila .c → binario
    gcc -O3 -I"$PROJECT_DIR/runtime" "$c_file" $RUNTIME -o "$bin_file" -ldl 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (gcc error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Run binary
    # Executa binario
    output=$("$bin_file" 2>&1)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}FAIL (exit code $exit_code)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    echo -e "${GREEN}PASS${NC}"
    PASS=$((PASS + 1))
    return 0
}

test_compile_and_expect() {
    local name="$1"
    local brc_file="$2"
    local expected="$3"
    local c_file="$BUILD_DIR/${name}.c"
    local bin_file="$BUILD_DIR/${name}"

    echo -ne "  ${name}... "

    # Compile .brc → .c
    # Compila .brc → .c
    $BRICK "$brc_file" -o "$c_file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (compiler error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Compile .c → binary
    # Compila .c → binario
    gcc -O3 -I"$PROJECT_DIR/runtime" "$c_file" $RUNTIME -o "$bin_file" -ldl 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "${RED}FAIL (gcc error)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    # Run binary and check output
    # Executa binario e verifica saida
    output=$("$bin_file" 2>&1)
    local exit_code=$?

    if [ $exit_code -ne 0 ]; then
        echo -e "${RED}FAIL (exit code $exit_code)${NC}"
        FAIL=$((FAIL + 1))
        return 1
    fi

    if [ "$output" != "$expected" ]; then
        echo -e "${RED}FAIL (output mismatch)${NC}"
        echo "  Expected: $expected"
        echo "  Got:      $output"
        FAIL=$((FAIL + 1))
        return 1
    fi

    echo -e "${GREEN}PASS${NC}"
    PASS=$((PASS + 1))
    return 0
}

echo ""
echo "Compiling and running examples..."
echo "Compilando e executando exemplos..."

# Test with a simple .brc example
# Testa com um exemplo .brc simples
# Create a minimal test that compiles but doesn't need I/O
# Cria um teste minimo que compila mas nao precisa de I/O
cat > "$BUILD_DIR/test_simple.brc" << 'EOF'
package TEST

block global = 1MB

struct Test {
    int value

    fn Test(int v) {
        value = v
    }

    fn get_value() -> int {
        return value
    }
}

fn main() {
    Test t = Test(42) @global
    int x = t.get_value()
    global.reset()
}
EOF

test_compile_and_run "test_simple" "$BUILD_DIR/test_simple.brc"

# Test blocks
# Testa blocos
cat > "$BUILD_DIR/test_blocks.brc" << 'EOF'
package TESTBLOCK

block global = 1MB
block temp = 512KB

struct Item {
    int id
    String name

    fn Item(int i, String n) {
        id = i
        name = n
    }
}

fn main() {
    Item i = Item(1, "test") @temp
    int x = i.id @global
    temp.reset()
    global.reset()
}
EOF

test_compile_and_run "test_blocks" "$BUILD_DIR/test_blocks.brc"

echo ""
echo "Testing IO (print)..."
echo "Testando IO (print)..."

# Test IO: print with various types
# Testa IO: print com varios tipos
cat > "$BUILD_DIR/test_io_print.brc" << 'EOF'
package TEST_IO

using IO

fn main() {
    print("hello")
    print(42)
    print(3.14)
    print(true)
    print('!')
}
EOF

test_compile_and_expect "test_io_print" "$BUILD_DIR/test_io_print.brc" \
"hello
42
3.140000
true
!"

# Test IO: formatted print
# Testa IO: print formatado
cat > "$BUILD_DIR/test_io_format.brc" << 'EOF'
package TEST_IO_FMT

using IO

fn main() {
    print("int={0}, str={1}", 10, "test")
}
EOF

test_compile_and_expect "test_io_format" "$BUILD_DIR/test_io_format.brc" \
"int=10, str=test"

# Test IO: print without using → should fail to compile
# Testa IO: print sem using → deve falhar ao compilar
cat > "$BUILD_DIR/test_io_no_using.brc" << 'EOF'
package TEST_NO_IO

fn main() {
    print("no io")
}
EOF

echo -n "  test_io_no_using... "
$BRICK "$BUILD_DIR/test_io_no_using.brc" -o "$BUILD_DIR/test_io_no_using.c" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo -e "${GREEN}PASS (expected compiler error)${NC}"
    PASS=$((PASS + 1))
else
    echo -e "${RED}FAIL (should have errored)${NC}"
    FAIL=$((FAIL + 1))
fi

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo -e "${CYAN}========================================${NC}"

exit $FAIL
