# Shiro Language Specification

[日本語版はこちら (README.ja.md)](./README.ja.md)

Shiro is a procedural programming language featuring a primitive integer and reference type system, user-defined structs, type inference, control flow expressions, explicit return statements, and function definitions with parameters.

## 1. Specifications
*   **Data Types**:
    *   Signed integers: `i8`, `i16`, `i32`, `i64`
    *   Unsigned integers: `u8`, `u16`, `u32`, `u64`
    *   Reference types: `&T`, `&&T`, `&&&T` (e.g. `&i8`, `&Point`)
    *   User-defined Structs: `struct StructName { member: Type, ... };` (Methods can be defined inside `struct` body)
*   **Type Inference**:
    *   Variables declared without an explicit type annotation (`let x;`) have an initially unresolved type (`unknown`). The type is automatically inferred and bound from the right-hand side of its first assignment (`x = expr`).
*   **Functions & Entry Point**:
    *   Functions are defined using `fn name(param: Type, ...) -> Type { ... }`.
    *   Functions support up to 6 parameters.
    *   Functions support explicit early returns via `return expr;`.
    *   Program execution begins at the `fn main() -> Type` function definition.
*   **Program Exit Value**: The return value of the `main` function becomes the exit code of the executable.
*   **Comments**:
    *   Single-line comments starting with `//` are ignored until the end of line (`\n`).
*   **Interface Meta-File Generation**:
    *   Running `shiro <file>.shiro -M` (or `--emit-meta`) generates `<file>.shiro.meta` containing interface declarations and a 64-bit FNV-1a source hash.

---

## 2. Grammar (EBNF Representation)

```ebnf
Program            ::= Definition*
Definition         ::= FunctionDefinition | StructDefinition

FunctionDefinition ::= "fn" Identifier "(" [ ParameterList ] ")" "->" Type Block
ParameterList      ::= Parameter ( "," Parameter )*
Parameter          ::= Identifier ":" Type

StructDefinition   ::= "struct" Identifier "{" [ StructMemberList ] "}" ";"
StructMemberList   ::= StructMember ( "," StructMember | ";" StructMember )* [ ";" ]
StructMember       ::= Identifier ":" Type | FunctionDefinition

Statement          ::= ExpressionStatement | VariableDeclareStatement | ReturnStatement
ReturnStatement    ::= "return" Expression ";"

ExpressionStatement        ::= Expression ";"
                             | Block [ ";" ]
                             | IfExpression [ ";" ]
                             | WhileExpression [ ";" ]
VariableDeclareStatement   ::= "let" Identifier [ ":" Type ] ";"

Type                       ::= "&"* BasicType
BasicType                  ::= "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64" | Identifier

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
Unary              ::= ( "!" | "~" | "-" | "&" ) Unary | MemberAccess

MemberAccess       ::= Primary ( "." Identifier )*

Primary            ::= Number 
                             | Character
                             | FunctionCall
                             | Identifier 
                             | "(" Expression ")" 
                             | Block 
                             | IfExpression 
                             | WhileExpression

Character          ::= "'" ( [^'\] | "\" ( "n" | "t" | "r" | "0" | "\" | "'" ) ) "'"
FunctionCall       ::= Identifier "(" [ ArgumentList ] ")"
ArgumentList       ::= Expression ( "," Expression )*
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
| 9 | `<<`, `>>` | Left | Bitwise Left/Right Shift | `x >> 1` |
| 10 | `+`, `-` | Left | Addition, Subtraction | `x + 5` |
| 11 | `*`, `/`, `%` | Left | Multiplication, Division, Modulo | `10 % 3` |
| 12 | `!`, `~`, `-`, `&` | Right | Logical NOT, Bitwise NOT, Unary Minus, Address-of | `&x`, `-x` |
| 13 (Highest) | `.`, `( )` | Left / None | Member Access, Method Call, Grouping | `p.x`, `p.double()`, `(2 + 3)` |

---

## 4. Language Specifications
*   **Function Definitions**: `fn <name>(<param1>: <type1>, <param2>: <type2>, ...) -> <type> { <body> }`
    *   Functions are defined at top-level. Parameter types and return types are strictly validated during semantic analysis.
*   **Function Calls**: `<function_name>(<arg1>, <arg2>, ...)`
    *   Calls an existing function, passing arguments matching the function signature.
*   **Return Statements**: `return <expression>;`
    *   Exits early from the enclosing function at any point, returning the evaluated `<expression>`.
*   **Explicit Type Declaration**: `let <variable_name>: <type>;`
    *   Declares a variable with an explicit type (e.g., `let x: i32;` or `let p: Point;`).
*   **Type Inferred Declaration**: `let <variable_name>;`
    *   Declares a variable without a type annotation. The type is initially `unknown` and is automatically inferred and fixed upon its first assignment (`x = expr`).
*   **Struct Definitions**: `struct <Name> { <field1>: <type1>, <field2>: <type2>, fn <method>(...) -> <type> { ... } };`
    *   Defines a compound data structure with named members and methods. Methods defined inside structs implicitly receive a first parameter `this: &StructName` (a reference to the struct instance) and are mangled as `StructName__methodName`.
*   **Member Access & Method Calls**: `<expr>.<member>` / `<expr>.<method>(<args...>)`
    *   Accesses a member or invokes a method on a struct instance or a reference to a struct. Automatically dereferences reference types if accessed via a reference (`rp.x` or `rp.double()`).
*   **Reference Types & Address Operator**: `&<type>` / `&<expr>`
    *   Takes the memory address of an lvalue (`&x`). Reading a reference automatically dereferences it to fetch the underlying value.
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
*   **Character Literals**: `'<character>'`
    *   Single characters wrapped in single quotes are parsed as unsigned 8-bit integers (`u8`). Supports escape sequences like `\n` (newline), `\t` (tab), `\r` (carriage return), `\0` (null byte), `\\` (backslash), and `\'` (single quote).
*   **Single-line Comments**: `// comment`
    *   Skips all characters after `//` until the end of the line.
*   **Meta-File Export (`-M` / `--emit-meta`)**:
    *   Generates a `.shiro.meta` interface summary file containing source hash and exported/private declarations without compiling to assembly.
*   **Semantic Validation Rules**:
    *   **No Duplicate Declarations**: You cannot declare variables with the same name in the same scope.
    *   **No Undeclared Variable Usage**: You cannot read from or assign to a variable before it is declared.
    *   **No Uninferred Variable Access**: Attempting to read a variable declared as `let x;` before its first assignment raises a compile-time error.
    *   **Type Safety**: Operations or assignments between mismatched types raise compile-time errors.

---

## 5. Code Examples

### Explicit Return Statement and Conditionals
```rust
fn max(a: i32, b: i32) -> i32 {
    if (a > b) {
        return a;    // Returns a immediately
    }
    return b;        // Returns b
}

fn main() -> i8 {
    max(10, 20);     // Evaluates to 20
}
```

### Recursive Function with Return
```rust
fn fact(n: i64) -> i64 {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

fn main() -> i8 {
    fact(5);         // Computes 5! = 120
}
```

### Function Parameters and Calls
```rust
fn add(x: i8, y: i8) -> i8 {
    x + y;
}

fn main() -> i8 {
    add(10, 32);     // Calls add(10, 32), evaluates to 42
}
```

### Explicit Type Declarations
```rust
fn main() -> i32 {
    let x: i32;
    x = 10;
    x + 5;           // Evaluates to 15
}
```

### Type Inference
```rust
fn main() -> i64 {
    let x;           // Type is initially unknown
    x = 42;          // Inferred as i64 on first assignment!
    x;               // Evaluates to 42
}
```

### Reference Types and Auto-Dereferencing
```rust
fn main() -> i8 {
    let x: i8;
    x = 42;
    let rx;          // Type inferred as &i8
    rx = &x;         // Stores address of x
    rx;              // Auto-dereferenced, evaluates to 42
}
```

### Structs and Reference Member Access
```rust
struct Point {
    x: i64,
    y: i64
};

fn main() -> i64 {
    let p: Point;
    p.x = 10;
    p.y = 20;

    let rp;
    rp = &p;         // Reference to struct Point
    rp.x = 50;       // Indirectly modifies p.x through reference

    p.x + p.y;       // Evaluates to 70
}
```

### Struct Methods and Reference Invocation
```rust
struct Point {
    x: i64,
    y: i64,

    fn double() -> i64 {
        return this.x * 2;
    }
};

fn main() -> i64 {
    let p: Point;
    p.x = 10;
    p.y = 20;

    let rp;
    rp = &p;
    rp.double();     // Method invocation via reference, evaluates to 20
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
    z = x + y;       // 200 + 100 = 300 -> Wraps around to 44 in u8!
    z;               // Evaluates to 44
}
```

### if Expressions and Semicolon Omission
```rust
fn main() -> i32 {
    let x;
    x = 10;
    if (x < 20) {
        x * 2;
    } else {
        0;
    }                // Semicolon is omitted. Evaluates to 20
}
```

### Block Expressions and Local Scopes
```rust
fn main() -> i64 {
    let x: i64;
    x = 5;
    {
        let y: i64;
        y = 10;
        x + y;       // The block evaluates to 15
    }                // Variable y is destroyed here
}
```

### while Loops with Type Inference
```rust
fn main() -> i64 {
    let x;           // Inferred as i64
    let sum;         // Inferred as i64
    x = 1;
    sum = 0;
    while (x <= 5) {
        sum = sum + x;
        x = x + 1;
    }                // Computes the sum from 1 to 5
    sum;             // Evaluates to 15
}
```

### Character Literals
```rust
fn main() -> i8 {
    let c: u8;
    c = 'A';         // 'A' is evaluated as u8 integer 65
    let nl: u8;
    nl = '\n';       // Escapes are supported, evaluated as 10
    c + 1;           // Evaluates to 66 (ASCII code for 'B')
}
```

### Comments and Explicit Return
```rust
// Compute maximum of two numbers
fn max(a: i32, b: i32) -> i32 {
    if (a > b) {
        return a;    // Returns a immediately
    }
    return b;
}

fn main() -> i8 {
    max(10, 20);     // Evaluates to 20
}
```

### Interface Meta-File Generation
```bash
# Generate main.shiro.meta
./shiro main.shiro -M
```

Example `main.shiro.meta` output:
```text
// shiro-interface
// source_file: main.shiro
// source_hash: 0832eb8b349bc3e3

// public definition

// private definition
fn max(a: i32, b: i32) -> i32;
fn main() -> i8;
```
