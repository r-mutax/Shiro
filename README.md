# Shiro Language Specification

[日本語版はこちら (README.ja.md)](./README.ja.md)

Shiro is a procedural programming language and compiler featuring a primitive integer type system and type inference.

## 1. Specifications
*   **Data Types**:
    *   Signed integers: `i8`, `i16`, `i32`, `i64`
    *   Unsigned integers: `u8`, `u16`, `u32`, `u64`
*   **Type Inference**:
    *   Variables declared without an explicit type annotation (`let x;`) have an initially unresolved type (`unknown`). The type is automatically inferred and bound from the right-hand side of its first assignment (`x = expr`).
*   **Entry Point**: Statements in the source code are implicitly compiled into the `main` function in the assembly.
*   **Program Exit Value**: The value of the last evaluated statement in the program becomes the exit code of the executable.

---

## 2. Grammar (EBNF Representation)

```ebnf
Program            ::= Statement*
Statement          ::= ExpressionStatement | VariableDeclareStatement

ExpressionStatement        ::= Expression ";"
                             | Block [ ";" ]
                             | IfExpression [ ";" ]
                             | WhileExpression [ ";" ]
VariableDeclareStatement   ::= "let" Identifier [ ":" Type ] ";"
Type                       ::= "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64"

Expression         ::= Assign
Assign             ::= LogicalOr [ "=" Assign ]
LogicalOr          ::= LogicalAnd ( "||" LogicalAnd )*
LogicalAnd         ::= BitOr ( "&&" BitOr )*
BitOr              ::= BitXor ( "|" BitXor )*
BitXor             ::= BitAnd ( "^" BitAnd )*
BitAnd             ::= Equality ( "&" Equality )*
Equality           ::= Relational ( ( "==" | "!=" ) Relational )*
Relational         ::= Shift ( ( "<" | "<=" | ">" | ">=" ) Shift )*
Shift              ::= AddSub ( ( "<<" | ">>" ) AddSub )*
AddSub             ::= MulDivMod ( ( "+" | "-" ) MulDivMod )*
MulDivMod          ::= Unary ( ( "*" | "/" | "%" ) Unary )*
Unary              ::= ( "!" | "~" | "-" ) Unary | Primary
Primary            ::= Number 
                             | Identifier 
                             | "(" Expression ")" 
                             | Block 
                             | IfExpression 
                             | WhileExpression

Block              ::= "{" Statement* "}"
IfExpression       ::= "if" "(" Expression ")" Expression [ "else" Expression ]
WhileExpression    ::= "while" "(" Expression ")" Expression

Identifier         ::= [a-zA-Z_][a-zA-Z0-9_]*
Number             ::= [0-9]+
```

---

## 3. Operator Precedence and Associativity

Precedence increases from top to bottom. The assignment operator (`=`) is **right-associative**, while all other binary operators are **left-associative**.

| Precedence | Operator | Associativity | Description | Example |
| :--- | :--- | :--- | :--- | :--- |
| 1 (Lowest) | `=` | Right | Assignment | `y = x = 10` |
| 2 | `\|\|` | Left | Logical OR (Short-circuiting) | `x \|\| y` |
| 3 | `&&` | Left | Logical AND (Short-circuiting) | `x && y` |
| 4 | `\|` | Left | Bitwise OR | `x \| y` |
| 5 | `^` | Left | Bitwise XOR | `x ^ y` |
| 6 | `&` | Left | Bitwise AND | `x & y` |
| 7 | `==`, `!=` | Left | Equality Comparisons | `x == 10` |
| 8 | `<`, `<=`, `>`, `>=` | Left | Relational Comparisons (Signed vs Unsigned aware) | `x < y` |
| 9 | `<<`, `>>` | Left | Bitwise Left/Right Shift (`>>` emits `sar` for signed, `shr` for unsigned) | `x >> 1` |
| 10 | `+`, `-` | Left | Addition, Subtraction | `x + 5` |
| 11 | `*`, `/`, `%` | Left | Multiplication, Division, Modulo (`/`, `%` emit `idiv` for signed, `div` for unsigned) | `10 % 3` |
| 12 | `!`, `~`, `-` | Right | Logical NOT, Bitwise NOT, Unary Minus | `-x` |
| 13 (Highest) | `( )` | None | Grouping (Parentheses) | `(2 + 3) * 4` |

---

## 4. Variables and Control Flow Specifications
*   **Explicit Type Declaration**: `let <variable_name>: <type>;`
    *   Declares a variable with an explicit primitive integer type (e.g., `let x: i32;`).
*   **Type Inferred Declaration**: `let <variable_name>;`
    *   Declares a variable without a type annotation. The type is initially `unknown` and is automatically inferred and fixed upon its first assignment (`x = expr`).
*   **Assignment**: `<lvalue> = <expression>`
    *   Assignment is treated as an expression, returning the assigned value itself. Since it is right-associative, chained assignment like `y = x = 10` is supported.
    *   Assigning a value of a mismatched type to an already typed variable results in a compile-time type mismatch error.
*   **Scope**: 
    *   Variables are scoped to the block `{ ... }` (or function scope) where they are declared.
    *   Declaring a variable inside an inner block with the same name shadows the outer variable.
*   **Block Expression**: `{ stmt1; stmt2; ... }`
    *   A block grouping multiple statements acts as an expression, returning the value of the last evaluated statement. An empty block `{}` evaluates to `0`.
*   **Conditionals (`if` expression)**: `if(condition) expr1 else expr2`
    *   If the condition is non-zero (true), it evaluates to `expr1`. Otherwise (false), it evaluates to `expr2`. If `else` is omitted, a false condition evaluates to `0`.
*   **Loops (`while` expression)**: `while(condition) expr`
    *   Repeatedly executes `expr` as long as `condition` evaluates to non-zero (true). The `while` expression evaluates to the value of the last loop body iteration (or `0` if the loop never ran).
*   **Short-circuit Evaluation (Logical Operations)**:
    *   `&&` (Logical AND) and `||` (Logical OR) perform short-circuit evaluation.
*   **Semantic Validation Rules**:
    *   **No Duplicate Declarations**: You cannot declare variables with the same name in the same scope.
    *   **No Undeclared Variable Usage**: You cannot read from or assign to a variable before it is declared.
    *   **No Uninferred Variable Access**: Attempting to read a variable declared as `let x;` before its first assignment raises a compile-time error.
    *   **Type Safety**: Operations or assignments between mismatched types raise compile-time errors.

---

## 5. Code Examples

### Explicit Type Declarations
```rust
let x: i32;
x = 10;
x + 5;         // Evaluates to 15
```

### Type Inference
```rust
let x;         // Type is initially unknown
x = 42;        // Inferred as i64 on first assignment!
x;             // Evaluates to 42
```

### Unsigned Overflow & Register Wrap-around
```rust
let x: u8;
x = 200;
let y: u8;
y = 100;
let z: u8;
z = x + y;     // 200 + 100 = 300 -> Wraps around to 44 in u8!
z;             // Evaluates to 44
```

### if Expressions and Semicolon Omission
```rust
let x;
x = 10;
if (x < 20) {
    x * 2;
} else {
    0;
}              // Semicolon is omitted. Evaluates to 20
```

### Block Expressions and Local Scopes
```rust
let x: i64;
x = 5;
{
    let y: i64;
    y = 10;
    x + y;     // The block evaluates to 15
}              // Variable y is destroyed here
```

### while Loops with Type Inference
```rust
let x;         // Inferred as i64
let sum;       // Inferred as i64
x = 1;
sum = 0;
while (x <= 5) {
    sum = sum + x;
    x = x + 1;
}              // Computes the sum from 1 to 5
sum;           // Evaluates to 15
```
