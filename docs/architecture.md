# 1. Overview & Philosophy

**Language Name:** freedomLanguage (mellis)

**Source Files**
- `.fl`  — source file
- `.flh` — optional interface/header file

The compiler treats both file types uniformly. Project organization is entirely controlled by the programmer.

---

## Vision

mellis is a modern systems programming language designed for building large native software.

Its primary goal is not to reinvent memory management, but to improve the compiler architecture, module system and developer tooling while maintaining native performance and memory safety.

Game engines are one possible application—not a language-specific target.

---

## Core Goals

- Native performance comparable to Rust/C++
- Memory safety by default
- Explicit programmer control
- Zero-cost abstractions
- No Garbage Collector
- Unsafe supported where necessary
- Cross-platform
- LLVM backend

# 2. Compiler Architecture

The mellis compiler is organized as a sequence of independent compiler passes.
Source (.fl)
        │
        ▼
Lexer
        ▼
Parser
        ▼
AST

──────────────
Middle End
──────────────

Resolver
        ▼
Type Checker
        ▼
Borrow Checker
        ▼
Lifetime Analyzer
        ▼
MVIR Generator
        ▼
Optimizer

──────────────
Back End
──────────────

LLVM IR Generator
        ▼
LLVM Optimizer
        ▼
Machine Code

Each compiler pass has a single responsibility.

No pass should perform work belonging to another stage.

**Golden Rules for Every Compiler Pass:**
Before implementing any pass, we must ask 3 questions:
1. **Pass này có đúng trách nhiệm không?** (Does this pass have the right responsibility?)
2. **Có làm việc của pass khác không?** (Is it doing another pass's job?)
3. **Có dễ mở rộng không?** (Is it easy to extend?)

# 3. Module System

mellis uses semantic imports rather than textual inclusion.

Modules communicate through symbol information instead of source code inclusion.

Visibility follows an explicit export model.

Default visibility:
- private

Explicit visibility:
- export

Long-term goals:

- incremental compilation
- compiler caching
- fast linking
- hot reload

# 4. Memory Model

Memory safety is a core design goal.

mellis adopts proven concepts including:

- Ownership
- Borrow Checking
- Move Semantics
- Safe / Unsafe boundary

Primitive values are stack allocated whenever possible.

Heap allocation is explicit.

Automatic garbage collection is not part of the language.

# 5. Type System

The type system is statically checked.

Every expression has a compile-time type.

Future compiler passes include:

- type inference
- generic constraints
- trait resolution
- lifetime checking

# 6. Diagnostic Engine

Every compiler pass reports errors through a shared Diagnostic Engine.

Responsibilities include:

- syntax diagnostics
- semantic diagnostics
- type diagnostics
- borrow diagnostics

Diagnostics should provide:

- source location
- highlighted code
- notes
- suggestions

# 7. Design Principles

The following principles guide every language feature.

## Safety before convenience

Unsafe behavior must always be explicit.

---

## Compiler over syntax

Compiler architecture is considered part of the language.

---

## Libraries over language features

Game-specific concepts belong in libraries rather than the language itself.

---

## Explicit over implicit

Hidden behavior should be minimized.

---

## Zero-cost abstractions

Abstractions should not introduce unnecessary runtime overhead.

---

## Syntax follows semantics

Language syntax should emerge from a well-defined semantic model rather than forcing compiler implementation.