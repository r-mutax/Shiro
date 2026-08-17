# Shiro 言語仕様書

[English version (README.md)](./README.md)

Shiro は、整数型および参照型システム、ユーザー定義構造体、型推論、制御構文（式）、明示的な return 文、および引数付き関数定義を備えた手続き型プログラミング言語およびそのコンパイラです。

## 1. 基本仕様
*   **データ型**:
    *   符号付き整数型: `i8`, `i16`, `i32`, `i64`
    *   符号なし整数型: `u8`, `u16`, `u32`, `u64`
    *   参照型: `&T` （例: `&i8`, `&Point`）。参照は初期化式 `let rx = &x;` で生成され、多重参照 `&rx` は同一実体を指す一重参照 `&T` に自動平坦化されます。
    *   ユーザー定義構造体: `struct 構造体名 { メンバ: 型, ... };` （構造体ブロック内に `fn` メソッドを定義可能）
*   **型推論 (Type Inference)**:
    *   型を明示しない変数宣言 (`let x;`) で初期化式がない場合、変数は最初 `unknown` 型（未決定）となり、最初の代入 (`x = expr`) のタイミングで右辺の型が自動適用・固定化されます。
    *   ただし、参照型の生成・設定は初期化式 (`let rx = &x;`) でのみ可能であり、代入文で `unknown` 変数を参照型に推論させることはできません。
*   **関数とエントリーポイント**:
    *   関数は `fn 関数名(仮引数: 型, ...) -> 型 { ... }` の形式で定義します。
    *   最大6個までの引数をサポートしています。
    *   `return 式;` による関数の途中脱出（早期リターン）に対応しています。
    *   プログラムの実行は `fn main() -> 型` から始まります。
*   **プログラムの戻り値**: `main` 関数の戻り値がプログラムの終了コード（Exit Code）になります。
*   **コメント**:
    *   `//` から行末（`\n`）までの記述は1行コメントとして読み飛ばされます。
*   **インターフェース要約ファイル生成**:
    *   `shiro <ファイル名>.shiro -M`（または `--emit-meta`）を実行することで、ソースハッシュと宣言プロトタイプを含む要約ファイル（`<ファイル名>.shiro.meta`）を高速生成できます。

---

## 2. 構文規則（EBNF表現）

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
VariableDeclareStatement   ::= "let" Identifier [ ":" Type ] [ "=" Expression ] ";"

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

## 3. 演算子の優先順位と結合規則

下に行くほど優先順位が高くなります。代入演算子（`=`）は**右結合**、それ以外の二項演算子は**左結合**です。

| 優先順位 | 演算子 | 結合性 | 説明 | 例 |
| :--- | :--- | :--- | :--- | :--- |
| 1 (低) | `=` | 右結合 | 代入 | `y = x = 10` |
| 2 | `||` | 左結合 | 論理和（短絡評価あり） | `x || y` |
| 3 | `&&` | 左結合 | 論理積（短絡評価あり） | `x && y` |
| 4 | `|` | 左結合 | ビット論理和 | `x | y` |
| 5 | `^` | 左結合 | ビット排他的論理和 | `x ^ y` |
| 6 | `&` | 左結合 | ビット論理積 | `x & y` |
| 7 | `==`, `!=` | 左結合 | 等価比較 | `x == 10` |
| 8 | `<`, `<=`, `>`, `>=` | 左結合 | 大小比較 | `x < y` |
| 9 | `<<`, `>>` | 左結合 | ビット左シフト、ビット右シフト | `x >> 1` |
| 10 | `+`, `-` | 左結合 | 加算、減算 | `x + 5` |
| 11 | `*`, `/`, `%` | 左結合 | 乗算、除算、剰余算 | `10 % 3` |
| 12 | `!`, `~`, `-`, `&` | 右結合 | 論理否定、ビット否定、単項マイナス、アドレス取得 | `&x`, `-x` |
| 13 (高) | `.`, `( )` | 左結合 / なし | メンバアクセス、メソッド呼び出し、グループ化（括弧） | `p.x`, `p.double()`, `(2 + 3)` |

---

## 4. 言語仕様詳細
*   **関数定義**: `fn <関数名>(<仮引数1>: <型1>, <仮引数2>: <型2>, ...) -> <戻り値型> { <本体> }`
    *   トップレベルで関数を定義します。仮引数の個数・型、および戻り値の型は静的意味解析で厳格に検証されます。
*   **関数呼び出し**: `<関数名>(<実引数1>, <実引数2>, ...)`
    *   定義された関数を呼び出し、引数を渡します。評価値として関数の戻り値を返します。
*   **`return` 文**: `return <式>;`
    *   関数の任意の場所から即座に脱出し、評価された `<式>` の値を戻り値として返します。
*   **明示的な型指定宣言**: `let <変数名>: <型> [= <初期化式>];`
    *   型を指定して変数を宣言します。（例: `let x: i32 = 10;`, `let p: Point;`）
*   **型推論による宣言**: `let <変数名> [= <初期化式>];`
    *   型アノテーションを省略して宣言できます。初期化式がある場合はその型から自動推論されます。初期化式がない場合、最初は `unknown` 型（未決定）となり、最初の代入 `x = expr` のタイミングで右辺の型が変数に自動適用・固定化されます。
*   **構造体定義**: `struct <構造体名> { <メンバ1>: <型1>, <メンバ2>: <型2>, fn <メソッド名>(...) -> <型> { ... } };`
    *   複数のフィールドおよびメソッドをまとめた複合データ型を定義します。構造体内で定義されたメソッドは暗黙的に第1引数 `this: &構造体名`（構造体への参照）を受け取る関数（ネームマングル名: `構造体名__メソッド名`）として定義・処理されます。
*   **メンバアクセス・メソッド呼び出し**: `<式>.<メンバ>` / `<式>.<メソッド>(<引数...>)`
    *   構造体インスタンスまたは構造体参照のメンバやメソッドにアクセス・呼び出しを行います。参照経由アクセスの場合は自動的にデリファレンスされます（`rp.x` や `rp.double()`）。
*   **参照型とアドレス取得演算子**: `&<型>` / `&<変数またはメンバ>`
    *   変数または構造体メンバのメモリ参照（アドレス）を取得します。参照型の生成およびバインドは宣言時の初期化式（`let rx = &x;`, `let rx = &p.x;` または `let rx: &i8 = &x;`）でのみ可能です（未初期化の `unknown` 変数に対する代入文 `rx = &x` による後からの参照設定・型推論はできません）。
    *   参照型変数 `rx` に対する `&rx` は多重参照（`&&T`）を作らず、同じ実体への一重参照 `&T` に自動平坦化されます（`let z = &rx;`）。
    *   参照型変数への値の代入 `rx = val;` は、参照先の変数の値を書き換えます。参照型の変数を評価した際は自動的にデリファレンスされて元の値が取得されます。
*   **代入**: `<左辺値> = <式>`
    *   代入は式として扱われ、代入された値自身を評価値として返します。右結合であるため、`y = x = 10` のような連続した代入が可能です。
    *   すでに型が決定している変数へ異なる型を代入しようとした場合、型不一致（Type Mismatch）エラーが発生します。
*   **スコープ**: 
    *   変数は宣言されたブロック `{ ... }`（または関数全体）のスコープの中で有効です。
    *   内側のブロックで同名の変数を宣言すると、外側の変数が隠蔽（シャドウイング）されます。
*   **ブロック式**: `{ stmt1; stmt2; ... }`
    *   複数の文を束ねるブロックは「式（Expression）」として機能し、ブロック内で最後に評価された文の値を評価値として返します。空ブロック `{}` の評価値は `0` です。
*   **条件分岐（`if`式）**: `if(condition) expr1 else expr2`
    *   条件式が `0` 以外（真）なら `式1`、`0`（偽）なら `式2` を評価し、その値を返します。`else` を省略した場合、偽の時は `0` を返します。
*   **繰り返し（`while`式）**: `while(condition) expr`
    *   条件式が `0` 以外（真）の間、`式` を繰り返し実行します。`while` 式自体は、最後に実行されたループ本体の評価値を返します。（一度もループが実行されなかった場合は `0` を返します）。
*   **短絡評価（論理演算）**:
    *   `&&`（論理積）および `||`（論理和）は短絡評価を行います。
    *   `&&` は左辺が偽 (`0`) の場合、右辺を評価せずに `0` を返します。
    *   `||` は左辺が真 (非`0`) の場合、右辺を評価せずに `1` を返します。
*   **文字リテラル**: `'<文字>'`
    *   シングルクォートで囲まれた1文字は、符号なし8ビット整数型（`u8`）として評価されます。`\n`（改行）、`\t`（タブ）、`\r`（キャリッジリターン）、`\0`（ヌル文字）、`\\`（バックスラッシュ）、`\'`（シングルクォート）などのエスケープシーケンスに対応しています。
*   **1行コメント**: `// コメント`
    *   `//` から行末までのテキストを読み飛ばします。
*   **要約ファイル出力 (`-M` / `--emit-meta`)**:
    *   ソースハッシュおよび公開/非公開宣言プロトタイプを含む `.shiro.meta` 要約ファイルを高速出力します。
*   **意味解析（検証規則）**:
    *   **二重宣言の禁止**: 同一スコープ内で同名の変数を複数回宣言することはできません。
    *   **未定義変数の使用禁止**: 宣言されていない変数を使用したり代入したりすることはできません。
    *   **型確定前の変数参照禁止**: `let x;` で宣言後、一度も代入を行わずに変数 `x` の値を参照しようとするとコンパイルエラーになります。
    *   **型不一致の禁止**: 互換性のない型同士の演算・代入はコンパイルエラーになります。

---

## 5. コード例

### 明示的な return 文と条件分岐
```rust
fn max(a: i32, b: i32) -> i32 {
    if (a > b) {
        return a;    // a の値を即座に返して脱出
    }
    return b;        // b の値を返す
}

fn main() -> i8 {
    max(10, 20);     // 評価値: 20
}
```

### return 文を用いた再帰関数
```rust
fn fact(n: i64) -> i64 {
    if (n <= 1) {
        return 1;
    }
    return n * fact(n - 1);
}

fn main() -> i8 {
    fact(5);         // 5! = 120 を計算
}
```

### 引数付き関数の定義と呼び出し
```rust
fn add(x: i8, y: i8) -> i8 {
    x + y;
}

fn main() -> i8 {
    add(10, 32);   // add(10, 32) を呼び出し、評価値 42 を返す
}
```

### 明示的な型指定と演算
```rust
fn main() -> i32 {
    let x: i32;    // i32 型の変数を宣言
    x = 10;
    x + 5;         // 評価値: 15
}
```

### 型推論 (Type Inference)
```rust
fn main() -> i64 {
    let x;         // 型未決定 (unknown)
    x = 42;        // 最初の代入により x の型が i64 に確定
    x;             // 評価値: 42
}
```

### 参照型と自動デリファレンス
```rust
fn main() -> i8 {
    let x: i8 = 42;
    let rx = &x;     // 型推論により rx は &i8 型に決定
    let rrx = &rx;   // 多重参照は平坦化され、rrx も x への &i8 参照となる
    rx = 100;        // 参照経由で x の値を 100 に書き換え
    rrx;             // 自動デリファレンスにより 100 を評価
}
```

### 構造体メンバへの参照
```rust
struct Point { x: i8, y: i8 };
fn main() -> i8 {
    let p: Point;
    p.x = 10;
    p.y = 20;
    let rx = &p.x;   // 構造体メンバ p.x の参照を取得
    rx = 50;         // 参照経由で p.x の値を更新
    p.x;             // 50
}
```

### 構造体と参照経由のメンバ操作
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
    rp = &p;         // 構造体 Point への参照
    rp.x = 50;       // 参照経由で p.x を変更

    p.x + p.y;       // 70 に評価される
}
```

### 構造体メソッドの定義と呼び出し
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
    rp.double();     // 参照経由でのメソッド呼び出し、20 に評価される
}
```

### 符号なし整数とオーバーフロー（ラップアラウンド）
```rust
fn main() -> u8 {
    let x: u8;
    x = 200;
    let y: u8;
    y = 100;
    let z: u8;
    z = x + y;     // 200 + 100 = 300 ➔ u8(8bit)でラップアラウンドして 44
    z;             // 評価値: 44
}
```

### if 式とセミコロン省略
```rust
fn main() -> i32 {
    let x;
    x = 10;
    if (x < 20) {
        x * 2;
    } else {
        0;
    }              // セミコロンなしでトップレベルに記述可能。評価値は 20
}
```

### ブロック式とローカルスコープ
```rust
fn main() -> i64 {
    let x: i64;
    x = 5;
    {
        let y: i64;
        y = 10;
        x + y;     // このブロック全体の評価値は 15 となる
    }              // 変数 y の寿命はここで尽きる
}
```

### while ループによる繰り返し処理
```rust
fn main() -> i64 {
    let x;         // 型推論
    let sum;       // 型推論
    x = 1;
    sum = 0;
    while (x <= 5) {
        sum = sum + x;
        x = x + 1;
    }              // 1 から 5 までの合計を計算する
    sum;           // 評価値: 15
}
```

### 文字リテラル (Character Literals)
```rust
fn main() -> i8 {
    let c: u8;
    c = 'A';       // 'A' は u8 型の整数値 65 として評価されます
    let nl: u8;
    nl = '\n';     // エスケープ文字に対応、10 として評価されます
    c + 1;         // 評価値: 66 (文字 'B' のアスキーコード)
}
```

### コメントと早期リターン
```rust
// 2つの数値の最大値を計算
fn max(a: i32, b: i32) -> i32 {
    if (a > b) {
        return a;    // 早期リターン
    }
    return b;
}

fn main() -> i8 {
    max(10, 20);     // 20 に評価される
}
```

### 要約ファイルの生成
```bash
# main.shiro.meta を出力
./shiro main.shiro -M
```

出力される `main.shiro.meta` の例:
```text
// shiro-interface
// source_file: main.shiro
// source_hash: 0832eb8b349bc3e3

// public definition

// private definition
fn max(a: i32, b: i32) -> i32;
fn main() -> i8;
```
