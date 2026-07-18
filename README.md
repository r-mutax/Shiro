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
                             | Block [ ";" ]
                             | IfExpression [ ";" ]
                             | WhileExpression [ ";" ]
VariableDeclareStatement   ::= "let" Identifier ";"

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
| 8 | `<`, `<=`, `>`, `>=` | Left | Relational Comparisons | `x < y` |
| 9 | `<<`, `>>` | Left | Bitwise Left Shift, Bitwise Right Shift | `1 << 2` |
| 10 | `+`, `-` | Left | Addition, Subtraction | `x + 5` |
| 11 | `*`, `/`, `%` | Left | Multiplication, Division, Modulo | `10 % 3` |
| 12 | `!`, `~`, `-` | Right | Logical NOT, Bitwise NOT, Unary Minus | `-x` |
| 13 (Highest) | `( )` | None | Grouping (Parentheses) | `(2 + 3) * 4` |

---

## 4. Variables and Control Flow Specifications
*   **Declaration**: `let <variable_name>;`
    *   Variables are automatically initialized to `0` upon declaration (zero initialization).
*   **Assignment**: `<lvalue> = <expression>`
    *   Assignment is treated as an expression, returning the assigned value itself. Since it is right-associative, chained assignment like `y = x = 10` is supported.
    *   Currently, only declared variables are valid as lvalues (left-hand side of assignments).
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
    *   `&&` evaluates LHS. If LHS evaluates to `0` (false), the RHS is skipped, and it returns `0`.
    *   `||` evaluates LHS. If LHS evaluates to non-zero (true), the RHS is skipped, and it returns `1`.
*   **Semicolon Omission Rules**:
    *   `Block`, `IfExpression`, and `WhileExpression` placed at the top-level of statements do not require a trailing semicolon (`;`).
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

### Block Expressions and Local Scopes
```rust
let x;
x = 5;
{
    let y;
    y = 10;
    x + y;     // The block evaluates to 15
}              // Variable y is destroyed here
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

### while Loops
```rust
let x;
let sum;
x = 1;
sum = 0;
while (x <= 5) {
    sum = sum + x;
    x = x + 1;
}              // Computes the sum from 1 to 5
sum;           // Evaluates to 15
```

### Bitwise Operations and Short-circuiting
```rust
let x;
x = 0;
1 || (x = 5);  // LHS is true, so RHS (x = 5) is skipped (short-circuiting)
x;             // Evaluates to 0 (x is not overwritten with 5)
```
