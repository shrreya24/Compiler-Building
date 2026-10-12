# Mini C Compiler (C++17)

A lightweight, modular compiler for a small C-like language written in modern **C++17** using standard library components only. The compiler translates high-level source code through lexical analysis, recursive-descent parsing, Abstract Syntax Tree (AST) construction, semantic type & symbol resolution, compile-time constant folding, and Three-Address Code (TAC) intermediate representation (IR) generation.

> **Scope Note**: This project is an educational compiler demonstrating frontend and middle-end compiler architecture for a **small C-like language**. It does not emit machine code, does not execute compiled programs directly, and does not claim LLVM backend integration.

---

## Architecture & Compiler Pipeline

The compilation pipeline is cleanly decoupled into five sequential phases:

```
Source Code (.mc)
       │
       ▼
 ┌───────────┐
 │   Lexer   │  Tokenization & character stream scanning (Lexical Analysis)
 └───────────┘
       │
       ▼
 ┌───────────┐
 │  Parser   │  LL(1) Recursive-Descent Parsing & Syntax Verification
 └───────────┘
       │
       ▼
 ┌───────────┐
 │    AST    │  Polymorphic Abstract Syntax Tree representation
 └───────────┘
       │
       ▼
 ┌───────────┐
 │ Semantics │  Symbol Table, scope verification, undeclared/redefinition checks
 └───────────┘
       │
       ▼
 ┌───────────┐
 │ Optimizer │  AST Constant Folding (e.g. 2 + 3 * 4 -> 14; safe div-by-zero)
 └───────────┘
       │
       ▼
 ┌───────────┐
 │    IR     │  Three-Address Code (TAC) generation with virtual registers (t0, t1...)
 └───────────┘
```

### 1. Lexical Analysis (`lexer.h`, `lexer.cpp`)
- Converts raw source text into a strongly typed token stream (`Token`).
- Tracks line and column positions for every token for accurate compiler diagnostics.
- Identifies keywords (`int`, `print`), identifiers, integer literals, arithmetic operators (`+`, `-`, `*`, `/`, `=`), parentheses, and semicolons.
- Safely strips whitespace and single-line comments (`// ...`).
- Catches unrecognized characters cleanly without crashing.

### 2. Syntax Analysis & AST (`parser.h`, `parser.cpp`, `ast.h`)
- Implements an **LL(1) Recursive-Descent Parser** matching the grammar:
  ```
  Program      ::= Statement* EOF
  Statement    ::= VarDecl | Assignment | PrintStmt
  VarDecl      ::= "int" IDENTIFIER ( "=" Expression )? ";"
  Assignment   ::= IDENTIFIER "=" Expression ";"
  PrintStmt    ::= "print" "(" Expression ")" ";"
  Expression   ::= Term ( ("+" | "-") Term )*
  Term         ::= Factor ( ("*" | "/") Factor )*
  Factor       ::= ("+" | "-")? Primary
  Primary      ::= INT_LITERAL | IDENTIFIER | "(" Expression ")"
  ```
- Enforces standard mathematical operator precedence (`*`, `/` higher than `+`, `-`) and explicit grouping with parentheses.
- Constructs a type-safe polymorphic **Abstract Syntax Tree (AST)** using modern C++ smart pointers (`std::unique_ptr`).

### 3. Semantic Analysis & Symbol Table (`semantic.h`, `semantic.cpp`)
- Maintains a `SymbolTable` mapping variable identifiers to declarations, types, and line numbers.
- Detects **undeclared variable references** in expressions or assignments.
- Detects **duplicate variable declarations** (redefinitions) in the same scope.
- Reports descriptive error messages with precise line and column numbers.

### 4. Constant Folding Optimization (`ast.h`)
- Traverses AST expression nodes prior to IR code emission.
- Evaluates constant subtrees at compile-time (e.g., `2 + 3 * 4` folds into `14`).
- **Safety Guarantee**: Avoids unsafe transformations, such as compile-time division by zero (e.g., `10 / 0` is preserved for runtime handling).

### 5. Intermediate Representation (`ir.h`, `ir.cpp`)
- Lowers the AST into **Three-Address Code (TAC)**.
- Allocates unbounded virtual temporary registers (`t0`, `t1`, `t2`, ...).
- Decomposes nested expressions into atomic instructions:
  - `dst = src1` (Assignment)
  - `dst = src1 op src2` (Binary operation)
  - `PRINT src1` (Output)

---

## Project Structure

```text
mini-c-compiler/
├── CMakeLists.txt              # Standard CMake build configuration (CTest enabled)
├── .gitignore                  # Git exclusions for builds and binaries
├── README.md                   # Project overview and technical documentation
├── main.cpp                    # Compiler CLI driver and pipeline coordinator
├── lexer.h / lexer.cpp         # Lexical analyzer & Token definitions
├── ast.h                       # AST node hierarchy & Constant folding optimizer
├── parser.h / parser.cpp       # Recursive-descent parser
├── semantic.h / semantic.cpp   # Symbol table & Semantic validation
├── ir.h / ir.cpp               # Three-Address Code generation
├── examples/
│   ├── basic.mc                # Sample valid Mini C source file
│   └── invalid.mc              # Sample source with intentional errors
└── tests/
    ├── test_runner.cpp         # Automated test harness
    ├── 01_valid_basic.mc
    ├── 02_operator_precedence.mc
    ├── 03_constant_folding.mc
    ├── 04_undeclared_variable.mc
    ├── 05_duplicate_declaration.mc
    ├── 06_missing_semicolon.mc
    ├── 07_invalid_char.mc
    └── 08_div_by_zero.mc
```

---

## Build Instructions

### Prerequisites
- C++17 compatible compiler (`g++` >= 8.0, `clang++` >= 7.0, or MSVC 2019+)
- CMake >= 3.12 (optional, direct g++ invocation supported)

### Option 1: Direct g++ Compilation

```bash
# Build the compiler
g++ -std=c++17 -Wall -Wextra main.cpp lexer.cpp parser.cpp semantic.cpp ir.cpp -o compiler

# Build the automated test runner
g++ -std=c++17 -Wall -Wextra tests/test_runner.cpp lexer.cpp parser.cpp semantic.cpp ir.cpp -o test_runner
```

### Option 2: CMake Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

---

## Usage & CLI Options

Compile a source file directly:

```bash
./compiler examples/basic.mc
```

### CLI Diagnostic Flags
The compiler includes inspection flags useful for debugging and interview walkthroughs:

| Flag | Description |
|------|-------------|
| `--dump-tokens` | Displays the token stream generated by the Lexer |
| `--dump-ast` | Displays a formatted ASCII visualization of the Abstract Syntax Tree |
| `--no-opt` | Disables constant folding to compare unoptimized vs. optimized IR |
| `--help`, `-h` | Prints command-line help and usage |

---

## Example Input & Output

### Input (`examples/basic.mc`):
```c
int a = 10;
int b = 20;
int c = a + b * 2;
print(c);
```

### Generated Three-Address Code Output:
```text
a = 10
b = 20
t0 = b * 2
t1 = a + t0
c = t1
PRINT c
```

---

## Testing

An automated test suite validates all functional and error-handling requirements:

```bash
./test_runner tests/
```

### Test Matrix
| Test Case | Scenario | Expected Outcome |
|-----------|----------|------------------|
| `01_Valid_Declarations` | Variable declarations, initialization, expressions | Success, valid TAC generated |
| `02_Operator_Precedence` | `*` / `/` evaluated before `+` / `-`, parentheses grouping | Success, correct operand order in IR |
| `03_Constant_Folding` | `2 + 3 * 4` compile-time evaluation | Success, folded to constant `14` |
| `04_Undeclared_Variable` | Reference to undeclared identifier | Caught by semantic analyzer with line number |
| `05_Duplicate_Declaration` | Redeclaration of variable in same scope | Caught by symbol table with line number |
| `06_Missing_Semicolon` | Statement omitted terminating `;` | Caught by parser with line number |
| `07_Invalid_Character` | Unrecognized characters (e.g. `@`) | Caught by lexer with line & column number |
| `08_Division_by_Zero_Safe`| Expression containing `/ 0` | Preserved without crash or unsafe folding |

---

## Why This Matters (System Software & NVIDIA Context)

For systems software and compiler roles:
1. **Frontend Isolation**: Clean separation between lexical analysis, syntax parsing, and semantic analysis mirrors production compiler designs (such as Clang/LLVM).
2. **Intermediate Representation (IR)**: Translating source code into Three-Address Code (TAC) decouples language syntax from target architectures, forming the foundation for machine-independent optimization passes (Dead Code Elimination, Common Subexpression Elimination).
3. **AST-Level vs IR-Level Optimizations**: Demonstrates practical understanding of compile-time transformations (constant folding) and safety preconditions (preventing illegal constant transformations like divide-by-zero).
4. **Clean Modern C++**: Employs `std::unique_ptr` for memory safety, avoids memory leaks, eliminates raw pointer ownership ambiguity, and utilizes standard idioms.

---

## Supported Syntax & Limitations

### Supported
- Integer variable declarations (`int x;`, `int x = 10;`)
- Variable assignment (`x = a + 5;`)
- Arithmetic expressions with binary operators: `+`, `-`, `*`, `/`
- Unary operators (`-x`, `+x`) and grouped expressions `(...)`
- Print statements (`print(x);`)
- Single-line comments (`// comment`)

### Known Limitations (Future Roadmap)
- No control flow (`if`/`else`, `while`, `for`)
- No floating-point or string types
- No function definitions or user-defined scopes
- Backend machine code / PTX generation not implemented (stops at Three-Address Code IR)
