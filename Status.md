# 📋 Trạng thái Dự án — freedomLanguage (fdlang)

| Hạng mục | Giá trị |
|----------|---------|
| **Branch** | `main` |
| **Commit mới nhất** | `f499c3b` — *parser first version* |
| **Phase hiện tại** | **Phase 7 — Resolver** |
| **Ngôn ngữ triển khai** | C++17 |
| **Build system** | CMake 3.20+ |
| **Backend** | LLVM (future) |
| **Cập nhật lần cuối** | 2026-07-16 |

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
    Resolver           ← Phase 7 hiện tại ✅
    ↓
    Type Checker       ← chưa có
    ↓
    Borrow Checker     ← chưa có
    ↓
    FLIR Generator     ← chưa có
    │
    ▼
[BackEnd]
    LLVM IR Generator  ← chưa có
    ↓
    LLVM Optimizer
    ↓
    Machine Code
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

---

## ⚠️ Đang thiếu / Chưa triển khai

### BackEnd — Ưu tiên cao nhất
| Module | Trạng thái |
|--------|-----------|
| `IRGenerator.cpp` | ❌ Trống |
| LLVM IR Emission | ❌ Chưa bắt đầu |

### MiddleEnd — Cần bổ sung
| Tính năng | Trạng thái | Ghi chú |
|-----------|-----------|---------|
| **Type Checker** | ❌ Chưa có | Đọc `symbolId`, kiểm tra kiểu |
| **Borrow Checker** | ❌ Chưa có | `BorrowState` architecture chừa sẵn |
| **FLIR** | ❌ Chưa có | IR nội bộ trước LLVM |
| **Initialization Check** | ⚠️ SemanticAnalyzer deprecated | Cần chuyển vào TypeChecker |

### FrontEnd — Tính năng còn thiếu
| Tính năng | Trạng thái |
|-----------|-----------|
| `fn` — hàm | ❌ Parser chưa xử lý |
| `return` | ❌ |
| `mod`, `export`, `extern` | ❌ |
| `const` | ❌ |
| Function call `foo(a, b)` | ❌ |
| `break` / `continue` | ❌ |
| Diagnostic Engine | ❌ `Diagnostic.cpp` trống |

---

## 🔧 Vấn đề kỹ thuật cần chú ý

1. **Transparent lookup (C++17/MSVC)** — `unordered_map::find(string_view)` với custom hash cần C++20 trên MSVC. Hiện dùng `Identifier key(name)` explicit. Fix dài hạn: nâng C++20 hoặc dùng custom hash map.
2. **`SemanticAnalyzer.cpp` deprecated** — excluded khỏi build, chưa xóa. Initialization check cần chuyển vào TypeChecker.
3. **`SourceLocation` chưa có line/col** — Lexer chưa track. Field có sẵn, điền sau khi Lexer upgrade.
4. **`README.MD` hoàn toàn trống** — cần viết hướng dẫn cài đặt.

---

## 📊 Tiến độ tổng thể

```
FrontEnd (Lexer + Parser)       ████████░░   ~70%
MiddleEnd — Resolver            ██████████   100% ✅
MiddleEnd — Type/Borrow/FLIR    ░░░░░░░░░░     0%
BackEnd (LLVM IR)               ░░░░░░░░░░     0%
Tests                           █████░░░░░   ~40%
Documentation                   █████░░░░░   ~50%
──────────────────────────────────────────────────
Tổng thể (Phase 7)              ████░░░░░░   ~35%
```

---

## 🗺️ Bước tiếp theo

### Ngắn hạn
1. **Hoàn thiện Parser** — thêm `fn`, `return`, function call, `const`
2. **Diagnostic Engine** — thay `std::exit()` và `cerr` bằng hệ thống lỗi đẹp
3. **Initialization checker** — chuyển logic từ SemanticAnalyzer cũ vào pass mới

### Trung hạn
4. **Type Checker** — đọc `symbolId`, kiểm tra kiểu trong biểu thức
5. **IRGenerator** — bắt đầu emit LLVM IR cho các construct đơn giản

### Dài hạn
6. **Borrow Checker** — Ownership/Borrowing đầy đủ
7. **Module system** — `mod`, `export`, `extern`
8. **StringInterner** — upgrade `Identifier` từ `std::string` sang `InternID`

---

*File được cập nhật sau Phase 7 — Resolver subsystem hoàn thành.*
