#!/bin/bash

# Function to run a test case
# Usage: assert "code" expected_exit_code
assert() {
    local code="$1"
    local expected="$2"
    expected=$(( expected & 255 ))

    # Write code to a temporary file
    echo "$code" > tmp.shiro

    # Compile Shiro source to Assembly (capture stderr/stdout)
    local compile_output
    compile_output=$(./shiro tmp.shiro -o tmp.s 2>&1)
    local compile_status=$?

    if [ $compile_status -ne 0 ]; then
        echo -e "\e[31m[FAIL] Compilation failed for: \"$code\"\e[0m"
        echo -e "--- Compiler Output ---"
        echo "$compile_output"
        echo -e "-----------------------"
        rm -f tmp.shiro tmp.s
        exit 1
    fi

    # Assemble and Link to executable (capture stderr/stdout)
    local asm_output
    asm_output=$(gcc -no-pie tmp.s -o tmp_bin 2>&1)
    local asm_status=$?

    if [ $asm_status -ne 0 ]; then
        echo -e "\e[31m[FAIL] Assembly failed for: \"$code\"\e[0m"
        echo -e "--- GCC/Assembler Output ---"
        echo "$asm_output"
        echo -e "----------------------------"
        # Print the generated assembly to help debug
        echo -e "--- Generated Assembly (tmp.s) ---"
        cat tmp.s
        echo -e "----------------------------------"
        rm -f tmp.shiro tmp.s tmp_bin
        exit 1
    fi

    # Run the compiled binary
    ./tmp_bin
    local actual=$?

    # Verify exit code
    if [ "$actual" -eq "$expected" ]; then
        echo -e "\e[32m[OK]   \"$code\" => $actual\e[0m"
    else
        echo -e "\e[31m[FAIL] \"$code\" => expected $expected, but got $actual\e[0m"
        rm -f tmp.shiro tmp.s tmp_bin
        exit 1
    fi

    # Cleanup temporary files
    rm -f tmp.shiro tmp.s tmp_bin
}

# Run tests
echo "Running Shiro compiler tests..."
assert "42;" 42
assert "40 + 2;" 42
assert "222 + 323;" 33
assert "100 - 30;" 70
assert "0;" 0
assert "5 * 5;" 25
assert "10 / 2;" 5
assert "10 % 3;" 1
assert "(2 + 3) * 4;" 20
assert "2 * (3 + 4);" 14
assert "(10 - 2) / (5 - 3);" 4
assert "1 << 2;" 4
assert "4 >> 2;" 1
assert "1 + 2; 3 + 4;" 7
assert "10 * 10; 20 / 2; 30 - 5;" 25
assert "1 == 1;" 1
assert "1 != 1;" 0
assert "let x; x;" 0
assert "let x; x + 42;" 42
assert "let x; let y; x + y;" 0
assert "let x; let y; y + 10; x + 5;" 5
assert "let x; x = 10; x;" 10
assert "let x; let y; x = 5; y = x + 2; y;" 7
assert "let x; let y; y = x = 10; x + y;" 20
assert "let x; (x = 5) + 5;" 10
assert "if(1) 10 else 20;" 10
assert "if(0) 10 else 20;" 20
assert "if(1) 10;" 10
assert "if(0) 10;" 0
assert "{ 10; 20; }" 20
assert "{ let x; x = 5; { let y; y = 10; x + y; } }" 15
assert "let x; x = 10; if(1) { let y; y = x * 2; y + 5; } else { 0; }" 25
assert "let x; x = 5; while(x != 0) { x = x - 1; } x;" 0
assert "let x; let sum; x = 5; sum = 0; while(x != 0) { sum = sum + x; x = x - 1; let y; y = 99; } sum;" 15
assert "let x; x = 0; while(x != 0) { x = 99; } x;" 0
assert "5 < 10;" 1
assert "10 < 5;" 0
assert "5 < 5;" 0
assert "5 <= 10;" 1
assert "10 <= 5;" 0
assert "5 <= 5;" 1
assert "10 > 5;" 1
assert "5 > 10;" 0
assert "5 > 5;" 0
assert "10 >= 5;" 1
assert "5 >= 10;" 0
assert "5 >= 5;" 1
assert "let x; let y; x = 5; y = 10; x < y;" 1
assert "let x; let y; x = 5; y = 10; x > y;" 0
assert "let x; let sum; x = 1; sum = 0; while(x <= 5) { sum = sum + x; x = x + 1; } sum;" 15
assert "let x; let sum; x = 1; sum = 0; while(x < 6) { sum = sum + x; x = x + 1; } sum;" 15
assert "5 & 3;" 1
assert "5 | 3;" 7
assert "5 ^ 3;" 6
assert "10 & 7;" 2
assert "10 | 7;" 15
assert "10 ^ 7;" 13
assert "5 | 3 & 2;" 7
assert "5 ^ 3 & 2;" 7
assert "6 | 4 ^ 3;" 7
assert "1 && 0;" 0
assert "0 && 1;" 0
assert "0 && 0;" 0
assert "1 && 1;" 1
assert "1 || 0;" 1
assert "0 || 1;" 1
assert "0 || 0;" 0
assert "1 || 1;" 1
assert "let x; x = 0; 0 && (x = 5); x;" 0
assert "let x; x = 0; 1 || (x = 5); x;" 0
assert "-5;" -5
assert "!0;" 1
assert "!5;" 0
assert "~1;" -2
assert "let x; x = 5; -x;" -5
assert "let x; x = 0; !x;" 1
assert "let x: i32; x = 42; x;" 42
assert "let x: i8; x = 10; x;" 10
assert "let x: u8; x = 20; x;" 20
assert "let x: i64; let y: i64; x = 5; y = 10; x + y;" 15
assert "let x: i32; x = 10; x + 5;" 15
assert "let x: u8; x = 200; x >> 1;" 100
assert "let x: u32; x = 100; x / 4;" 25
assert "let x: u8; x = 25; x % 10;" 5

echo -e "\e[32mAll tests passed successfully!\e[0m"

