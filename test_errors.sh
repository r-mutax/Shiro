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
assert_error "fn main() -> {}" "Expected type name"
assert_error "fn main() -> i8 { let 123; }" "Expected IDENT after let"
assert_error "fn main() -> i8 { 1 + 2 }" "Expected ';' after expression"
assert_error "fn f(a: i8, b: i8) -> i8 { a; } fn main() -> i8 { f(1 2); }" "Expected ',' before next argument"
assert_error "fn main() -> i8 { if 1 { 2; } }" "Expected '(' after 'if'"
assert_error "fn main() -> i8 { if (1 { 2; } }" "Expected ')' after 'if' condition"
assert_error "fn main() -> i8 { while 1 { 2; } }" "Expected '(' after 'while'"
assert_error "fn main() -> i8 { while (1 { 2; } }" "Expected ')' after 'while' condition"
assert_error "fn main() -> i8 { { let x; }" "Unexpected token: end of file"

# --- Semantic Errors ---
assert_error "fn main() -> MyType { 1; }" "Type 'MyType' is not declared"
assert_error "fn main() -> main { 1; }" "Type 'main' is not declared"
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
assert_error "fn foo(r: &i8) -> i8 { return r; } fn main() -> i8 { let x: i8 = 10; return foo(x); }" "Type mismatch in argument 1 of function 'foo'"
# --- Division by Constant Zero ---
assert_error "fn main() -> i64 { return 10 / 0; }" "Division by zero"
assert_error "fn main() -> i64 { return 10 % 0; }" "Modulo by zero"
assert_error "fn main() -> i64 { return 10 / (5 - 5); }" "Division by zero"

# --- Character Literal Errors ---
assert_error "fn main() -> i8 { let x = 'A; }" "Expected ''' at the end of character literal"
assert_error "fn main() -> i8 { let x = '\\x'; }" "Unknown escape sequence: \\x"

assert_error "fn main() -> i8 { let x: i8 = 10; x[0]; }" "Not an array"
assert_error "struct Point { x: i8 }; fn main() -> i8 { let arr: [5]i8; let p: Point; arr[p]; }" "Index must be an integer"
assert_error "fn main() -> i8 { let arr: [0]i8; }" "Array length must be positive"

# --- Integer Literal Range ---
assert_error "fn main() -> i8 { let x: i8 = 300; return x; }" "Integer literal 300 is out of range for type 'i8'"
assert_error "fn main() -> i8 { let x: i8 = -129; return x; }" "Integer literal -129 is out of range for type 'i8'"
assert_error "fn main() -> u8 { let x: u8 = -1; return x; }" "Integer literal -1 is out of range for type 'u8'"
assert_error "fn main() -> i8 { let x: i8; x = 128; return x; }" "Integer literal 128 is out of range for type 'i8'"
assert_error "fn f(v: u8) -> u8 { return v; } fn main() -> u8 { return f(256); }" "Integer literal 256 is out of range for type 'u8'"
assert_error "fn main() -> i8 { return 1000; }" "Integer literal 1000 is out of range for type 'i8'"

assert_error "struct S { a: i8, fn set(v: i8) -> i8 { return v; } }; fn main() -> i8 { let s: S; return s.set(300); }" "Integer literal 300 is out of range for type 'i8'"
assert_error "fn main() -> i8 { let y: i8 = 1; let x: &i8 = &y; x = 300; return y; }" "Integer literal 300 is out of range for type 'i8'"
assert_error "fn main() -> i8 { let y: i8 = 1; let z: i8 = if (1) 300 else y; return z; }" "Integer literal 300 is out of range for type 'i8'"

echo -e "\e[32mAll error diagnostic tests passed successfully!\e[0m"
