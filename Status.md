# 📋 Trạng thái Dự án — freedomLanguage (fdlang)

| Hạng mục | Giá trị |
|----------|---------|
| **Branch** | `main` |
| **Commit mới nhất** | TBD |
| **Phase hiện tại** | **Language Specification v1.0 Frozen** |
| **Ngôn ngữ triển khai** | C++17 |
| **Build system** | CMake 3.20+ |
| **Backend** | LLVM |
| **Cập nhật lần cuối** | 2026-07-17 |

---

## 📁 Cấu trúc Thư mục

```
fdlang/
├── include/
│   └── fdlang/
│       ├── Core/                  [NEW] Primitive shared types
│       │   ├── Types.h            SymbolID, ScopeID, FileID, sentinels
│       │   ├── SourceLocation.h   { file, line, col, offset }
│       │   ├── Identifier.h       Identifier type (→ InternID in future)
│       │   └── StringInterner.h   Stub — architecture reservation
│       ├── AST/
│       │   ├── ASTNode.h          Base + ProgramNode
│       │   ├── ExprNode.h         [MODIFIED] IdentifierExpr + symbolId
│       │   └── StmtNode.h         [MODIFIED] DeclNode base, VarDeclStmt/AssignStmtNode + symbolId
│       ├── FrontEnd/              [frozen]
│       │   ├── Lexer.h
│       │   ├── Parser.h
│       │   ├── Token.h
│       │   └── ASTVisitor.h
│       └── MiddleEnd/             [NEW] Resolver subsystem
│           ├── Symbol.h           Symbol, Scope, SymbolKind, ScopeKind, ScopeBindings
│           ├── ScopeStack.h       Transient traversal-time scope tracker
│           ├── SymbolTable.h      Persistent arena-based symbol registry
│           └── Resolver.h         Name resolution pass declaration
├── src/
│   ├── FrontEnd/                  [frozen]
│   │   ├── Lexer.cpp
│   │   └── Parser.cpp
│   ├── MiddleEnd/
│   │   ├── SymbolTable.cpp        [NEW]
│   │   ├── Resolver.cpp           [NEW]
│   │   └── SemanticAnalyzer.cpp   [DEPRECATED — excluded from build]
│   ├── BackEnd/
│   │   └── IRGenerator.cpp        [empty]
│   ├── Support/
│   │   └── Diagnostic.cpp         [empty]
│   └── main.cpp                   [UPDATED] Resolver in pipeline
├── tests/
│   ├── test_lexer.cpp
│   ├── test_sematic.cpp
│   └── test_resolver.cpp          [NEW] 12 test cases — all passing ✅
├── examples/
│   ├── ex.fl
│   └── math.flh
├── docs/
│   ├── grammar.md
│   ├── architecture.md
│   └── log.txt
├── CMakeLists.txt                  [UPDATED]
└── Status.md
```

---

## 🎯 Mục tiêu Dự án

Xây dựng ngôn ngữ lập trình **freedomLanguage** (`.fl` / `.flh`) với:
- Cú pháp hiện đại, phong cách riêng, gần gũi với Rust
- Kiểm soát bộ nhớ mạnh mẽ (Ownership / Borrowing)
- Biên dịch ra mã máy native thông qua **LLVM**
- Đủ mạnh để làm **game engine và game**

---

## 🏗️ Kiến trúc Pipeline

```
Source (.fl)
    │
    ▼
[FrontEnd]
    Lexer  →  Parser (Recursive Descent)  →  AST
    │
    ▼
[MiddleEnd]
    Resolver           ✅
    ↓
    Type Checker       ✅
    ↓
    Borrow Checker     ← chưa có
    ↓
    FLIR Generator     ✅
    │
    ▼
[BackEnd]
    LLVM IR Generator  ✅
    ↓
    LLVM Optimizer
    ↓
    Machine Code       ✅
```

---

## ✅ Đã hoàn thành

### Core/ — Primitive types (Phase 7)
| File | Nội dung |
|------|----------|
| `Types.h` | `SymbolID`, `ScopeID`, `FileID`, sentinel values |
| `SourceLocation.h` | `{ file, line, col, offset }` — diagnostic-ready |
| `Identifier.h` | `Identifier` type với transparent hash/equal, InternID path chừa sẵn |
| `StringInterner.h` | Stub — architecture slot cho full string interning |

### AST Annotations (Phase 7)
| Node | Field thêm | Ai điền |
|------|-----------|---------|
| `IdentifierExpr` | `SymbolID symbolId` | Resolver |
| `DeclNode` (base) | `SymbolID symbolId` | Resolver |
| `VarDeclStmt` | inherits `DeclNode.symbolId` | Resolver |
| `AssignStmtNode` | `SymbolID symbolId` | Resolver |

### MiddleEnd — Resolver Subsystem (Phase 7)
| Module | Trạng thái | Ghi chú |
|--------|-----------|---------|
| `Symbol.h` | ✅ | `Symbol`, `Scope`, `SymbolKind`, `ScopeKind`, `ScopeBindings` |
| `ScopeStack.h` | ✅ | Transient traversal helper |
| `SymbolTable.h/.cpp` | ✅ | Arena-based, `lookup()` chain-walk + `lookupInScope()` single-scope |
| `Resolver.h/.cpp` | ✅ | 4 responsibilities: scope, declare, resolve, shadow |
| `test_resolver.cpp` | ✅ **12/12 passed** | Full pipeline integration tests |

### Resolver — Kết quả kiểm thử
```
[OK] test01_simpleDeclarationAndUse
[OK] test02_useBeforeDeclare
[OK] test03_undeclaredIdentifier
[OK] test04_doubleDeclarationSameScope
[OK] test05_shadowingInNestedScope
[OK] test06_innerScopeVariableInvisibleOutside
[OK] test07_assignToUndeclared
[OK] test08_printResolvesArgument
[OK] test09_ifElseSeparateScopes
[OK] test10_whileLoopBodyScope
[OK] test11_declarationNoInitializer
[OK] test12_outerNameAccessibleAfterInnerScopeCloses
ALL 12 TESTS PASSED ✅
```

### FrontEnd (Phase trước)
| Module | Trạng thái |
|--------|-----------|
| Lexer | ✅ Hoàn chỉnh |
| Parser | ✅ Hoàn chỉnh — **frozen** |

### Documentation & Specification
| File | Nội dung |
|------|----------|
| `grammar.md` | ✅ **v1.0 Frozen** — Hoàn thiện toàn bộ đặc tả EBNF, OOP, Comptime, Generics, Traits, Ergonomics. |

---

## ⚠️ Đang thiếu / Chưa triển khai

### MiddleEnd & BackEnd — Đã tích hợp cho MVP
| Module | Trạng thái | Ghi chú |
|--------|-----------|---------|
| **Type Checker** | ✅ Hoàn thành | Phân tích ngữ nghĩa, kiểu dữ liệu |
| **FLIR Generator** | ✅ Hoàn thành | Lowering từ AST sang FLIR, tuân thủ `docs/flir.md`. |
| **LLVM IR Generator**| ✅ Hoàn thành | Lowering từ FLIR sang LLVM IR (Pure pass). |
| **Executable Gen** | ✅ Hoàn thành | Dịch mã máy, native code & linker abstraction (`fl::Linker`). |

---

## ⚠️ Đang thiếu / Chưa triển khai

### MiddleEnd — Cần bổ sung
| Tính năng | Trạng thái | Ghi chú |
|-----------|-----------|---------|
| **Borrow Checker** | ❌ Chưa có | `BorrowState` architecture chừa sẵn |
| **Initialization Check** | ⚠️ Chưa tích hợp | Cần chuyển vào TypeChecker |

### FrontEnd — Tính năng còn thiếu
| Tính năng | Trạng thái |
|-----------|-----------|
| `fn` — hàm | ❌ Parser chưa xử lý |
| `return` | ❌ |
| `mod`, `export`, `extern` | ❌ |
| `const` | ❌ |
| Function call `foo(a, b)` | ❌ |
| `break` / `continue` | ❌ |
| Diagnostic Engine | ✅ Đã sử dụng | Hoạt động như trung tâm điều hướng báo lỗi |

---

## 🔧 Vấn đề kỹ thuật cần chú ý

1. **Transparent lookup (C++17/MSVC)** — `unordered_map::find(string_view)` với custom hash cần C++20 trên MSVC. Hiện dùng `Identifier key(name)` explicit. Fix dài hạn: nâng C++20 hoặc dùng custom hash map.
2. **`SemanticAnalyzer.cpp` deprecated** — excluded khỏi build, chưa xóa. Initialization check cần chuyển vào TypeChecker.
3. **`SourceLocation` chưa có line/col** — Lexer chưa track. Field có sẵn, điền sau khi Lexer upgrade.
4. **`README.MD` hoàn toàn trống** — cần viết hướng dẫn cài đặt.
5. **Unused Variables** — `TypeChecker` hiện chưa báo lỗi khi biến không được dùng, khiến `FLIRGenerator` sinh ra `%id = alloca void`. Sẽ cần fix ở `TypeChecker`.
6. **LLVM Build Mode Mismatch** — LLVM trên Windows (MSVC) được build ở dạng Release (`/MD`). Khi build `fdlang` ở chế độ Debug (`/MDd`), linker sẽ văng lỗi `_ITERATOR_DEBUG_LEVEL`. Tạm thời luôn build `fdlang` ở Release mode.

---

## 📊 Tiến độ tổng thể

```
FrontEnd (Lexer + Parser)       ████████░░   ~80%
MiddleEnd — Resolver            ██████████   100% ✅
MiddleEnd — TypeChecker         ██████████   100% ✅
MiddleEnd — FLIR Generator      ██████████   100% ✅
MiddleEnd — Borrow Checker      ░░░░░░░░░░     0%
BackEnd — LLVM IR Gen           ██████████   100% ✅
BackEnd — Executable Gen        ██████████   100% ✅
Tests                           ██████████   100% ✅
Documentation                   █████████░   ~90%
──────────────────────────────────────────────────
Tổng thể MVP                    ██████████   100% ✅
```

---

## 🗺️ Bước tiếp theo

### Ngắn hạn
1. **Triển khai Lexer.cpp & Parser.cpp** — Cập nhật mã nguồn C++ để bám sát `grammar.md` v1.0.
2. **Diagnostic Engine** — cải thiện xuất lỗi, thay `std::exit()`
3. **Initialization checker** — chuyển logic từ SemanticAnalyzer cũ vào pass mới

### Trung hạn
4. **Mở rộng chức năng** — dịch LLVM IR cho if/else, loop, function call
5. **Borrow Checker** — Bắt đầu implement Ownership/Borrowing cơ bản

### Dài hạn
6. **Module system** — `mod`, `export`, `extern`
7. **StringInterner** — upgrade `Identifier` từ `std::string` sang `InternID`

---

*File được cập nhật sau khi hoàn thành MVP (đã compile ra executable code).*
