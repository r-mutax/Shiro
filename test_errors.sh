#!/bin/bash

# Function to run a test case for compiler diagnostics
# Usage: assert_error "code" "expected_error_substring"
assert_error() {
    local code="$1"
    local expected="$2"

    # Write code to a temporary file
    echo "$code" > tmp_err.shiro

    # Run the compiler and capture stderr/stdout
    local output
    output=$(./shiro tmp_err.shiro -o tmp_err.s 2>&1)
    local status=$?

    # Strip ANSI color codes
    output=$(echo "$output" | sed 's/\x1b\[[0-9;]*m//g')

    if [ $status -eq 0 ]; then
        echo -e "\e[31m[FAIL] Compilation succeeded unexpectedly for: \"$code\"\e[0m"
        rm -f tmp_err.shiro tmp_err.s
        exit 1
    fi

    if [[ "$output" == *"$expected"* ]]; then
        echo -e "\e[32m[OK]   Expected error caught: \"$expected\"\e[0m"
    else
        echo -e "\e[31m[FAIL] Expected error: \"$expected\", but got:\e[0m"
        echo -e "--- Compiler Output ---"
        echo "$output"
        echo -e "-----------------------"
        rm -f tmp_err.shiro tmp_err.s
        exit 1
    fi

    # Cleanup temporary files
    rm -f tmp_err.shiro tmp_err.s
}

echo "Running Shiro compiler error diagnostic tests..."

# --- Lexical Errors ---
assert_error "fn main() -> i8 { let x = @; }" "Unknown token: @"

# --- Parser Errors ---
assert_error "let x;" "Expected function definition."
assert_error "fn 123() -> i8 {}" "Expected IDENT after fn"
assert_error "fn main) -> i8 {}" "Expected '(' after function name"
assert_error "fn main(a: i8 b: i8) -> i8 {}" "Expected ',' before next parameter"
assert_error "fn main(123: i8) -> i8 {}" "Expected IDENT for function parameter"
assert_error "fn main() -> {}" "Expected type after ->"
assert_error "fn main() -> i8 { let 123; }" "Expected IDENT after let"
assert_error "fn main() -> i8 { 1 + 2 }" "Expected ';' after expression"
assert_error "fn f(a: i8, b: i8) -> i8 { a; } fn main() -> i8 { f(1 2); }" "Expected ',' before next argument"
assert_error "fn main() -> i8 { if 1 { 2; } }" "Expected '(' after 'if'"
assert_error "fn main() -> i8 { if (1 { 2; } }" "Expected ')' after 'if' condition"
assert_error "fn main() -> i8 { while 1 { 2; } }" "Expected '(' after 'while'"
assert_error "fn main() -> i8 { while (1 { 2; } }" "Expected ')' after 'while' condition"
assert_error "fn main() -> i8 { { let x; }" "Unexpected token: end of file"

# --- Semantic Errors ---
assert_error "fn main() -> MyType { 1; }" "Cannot find type 'MyType' in this scope"
assert_error "fn main() -> main { 1; }" "Cannot find type 'main' in this scope"
assert_error "fn f(x: i8, x: i8) -> i8 { 1; } fn main() -> i8 { 1; }" "Duplicated parameter 'x'"
assert_error "fn f() -> i8 { 1; } fn f() -> i8 { 2; } fn main() -> i8 { 1; }" "Function 'f' is already declared"
assert_error "fn main() -> i8 { f(); }" "Function 'f' is not declared"
assert_error "fn main() -> i8 { let f: i8; f = 1; f(); }" "'f' is not a function"
assert_error "fn f(a: i8) -> i8 { a; } fn main() -> i8 { f(1, 2); }" "expects 1 arguments, but got 2 arguments."
assert_error "fn f(a: i8) -> i8 { a; } fn main() -> i8 { let x: i32; x = 1; f(x); }" "Type mismatch in argument 1 of function 'f'. Expected 'i8', but got 'i32'"
assert_error "fn main() -> i8 { let x: MyType; 1; }" "Type 'MyType' is not declared"
assert_error "fn main() -> i8 { let x: main; 1; }" "'main' is not a type"
assert_error "fn main() -> i8 { let x: i8; let x: i8; 1; }" "Variable 'x' is already declared"
assert_error "fn main() -> i8 { x = 1; }" "Variable 'x' is not declared"
assert_error "fn main() -> i8 { let x; x; }" "Cannot infer type of variable 'x' before assignment"
assert_error "fn main() -> i8 { let x: i32; x = 1; return x; }" "Return type 'i8' does not match expression type 'i32'"
assert_error "fn main() -> i8 { let x: i8; let y: i32; x + y; }" "Types of left and right operands do not match. Left: 'i8', Right: 'i32'"
assert_error "fn main() -> i8 { 1 = 2; }" "Left value of assignment is not a variable"
assert_error "fn main() -> i8 { let x: i8; let y: i32; x = y; }" "Type mismatch in assignment. Left: 'i8', Right: 'i32'"
assert_error "fn main() -> i8 { if (1) { let x: i8; x = 1; x; } else { let y: i32; y = 2; y; }; }" "Type mismatch between 'then' and 'else' branches. Then: 'i8', Else: 'i32'"

# --- Character Literal Errors ---
assert_error "fn main() -> i8 { let x = 'A; }" "Expected ''' at the end of character literal"
assert_error "fn main() -> i8 { let x = '\\x'; }" "Unknown escape sequence: \\x"

echo -e "\e[32mAll error diagnostic tests passed successfully!\e[0m"
