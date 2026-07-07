#!/bin/bash

# Function to run a test case
# Usage: assert "code" expected_exit_code
assert() {
    local code="$1"
    local expected="$2"

    # Write code to a temporary file
    echo "$code" > tmp.shiro

    # Compile Shiro source to Assembly
    ./shiro tmp.shiro > tmp.s 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "\e[31m[FAIL] Compilation failed for: $code\e[0m"
        rm -f tmp.shiro tmp.s
        exit 1
    fi

    # Assemble and Link to executable
    gcc -no-pie tmp.s -o tmp_bin 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "\e[31m[FAIL] Assembly failed for: $code\e[0m"
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
assert "42" 42
assert "40 + 2" 42
assert "222 + 323" 33
assert "100 - 30" 70
assert "0" 0

echo -e "\e[32mAll tests passed successfully!\e[0m"
