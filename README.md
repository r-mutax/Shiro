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

Expression         ::= Shift
Shift              ::= AddSub ( ( "<<" | ">>" ) AddSub )*
AddSub             ::= MulDivMod ( ( "+" | "-" ) MulDivMod )*
MulDivMod          ::= Primary ( ( "*" | "/" | "%" ) Primary )*
Primary            ::= Number | Identifier | "(" Expression ")"

Identifier         ::= [a-zA-Z_][a-zA-Z0-9_]*
Number             ::= [0-9]+
```

---

## 3. Operator Precedence and Associativity

Precedence increases from top to bottom. All binary operators are **left-associative**.

| Precedence | Operator | Description | Example |
| :--- | :--- | :--- | :--- |
| 1 (Lowest) | `<<`, `>>` | Bitwise Left Shift, Bitwise Right Shift | `1 << 2` |
| 2 | `+`, `-` | Addition, Subtraction | `x + 5` |
| 3 | `*`, `/`, `%` | Multiplication, Division, Modulo | `10 % 3` |
| 4 (Highest) | `( )` | Grouping (Parentheses) | `(2 + 3) * 4` |

---

## 4. Variable Specifications
*   **Declaration**: `let <variable_name>;`
    *   Variables are automatically initialized to `0` upon declaration (zero initialization).
*   **Scope**: 
    *   Variables are scoped to the block where they are declared (currently only the function scope is supported).
*   **Semantic Validation Rules**:
    *   **No Duplicate Declarations**: You cannot declare variables with the same name in the same scope.
    *   **No Undeclared Variable Usage**: You cannot use a variable before it is declared (no forward reference/post-declaration usage).

---

## 5. Code Examples

### Basic Operations and Variables
```rust
let x;         // Initializes x to 0
let y;         // Initializes y to 0
y + 10;        // Evaluates to 10
x + 5;         // The final evaluated statement (5) becomes the exit code
```

### Operator Precedence
```rust
let a;
(a + 2) * 3;   // Result: 6
```
