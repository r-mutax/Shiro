# Shiro Language Specification

[日本語版はこちら (README.ja.md)](./README.ja.md)

Shiro is a very simple procedural programming language and its compiler that deals exclusively with 64-bit signed integers.

## 1. Specifications
*   **Data Type**: Implicitly **64-bit signed integer** (`qword`).
*   **Entry Point**: Statements in the source code are implicitly compiled into the `main` function in the assembly.
*   **Program Exit Value**: The value of the last evaluated statement in the program becomes the exit code of the executable.

---

## 2. Grammar (EBNF Representation)

```ebnf
Program            ::= Statement*
Statement          ::= ExpressionStatement | VariableDeclareStatement

ExpressionStatement        ::= Expression ";"
VariableDeclareStatement   ::= "let" Identifier ";"

Expression         ::= Assign
Assign             ::= Shift [ "=" Assign ]
Shift              ::= AddSub ( ( "<<" | ">>" ) AddSub )*
AddSub             ::= MulDivMod ( ( "+" | "-" ) MulDivMod )*
MulDivMod          ::= Primary ( ( "*" | "/" | "%" ) Primary )*
Primary            ::= Number | Identifier | "(" Expression ")"

Identifier         ::= [a-zA-Z_][a-zA-Z0-9_]*
Number             ::= [0-9]+
```

---

## 3. Operator Precedence and Associativity

Precedence increases from top to bottom. The assignment operator (`=`) is **right-associative**, while all other binary operators are **left-associative**.

| Precedence | Operator | Associativity | Description | Example |
| :--- | :--- | :--- | :--- | :--- |
| 1 (Lowest) | `=` | Right | Assignment | `y = x = 10` |
| 2 | `<<`, `>>` | Left | Bitwise Left Shift, Bitwise Right Shift | `1 << 2` |
| 3 | `+`, `-` | Left | Addition, Subtraction | `x + 5` |
| 4 | `*`, `/`, `%` | Left | Multiplication, Division, Modulo | `10 % 3` |
| 5 (Highest) | `( )` | None | Grouping (Parentheses) | `(2 + 3) * 4` |

---

## 4. Variable Specifications
*   **Declaration**: `let <variable_name>;`
    *   Variables are automatically initialized to `0` upon declaration (zero initialization).
*   **Assignment**: `<lvalue> = <expression>`
    *   Assignment is treated as an expression, returning the assigned value itself. Since it is right-associative, chained assignment like `y = x = 10` is supported.
    *   Currently, only declared variables are valid as lvalues (left-hand side of assignments).
*   **Scope**: 
    *   Variables are scoped to the block where they are declared (currently only the function scope is supported).
*   **Semantic Validation Rules**:
    *   **No Duplicate Declarations**: You cannot declare variables with the same name in the same scope.
    *   **No Undeclared Variable Usage**: You cannot read from or assign to a variable before it is declared.
    *   **Lvalue Validation**: The left-hand side of an assignment must be an assignable expression (currently only variables).

---

## 5. Code Examples

### Basic Operations and Variables
```rust
let x;         // Initializes x to 0
let y;         // Initializes y to 0
x = 5;         // Assigns 5 to x
y = x + 2;     // Assigns x + 2 (7) to y
y;             // The final evaluated statement (7) becomes the exit code
```

### Chained Assignment (Right Associativity)
```rust
let x;
let y;
y = x = 10;    // Assigns 10 to both x and y
x + y;         // Final evaluated statement: 20
```

### Evaluation Value of Assignment Expressions
```rust
let x;
(x = 5) + 5;   // The assignment (x = 5) itself evaluates to 5, which is then added to 5. Final value: 10
```
