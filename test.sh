#!/bin/bash

# Function to run a test case
# Usage: assert "code" expected_exit_code
assert() {
    local code="$1"
    local expected="$2"

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



echo -e "\e[32mAll tests passed successfully!\e[0m"
