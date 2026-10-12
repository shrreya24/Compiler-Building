# Mini C Compiler (C++17)

A simple, beginner-friendly **Mini C Compiler** written in **C++17** that translates a small C-like language into **Three-Address Intermediate Representation (IR)**. Designed for college portfolios and systems software engineering interviews (e.g., NVIDIA System Software Intern).

---

## 🚀 Quick Start (Single File Compilation)

Compile and run with just **1 source file**:

```bash
# 1. Compile the compiler
g++ -std=c++17 -Wall -Wextra compiler.cpp -o compiler

# 2. Run on basic example
./compiler basic.mc
```

---

## 📌 Example

### Input (`basic.mc`):
```c
int a = 10;
int b = 20;
int c = a + b * 2;
print(c);
```

### Generated Three-Address Code (TAC):
```text
a = 10
b = 20
t0 = b * 2
t1 = a + t0
c = t1
PRINT c
```

---

## 🏗️ Compiler Architecture & Pipeline

```
Source Code (.mc)
       │
       ▼
 ┌───────────┐
 │   Lexer   │  Tokenizes keywords, identifiers, numbers, operators
 └───────────┘
       │
       ▼
 ┌───────────┐
 │  Parser   │  Recursive-descent LL(1) parser building Abstract Syntax Tree (AST)
 └───────────┘
       │
       ▼
 ┌───────────┐
 │ Semantics │  Symbol Table tracking variables, undeclared/redefinition checks
 └───────────┘
       │
       ▼
 ┌───────────┐
 │ Optimizer │  Constant Folding (e.g., 2 + 3 * 4 -> 14, safe div-by-zero)
 └───────────┘
       │
       ▼
 ┌───────────┐
 │    IR     │  Three-Address Code (TAC) emission with virtual registers (t0, t1...)
 └───────────┘
```

---

## 📁 Project Structure

```text
compiler projects/
├── compiler.cpp      <-- Complete, all-in-one C++17 compiler (clean & beginner-friendly)
├── basic.mc          <-- Valid example source code
├── invalid.mc        <-- Invalid code demonstrating error handling
├── README.md         <-- This documentation
└── mini-c-compiler/  <-- Modular multi-file version with CMake & test suite
```

---

## 🧪 Testing Error Handling

Run against `invalid.mc`:
```bash
./compiler invalid.mc
```

Outputs clear, line-numbered diagnostic errors:
```text
[Semantic Error] Line 3, Column 5: Redefinition of variable 'x' (previously declared at line 2)
[Semantic Error] Line 4, Column 1: Assignment to undeclared variable 'y'
[Semantic Error] Line 5, Column 7: Use of undeclared variable 'z'
```
