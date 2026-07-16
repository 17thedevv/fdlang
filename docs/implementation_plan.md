# Resolver Subsystem — fdlang Implementation Plan

## Background

The current `SemanticAnalyzer` violates the single-responsibility principle: it does scope management, name resolution, **and** initialization checking all in one pass. This works for an MVP but cannot survive language evolution.

This plan redesigns the MiddleEnd from scratch with a clean Resolver whose only job is **name resolution**.

---

## Architecture Philosophy

### Core Principle: One Pass, One Job

```
Parser      → builds AST (nodes, no meaning)
Resolver    → connects identifiers to symbols  ← THIS PLAN
TypeChecker → verifies types
BorrowChecker → ownership rules
FLIR        → IR generation
```

Each pass reads the previous pass's output and writes annotations. No pass does another's work.

---

## Comparison: rustc / Clang / Swift

| Aspect | rustc | Clang | Swift | fdlang (this plan) |
|--------|-------|-------|-------|-------------------|
| Symbol ID | `DefId` (crate_id + local_id) | `DeclContext*` pointer | `DeclContext*` | `SymbolID` (u32 index) |
| Scope model | `Rib` stack (one Rib per scope kind) | `DeclContext` hierarchy | `ASTScope` tree | `ScopeStack` + `ScopeID` |
| Symbol store | `Definitions` arena | `ASTContext` | `ASTContext` | `SymbolTable` arena |
| Resolution output | `ResolutionMap<NodeId, Res>` | in-place on `NamedDecl*` | in-place on `DeclRefExpr*` | in-place on `IdentifierExpr.symbolId` |
| Separation from types | Strict — resolve before ty | Weak — Sema does both | Moderate | Strict |

**Key difference from Clang**: Clang's `Sema` class does name resolution AND type checking in a single pass. This makes Clang extremely fast but hard to extend. fdlang follows rustc's philosophy: strict pass separation.

**Key difference from rustc**: rustc uses a 2-level `DefId (crate_id, local_id)` because it handles multiple crates. fdlang uses a flat `uint32_t` — simpler for a single-crate MVP, extensible later by making it a struct.

---

## Design: Data Structures

### 1. `SymbolID` and `ScopeID`

```cpp
// include/fdlang/Core/Types.h
using SymbolID = uint32_t;
using ScopeID  = uint32_t;
constexpr SymbolID kInvalidSymbolID = UINT32_MAX;
constexpr ScopeID  kInvalidScopeID  = UINT32_MAX;
```

**Why not `std::optional<SymbolID>`?**
Every AST node stores a SymbolID. Using `optional` adds 8 bytes overhead per node (alignment). A sentinel value is the industry standard (rustc, LLVM all use sentinel IDs for "not set yet").

### 2. `SymbolKind` and `ScopeKind`

```
SymbolKind:  Variable | Constant | Function | Parameter |
             Struct | Enum | EnumVariant | Module | Trait | BuiltinStatement
             
ScopeKind:   Global | Module | Function | Block | Loop | Conditional
```

`BuiltinStatement` covers `print` for now. When `print` moves to stdlib, we swap its SymbolKind to `Function` with zero AST changes.

### 3. `Symbol` — minimal, intentionally

```cpp
struct Symbol {
    SymbolID    id;
    ScopeID     declaredInScope;
    std::string name;        // owned — safe across AST lifetime
    SymbolKind  kind;
    uint32_t    declOffset;  // for diagnostics only
    
    // NOT here: type, borrow state, llvm::Value*, isInitialized
    // Those belong to later passes
};
```

**Why `std::string` not `string_view`?**
`string_view` is dangerous — the source buffer may be freed or reallocated before type checking runs. `Symbol` owns its name.

### 4. `Scope`

```cpp
struct Scope {
    ScopeID  id;
    ScopeID  parentId;   // kInvalidScopeID if global
    ScopeKind kind;
    std::unordered_map<std::string, SymbolID> bindings;
};
```

### 5. `SymbolTable` — pure data store

```cpp
class SymbolTable {
    std::vector<Symbol> arena_;   // indexed by SymbolID — O(1) access
    std::vector<Scope>  scopes_;  // indexed by ScopeID  — O(1) access
public:
    ScopeID  createScope(ScopeKind kind, ScopeID parentId);
    SymbolID declareSymbol(string_view name, SymbolKind, ScopeID, uint32_t offset);
    std::optional<SymbolID> lookup(string_view name, ScopeID fromScope) const;
    const Symbol& getSymbol(SymbolID) const;
    Symbol&       getMutableSymbol(SymbolID);
    const Scope&  getScope(ScopeID) const;
};
```

**Why separate from Resolver?** SymbolTable is a dumb data store. Future passes (TypeChecker, BorrowChecker, IRGenerator) all need to query it. Keeping traversal logic in Resolver means SymbolTable has zero dependencies on AST.

### 6. `ScopeStack` — traversal-time only

```cpp
class ScopeStack {
    std::vector<ScopeID> stack_;
public:
    void    push(ScopeID id);
    void    pop();
    ScopeID current() const;
    size_t  depth() const;
};
```

Exists only during a Resolver pass. After Resolver finishes, ScopeStack is discarded. SymbolTable retains all scope data permanently.

### 7. `Resolver`

```cpp
class Resolver : public ASTVisitor {
    SymbolTable& table_;
    ScopeStack   scopeStack_;
    size_t       errorCount_ = 0;
public:
    explicit Resolver(SymbolTable& table);
    bool   resolve(ProgramNode* program);
    size_t errorCount() const;
    
    // ASTVisitor overrides...
private:
    void     enterScope(ScopeKind kind);
    void     exitScope();
    SymbolID declare(string_view name, SymbolKind kind, uint32_t offset);
    SymbolID resolveId(string_view name, uint32_t offset);
    void     reportError(const std::string& msg);
};
```

---

## AST Annotations (Minimal Changes)

> [!IMPORTANT]
> Parser is **frozen**. We only ADD fields to existing nodes — zero parser logic changes.

### `IdentifierExpr` (ExprNode.h)
```diff
 class IdentifierExpr : public ExprNode {
 public:
     std::string_view name;
+    SymbolID symbolId = kInvalidSymbolID;  // filled by Resolver
```

### `VarDeclStmt` (StmtNode.h)
```diff
 class VarDeclStmt : public StmtNode {
 public:
     std::string_view varName;
     std::unique_ptr<ExprNode> initializer;
+    SymbolID symbolId = kInvalidSymbolID;  // filled by Resolver
```

### `AssignStmtNode` (StmtNode.h)
```diff
 class AssignStmtNode : public StmtNode {
 public:
     std::string_view varName;
     std::unique_ptr<ExprNode> value;
+    SymbolID symbolId = kInvalidSymbolID;  // filled by Resolver
```

---

## Folder Layout (After This Plan)

```
include/fdlang/
├── Core/
│   └── Types.h              [NEW] — SymbolID, ScopeID, sentinel values
├── AST/
│   ├── ASTNode.h            [unchanged]
│   ├── ExprNode.h           [MODIFY] — add symbolId to IdentifierExpr
│   └── StmtNode.h           [MODIFY] — add symbolId to VarDeclStmt, AssignStmtNode
├── FrontEnd/                [frozen — parser untouched]
│   ├── ASTVisitor.h
│   ├── Lexer.h
│   ├── Parser.h
│   └── Token.h
└── MiddleEnd/               [NEW directory]
    ├── Symbol.h             [NEW] — Symbol, Scope, SymbolKind, ScopeKind structs
    ├── SymbolTable.h        [NEW] — clean SymbolTable (replaces FrontEnd/SymbolTable.h)
    ├── ScopeStack.h         [NEW] — ScopeStack helper
    └── Resolver.h           [NEW] — Resolver declaration

src/
├── FrontEnd/                [frozen]
│   ├── Lexer.cpp
│   └── Parser.cpp
├── MiddleEnd/
│   ├── SymbolTable.cpp      [NEW]
│   ├── ScopeStack.cpp       [NEW]
│   ├── Resolver.cpp         [NEW]
│   └── SemanticAnalyzer.cpp [DEPRECATED — left in place, removed from pipeline]
├── BackEnd/
│   └── IRGenerator.cpp      [unchanged — empty]
├── Support/
│   └── Diagnostic.cpp       [unchanged — empty]
└── main.cpp                 [MODIFY] — add Resolver to pipeline

tests/
├── test_lexer.cpp           [unchanged]
├── test_sematic.cpp         [unchanged]
└── test_resolver.cpp        [NEW] — 12 test cases
```

> [!WARNING]
> `FrontEnd/SymbolTable.h` and `FrontEnd/SemanticAnalyzer.h` will be **deprecated but NOT deleted** in this phase. They are orphaned — no longer connected to the pipeline. Deletion is a separate cleanup commit.

---

## Class Diagram

```
┌─────────────────────────────────────────────────────┐
│                    Core/Types.h                      │
│  SymbolID (u32)    ScopeID (u32)                    │
│  kInvalidSymbolID  kInvalidScopeID                  │
└─────────────────┬───────────────────────────────────┘
                  │ (included by)
        ┌─────────┴──────────┐
        │                    │
┌───────▼────────┐   ┌───────▼──────────────────────┐
│  AST/ExprNode  │   │    MiddleEnd/Symbol.h         │
│  IdentifierExpr│   │  SymbolKind enum              │
│  + symbolId    │   │  ScopeKind  enum              │
└────────────────┘   │  Symbol struct                │
                     │  Scope  struct                │
┌───────────────┐    └──────────┬───────────────────┘
│  AST/StmtNode │               │
│  VarDeclStmt  │    ┌──────────▼───────────────────┐
│  + symbolId   │    │   MiddleEnd/SymbolTable.h     │
│  AssignStmt   │    │  - arena_: vector<Symbol>     │
│  + symbolId   │    │  - scopes_: vector<Scope>     │
└───────────────┘    │  + createScope()              │
                     │  + declareSymbol()            │
                     │  + lookup()                   │
                     │  + getSymbol()                │
                     └──────────┬────────────────────┘
                                │ (owns)
                     ┌──────────▼────────────────────┐
                     │   MiddleEnd/ScopeStack.h       │
                     │  - stack_: vector<ScopeID>    │
                     │  + push() / pop()             │
                     │  + current()                  │
                     └──────────┬────────────────────┘
                                │ (used by)
                     ┌──────────▼────────────────────┐
                     │   MiddleEnd/Resolver.h         │
                     │  extends ASTVisitor            │
                     │  - table_: SymbolTable&        │
                     │  - scopeStack_: ScopeStack     │
                     │  + resolve(ProgramNode*)       │
                     └───────────────────────────────┘
```

---

## Data Flow

```
Source code (.fl)
      │
      ▼
[Lexer] → Token stream
      │
      ▼
[Parser] → AST (IdentifierExpr.symbolId = kInvalidSymbolID)
      │
      ▼
[Resolver]
  ├── enterScope(Global)
  ├── visit(VarDeclStmt "x")
  │     ├── SymbolTable.createScope(...)   ← if needed
  │     ├── SymbolTable.declareSymbol("x", Variable, currentScope, offset)
  │     └── node.symbolId = newId          ← AST annotated ✓
  ├── visit(IdentifierExpr "x")
  │     ├── SymbolTable.lookup("x", currentScope)
  │     └── node.symbolId = foundId        ← AST annotated ✓
  └── exitScope()
      │
      ▼
AST (all IdentifierExpr.symbolId resolved)
      │
      ▼
[TypeChecker] (future) — reads symbolId, queries SymbolTable
      │
      ▼
[BorrowChecker] (future)
      │
      ▼
[IRGenerator] — reads symbolId → looks up llvm::Value* from its own map
```

---

## Resolver Logic: Shadowing

```
// Outer scope
dec x = 10;         // symbolId = 1

{   // Inner scope (enterScope)
    dec x = 20;     // symbolId = 2  ← new Symbol, shadows outer x
    print x;        // resolves to symbolId = 2  ✓
}   // exitScope → outer x (symbolId=1) visible again

print x;            // resolves to symbolId = 1  ✓
```

`SymbolTable.lookup()` walks scope chain from innermost to outermost, returning the **first** match. This naturally handles shadowing.

---

## Implementation Order (9 Commits)

| # | Commit | Files | Description |
|---|--------|-------|-------------|
| 1 | `feat(core): add Types.h` | `include/fdlang/Core/Types.h` | SymbolID, ScopeID, sentinels |
| 2 | `feat(ast): annotate nodes with symbolId` | `ExprNode.h`, `StmtNode.h` | Add symbolId fields |
| 3 | `feat(middleend): add Symbol.h` | `include/fdlang/MiddleEnd/Symbol.h` | Symbol, Scope, enums |
| 4 | `feat(middleend): add ScopeStack` | `ScopeStack.h`, `ScopeStack.cpp` | Traversal-time scope tracking |
| 5 | `feat(middleend): add SymbolTable` | `SymbolTable.h`, `SymbolTable.cpp` | Arena + lookup chain |
| 6 | `feat(middleend): add Resolver` | `Resolver.h`, `Resolver.cpp` | Name resolution pass |
| 7 | `feat(main): wire Resolver into pipeline` | `main.cpp` | Replace SemanticAnalyzer |
| 8 | `test: add Resolver unit tests` | `test_resolver.cpp` | 12 test cases |
| 9 | `build: update CMakeLists.txt` | `CMakeLists.txt` | Add new src + test target |

---

## Refactor: Existing SymbolTable

The current `FrontEnd/SymbolTable.h` has these issues:

| Issue | Problem | Fix in new design |
|-------|---------|-------------------|
| Lives in FrontEnd/ | SymbolTable is not a frontend concern | Moved to MiddleEnd/ |
| Mixed traversal + storage | `scopes_` vec acts as both scope data and active stack | Split into `SymbolTable` (data) + `ScopeStack` (traversal) |
| `string_view` for names | Dangling if source buffer freed | `std::string` in Symbol |
| Borrow/Ownership fields | Not resolver's job | Removed from Symbol |
| Logic in header | `lookupId()` / `getSymbol()` in .h | Moved to .cpp |
| No ScopeID concept | Can't query "which scope is this in?" | `ScopeID` as first-class ID |

---

## Edge Cases Covered

1. **Undeclared identifier**: `print z;` where z never declared → Resolver error
2. **Double declaration same scope**: `dec x; dec x;` → Resolver error
3. **Shadowing in nested scope**: inner `dec x` shadows outer `dec x` → OK
4. **Use before declare**: `print x; dec x;` → Resolver error (lookup fails)
5. **Assignment to undeclared**: `y = 5;` where y never declared → error
6. **Empty block**: `{}` → enter/exit scope, no crash
7. **Deeply nested scopes**: `{ { { dec x; } print x; } }` → outer print fails ✓
8. **`print` as builtin**: resolves without looking up in SymbolTable
9. **Shadowing across if/else branches**: each branch is a separate scope
10. **While loop body scope**: variables declared inside while not visible after
11. **Declaration with no initializer**: `dec x;` → symbolId assigned, no crash
12. **Re-use name after inner scope closes**: outer x accessible after inner x's scope closes

---

## Future Extensibility

| Feature | What changes |
|---------|-------------|
| Functions `fn` | Add `SymbolKind::Function`, `ScopeKind::Function`. Resolver declares function name, opens function scope for params |
| Structs | `SymbolKind::Struct`. Resolver creates a Module-like scope for fields |
| Modules `mod` | `ScopeKind::Module`. `lookup()` already walks parent chain |
| `print` → stdlib | Change SymbolKind from `BuiltinStatement` to `Function`. Zero AST changes |
| Multi-crate | Extend `SymbolID` from `u32` to `struct { crate_id, local_id }` |
| Generics | Resolver ignores type params (TypeChecker's job). No resolver changes |
| Traits | `SymbolKind::Trait`. Resolver resolves trait names; impl checking is TypeChecker's job |

---

## Open Questions

> [!IMPORTANT]
> **Q1: Old SemanticAnalyzer** — The existing `SemanticAnalyzer` does initialization checking (`isInitialized`). After Resolver replaces it in the pipeline, where does initialization checking live?
>
> **Option A**: Keep a trimmed `SemanticAnalyzer` running after Resolver (reads `symbolId`, checks init state)
> **Option B**: Move to TypeChecker (deferred)
> **Recommendation**: Option A for now — keeps us unblocked while TypeChecker is not yet built.

> [!IMPORTANT]
> **Q2: `print` in Resolver** — Should `print` be completely transparent to Resolver (Resolver just resolves its argument expression, ignoring the `print` keyword itself), or should Resolver also annotate it with a `SymbolID` pointing to a built-in `print` symbol?
>
> **Recommendation**: Transparent — Resolver visits `PrintStmtNode` by resolving its argument expression only. No SymbolID on the `print` keyword itself. When `print` moves to stdlib, it becomes a `FuncCallExpr` whose `IdentifierExpr("print")` gets a SymbolID naturally.

*Proceed to execute? Approve to begin Commit 1 → 9.*
