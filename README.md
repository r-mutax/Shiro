# Shiro Language Specification

[日本語版はこちら (README.ja.md)](./README.ja.md)

Shiro is a procedural programming language and compiler featuring a primitive integer type system, type inference, and function definitions.

## 1. Specifications
*   **Data Types**:
    *   Signed integers: `i8`, `i16`, `i32`, `i64`
    *   Unsigned integers: `u8`, `u16`, `u32`, `u64`
*   **Type Inference**:
    *   Variables declared without an explicit type annotation (`let x;`) have an initially unresolved type (`unknown`). The type is automatically inferred and bound from the right-hand side of its first assignment (`x = expr`).
*   **Functions & Entry Point**:
    *   Functions are defined using `fn name() -> Type { ... }`.
    *   Program execution begins at the `fn main() -> Type` function definition.
*   **Program Exit Value**: The return value of the `main` function becomes the exit code of the executable.

---

## 2. Grammar (EBNF Representation)

```ebnf
Program            ::= FunctionDefinition*
FunctionDefinition ::= "fn" Identifier "(" ")" "->" Type Block

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
                             | FunctionCall
                             | Identifier 
                             | "(" Expression ")" 
                             | Block 
                             | IfExpression 
                             | WhileExpression

FunctionCall       ::= Identifier "(" ")"
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

## 4. Functions and Language Specifications
*   **Function Definitions**: `fn <name>() -> <type> { <body> }`
    *   Functions are defined at top-level. Return types are specified using `-> <type>`.
*   **Function Calls**: `<function_name>()`
    *   Calls an existing function. Evaluates to the return value of the called function.
*   **Explicit Type Declaration**: `let <variable_name>: <type>;`
    *   Declares a variable with an explicit primitive integer type (e.g., `let x: i32;`).
*   **Type Inferred Declaration**: `let <variable_name>;`
    *   Declares a variable without a type annotation. The type is initially `unknown` and is automatically inferred and fixed upon its first assignment (`x = expr`).
*   **Assignment**: `<lvalue> = <expression>`
    *   Assignment is treated as an expression, returning the assigned value itself.
*   **Scope**: 
    *   Variables are scoped to the block `{ ... }` (or function scope) where they are declared.
*   **Block Expression**: `{ stmt1; stmt2; ... }`
    *   A block grouping multiple statements acts as an expression, returning the value of the last evaluated statement.
*   **Conditionals (`if` expression)**: `if(condition) expr1 else expr2`
*   **Loops (`while` expression)**: `while(condition) expr`

---

## 5. Code Examples

### Function Definition and Call
```rust
fn bar() -> i8 {
    10;
}

fn main() -> i8 {
    bar();     // Calls bar(), evaluates to 10
}
```

### Multiple Functions and Type Inference
```rust
fn helper() -> u32 {
    100;
}

fn main() -> i64 {
    let x;     // Inferred as u32 from assignment
    x = helper();
    x + 50;    // Evaluates to 150
}
```

### Explicit Type Declarations and Operations
```rust
fn main() -> i32 {
    let x: i32;
    x = 10;
    x + 5;     // Evaluates to 15
}
```

### Unsigned Overflow & Register Wrap-around
```rust
fn main() -> u8 {
    let x: u8;
    x = 200;
    let y: u8;
    y = 100;
    let z: u8;
    z = x + y; // 200 + 100 = 300 -> Wraps around to 44 in u8!
    z;         // Evaluates to 44
}
```

