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
assert "fn main() -> i8 {42;}" 42
assert "fn main() -> i8 {40 + 2;}" 42
assert "fn main() -> i8 {222 + 323;}" 33
assert "fn main() -> i8 {100 - 30;}" 70
assert "fn main() -> i8 {0;}" 0
assert "fn main() -> i8 {5 * 5;}" 25
assert "fn main() -> i8 {10 / 2;}" 5
assert "fn main() -> i8 {10 % 3;}" 1
assert "fn main() -> i8 {(2 + 3) * 4;}" 20
assert "fn main() -> i8 {2 * (3 + 4);}" 14
assert "fn main() -> i8 {(10 - 2) / (5 - 3);}" 4
assert "fn main() -> i8 {1 << 2;}" 4
assert "fn main() -> i8 {4 >> 2;}" 1
assert "fn main() -> i8 {1 + 2; 3 + 4;}" 7
assert "fn main() -> i8 {10 * 10; 20 / 2; 30 - 5;}" 25
assert "fn main() -> i8 {1 == 1;}" 1
assert "fn main() -> i8 {1 != 1;}" 0
assert "fn main() -> i8 {let x; x = 0; x;}" 0
assert "fn main() -> i8 {let x: i64; x + 42;}" 42
assert "fn main() -> i8 {let x: i64; let y: i64; x + y;}" 0
assert "fn main() -> i8 {let x: i64; let y: i64; y + 10; x + 5;}" 5
assert "fn main() -> i8 {let x; x = 10; x;}" 10
assert "fn main() -> i8 {let x; let y; x = 5; y = x + 2; y;}" 7
assert "fn main() -> i8 {let x; let y; y = x = 10; x + y;}" 20
assert "fn main() -> i8 {let x; (x = 5) + 5;}" 10
assert "fn main() -> i8 {if(1) 10 else 20;}" 10
assert "fn main() -> i8 {if(0) 10 else 20;}" 20
assert "fn main() -> i8 {if(1) 10;}" 10
assert "fn main() -> i8 {if(0) 10;}" 0
assert "fn main() -> i8 { 10; 20; }" 20
assert "fn main() -> i8 {let x; x = 5; { let y; y = 10; x + y; } }" 15
assert "fn main() -> i8 {let x; x = 10; if(1) { let y; y = x * 2; y + 5; } else { 0; } }" 25
assert "fn main() -> i8 {let x; x = 5; while(x != 0) { x = x - 1; } x;}" 0
assert "fn main() -> i8 {let x; let sum; x = 5; sum = 0; while(x != 0) { sum = sum + x; x = x - 1; let y; y = 99; } sum;}" 15
assert "fn main() -> i8 {let x; x = 0; while(x != 0) { x = 99; } x;}" 0
# Temps live across a call must survive the call (regression: caller-saved clobber)
assert "fn f(x: i64) -> i64 { return x + 1; } fn main() -> i64 { let n: i64 = 10; return f(n - 1) + f(n - 2); }" 19
assert "fn fib(n: i64) -> i64 { if (n < 2) { return n; } return fib(n - 1) + fib(n - 2); } fn main() -> i64 { return fib(10); }" 55
# Negative literals get the same expected-type propagation as positive ones
assert "fn main() -> i8 { let x: i8 = -56; return x + 100; }" 44
assert "fn main() -> i16 { let x: i16 = -1000; return x + 2000; }" 232
assert "fn main() -> i8 { let x: i8; x = -56; return x + 100; }" 44
assert "fn f(v: i8) -> i8 { return v + 100; } fn main() -> i8 { return f(-56); }" 44
assert "fn main() -> i64 { return 10 - -3; }" 13
# Boundary literals that must be accepted
assert "fn main() -> i8 { let x: i8 = -128; return x; }" 128
assert "fn main() -> i8 { let x: i8 = 127; return x; }" 127
assert "fn main() -> u8 { let x: u8 = 255; return x; }" 255
assert "fn main() -> i16 { let x: i16 = -32768; return x + 32767; }" 255
assert "fn main() -> i8 { return -128; }" 128
assert "fn main() -> i8 {5 < 10;}" 1
assert "fn main() -> i8 {10 < 5;}" 0
assert "fn main() -> i8 {5 < 5;}" 0
assert "fn main() -> i8 {5 <= 10;}" 1
assert "fn main() -> i8 {10 <= 5;}" 0
assert "fn main() -> i8 {5 <= 5;}" 1
assert "fn main() -> i8 {10 > 5;}" 1
assert "fn main() -> i8 {5 > 10;}" 0
assert "fn main() -> i8 {5 > 5;}" 0
assert "fn main() -> i8 {10 >= 5;}" 1
assert "fn main() -> i8 {5 >= 10;}" 0
assert "fn main() -> i8 {5 >= 5;}" 1
assert "fn main() -> i8 {let x; let y; x = 5; y = 10; x < y;}" 1
assert "fn main() -> i8 {let x; let y; x = 5; y = 10; x > y;}" 0
assert "fn main() -> i8 {let x; let sum; x = 1; sum = 0; while(x <= 5) { sum = sum + x; x = x + 1; } sum;}" 15
assert "fn main() -> i8 {let x; let sum; x = 1; sum = 0; while(x < 6) { sum = sum + x; x = x + 1; } sum;}" 15
assert "fn main() -> i8 {5 & 3;}" 1
assert "fn main() -> i8 {5 | 3;}" 7
assert "fn main() -> i8 {5 ^ 3;}" 6
assert "fn main() -> i8 {10 & 7;}" 2
assert "fn main() -> i8 {10 | 7;}" 15
assert "fn main() -> i8 {10 ^ 7;}" 13
assert "fn main() -> i8 {5 | 3 & 2;}" 7
assert "fn main() -> i8 {5 ^ 3 & 2;}" 7
assert "fn main() -> i8 {6 | 4 ^ 3;}" 7
assert "fn main() -> i8 {1 && 0;}" 0
assert "fn main() -> i8 {0 && 1;}" 0
assert "fn main() -> i8 {0 && 0;}" 0
assert "fn main() -> i8 {1 && 1;}" 1
assert "fn main() -> i8 {1 || 0;}" 1
assert "fn main() -> i8 {0 || 1;}" 1
assert "fn main() -> i8 {0 || 0;}" 0
assert "fn main() -> i8 {1 || 1;}" 1
assert "fn main() -> i8 {let x; x = 0; 0 && (x = 5); x;}" 0
assert "fn main() -> i8 {let x; x = 0; 1 || (x = 5); x;}" 0
assert "fn main() -> i8 {-5;}" -5
assert "fn main() -> i8 {!0;}" 1
assert "fn main() -> i8 {!5;}" 0
assert "fn main() -> i8 {~1;}" -2
assert "fn main() -> i8 {let x; x = 5; -x;}" -5
assert "fn main() -> i8 {let x; x = 0; !x;}" 1
assert "fn main() -> i8 {let x: i32; x = 42; x;}" 42
assert "fn main() -> i8 {let x: i8; x = 10; x;}" 10
assert "fn main() -> i8 {let x: u8; x = 20; x;}" 20
assert "fn main() -> i8 {let x: i64; let y: i64; x = 5; y = 10; x + y;}" 15
assert "fn main() -> i8 {let x: i32; x = 10; x + 5;}" 15
assert "fn main() -> i8 {let x: u8; x = 200; x >> 1;}" 100
assert "fn main() -> i8 {let x: u32; x = 100; x / 4;}" 25
assert "fn main() -> i8 {let x: u8; x = 25; x % 10;}" 5
assert "fn main() -> i8 {let x: u16; x = 500; x;}" 244
assert "fn main() -> i8 {let x: i16; x = 300; x;}" 44
assert "fn main() -> i8 {let x: u64; x = 123456; x;}" 64
assert "fn foo() -> i8 { 10; } fn main() -> i8 { 42; }" 42
assert "fn helper() -> u32 { 100; } fn main() -> i64 { 200; }" 200
assert "fn main() -> i8 { 5; } fn bar() -> i8 { 10; }" 5
assert "fn bar() -> i8 { 10; } fn main() -> i8 { bar(); } " 10
assert "fn add(x: i8, y: i8) -> i8 { x + y; } fn main() -> i8 { add(10, 32); }" 42
assert "fn square(x: i32) -> i32 { x * x; } fn main() -> i8 { square(5); }" 25
assert "fn sum6(a: i8, b: i8, c: i8, d: i8, e: i8, f: i8) -> i8 { a + b + c + d + e + f; } fn main() -> i8 { sum6(1, 2, 3, 4, 5, 6); }" 21
assert "fn fact(n: i64) -> i64 { if (n <= 1) 1 else n * fact(n - 1); } fn main() -> i8 { fact(5); }" 120
assert "fn test() -> i8 {10;} fn main() -> i8 { if(1){ return 10; } else { return test(); } 1; }" 10
assert "struct Point { x: i64, y:i64 }; fn main() -> i8 {42;}" 42
assert "struct Point { x: i64, y:i64 }; fn main() -> i8 {let p : Point; p.x = 5; p.y = 10; p.x + p.y;}" 15
assert "fn main() -> i8 { let x : i8 = 10; return x; }" 10
assert "fn main() -> i8 { let y : i8; y = 10; let x : i8 = y; return x; }" 10
assert "fn foo(x : i8) -> i8 { let y : i8 = x; return y; } fn main() -> i8 { return foo(20); }" 20

# --- Reference Types ---
assert "fn main() -> i8 { let x: i8 = 42; let y = &x; y; }" 42
assert "fn main() -> i8 { let x: i8 = 10; let rx = &x; let rrx = &rx; let rrrx = &rrx; rrrx; }" 10
assert "fn main() -> i8 { let x: i8 = 5; let rx = &x; x = 15; rx; }" 15
assert "struct Point { x: i8, y: i8 }; fn main() -> i8 { let p: Point; p.x = 10; p.y = 20; let rp = &p; rp.x + rp.y; }" 30
assert "struct Point { x: i8, y: i8 }; fn main() -> i8 { let p: Point; p.x = 10; p.y = 20; let rp = &p; rp.x = 50; p.x; }" 50

# --- Character Literals ---
assert "fn main() -> i8 { 'A'; }" 65
assert "fn main() -> i8 { '\n'; }" 10
assert "fn main() -> i8 { '\\\\'; }" 92
assert "fn main() -> i8 { 'A' + 1; }" 66
assert "fn main() -> i8 { let c: u8; c = 'Z'; c; }" 90

assert "fn main() -> i8 { let x: i8 = 42; let rx = &x; rx; }" 42
assert "fn main() -> i8 { let x: i8 = 42; let rx: &i8 = &x; rx; }" 42
assert "fn main() -> i8 { let y: i8 = 10; let x = &y; let z = &x; z; }" 10
assert "fn main() -> i8 { let y: i8 = 10; let x = &y; let z = &x; x = 42; return z; }" 42
assert "fn main() -> i8 { let a: i8 = 10; let b: i8 = 20; let ra: &i8 = &a; let rb: &i8 = &b; ra + rb; }" 30

assert "fn main() -> i8 { let x: i8 = 10; return x; }" 10
assert "fn main() -> i8 { let a: i8 = 20; let b: i8 = a + 22; return b; }" 42
assert "fn foo(x: i8) -> i8 { let y: i8 = x + 5; return y; } fn main() -> i8 { return foo(37); }" 42
assert "struct Point { x: i8, y: i8 }; fn main() -> i8 { let p: Point; p.x = 10; p.y = 20; let rx = &p.x; let ry = &p.y; rx = 50; return rx + ry; }" 70

assert "fn inc(r: &i8) -> i8 { r = r + 1; return r; } fn main() -> i8 { let x: i8 = 41; inc(&x); return x; }" 42
assert "struct Point { x: i8, y: i8 }; fn sum(p: &Point) -> i8 { return p.x + p.y; } fn main() -> i8 { let p: Point; p.x = 10; p.y = 32; return sum(&p); }" 42

assert "fn main() -> i8 { let arr: [5]i8; arr[0] = 10; arr[1] = 20; arr[2] = 30; return arr[0] + arr[2]; }" 40
assert "fn main() -> i8 { let arr: [5]i8; arr[0] = 10; let r0 = &arr; r0[0] = 99; return arr[0]; }" 99
assert "struct Point { x: i8, y: i8 }; fn main() -> i8 { let pts: [3]Point; pts[1].x = 10; pts[1].y = 20; return pts[1].x + pts[1].y; }" 30
assert "struct Point { x: i8, y: i8 }; fn main() -> i8 { let pts: [3]Point; pts[0].x = 40; let rp = &pts[0]; rp.y = 2; return pts[0].x + pts[0].y; }" 42

# Regression tests (issue #18): arrays and references must not be confused
assert "fn main() -> i8 { let arr: [4]i8; arr[3] = 7; let r = &arr; r[3] = r[3] + 1; return arr[3]; }" 8
assert "fn main() -> i8 { let arr: [4]i8; arr[0] = 1; arr[3] = 9; let r = &arr; return r[0] + r[3]; }" 10
assert "struct P { x: i8, y: i8 }; fn main() -> i8 { let a: [2]P; a[1].x = 5; a[1].y = 6; let r = &a; r[1].y = 7; return a[1].x + r[1].y; }" 12
assert "struct P { x: i8, y: i8 }; fn main() -> i8 { let a: [2]P; a[0].x = 3; let p = &a[0]; p.y = 4; return a[0].x * a[0].y; }" 12
assert "fn set(a: &[3]i8) -> i8 { a[2] = 21; return 0; } fn main() -> i8 { let arr: [3]i8; set(&arr); return arr[2]; }" 21
# Forward function references (issue #2)
assert "fn main() -> i8 { return foo(); } fn foo() -> i8 { return 42; }" 42
assert "fn is_even(n: i8) -> i8 { if (n == 0) { return 1; } return is_odd(n - 1); } fn is_odd(n: i8) -> i8 { if (n == 0) { return 0; } return is_even(n - 1); } fn main() -> i8 { return is_even(10) + 41; }" 42
assert "fn main() -> i8 { return calc(10, 20, 12); } fn calc(a: i8, b: i8, c: i8) -> i8 { return a + b + c; }" 42

echo -e "\e[32mAll tests passed successfully!\e[0m"

