# Yêu cầu phiên bản CMake tối thiểu
cmake_minimum_required(VERSION 3.20)

# Khai báo tên dự án và ngôn ngữ sử dụng
project(fdlang VERSION 1.0.0 LANGUAGES CXX)

# Ép buộc sử dụng C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# PHẦN 1: TÌM VÀ CẤU HÌNH LLVM
# ==========================================
list(APPEND CMAKE_PREFIX_PATH "D:/Programs/LLVM-DEV")
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

link_directories("C:/Program Files/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64")
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS Core ExecutionEngine Interpreter Support native)

# ==========================================
# PHẦN 2: CẤU HÌNH DỰ ÁN FDLANG
# ==========================================

# Thư mục include chính
include_directories(include)

# ── Source files ──────────────────────────────────────────────────────────────
# Gom Frontend sources (parser frozen — no changes)
file(GLOB FRONTEND_SRC "src/FrontEnd/*.cpp")

# MiddleEnd sources — explicit list to exclude deprecated SemanticAnalyzer.cpp
set(MIDDLEEND_SRC
    src/MiddleEnd/SymbolTable.cpp
    src/MiddleEnd/Resolver.cpp
    # src/MiddleEnd/SemanticAnalyzer.cpp  <- DEPRECATED: replaced by Resolver
    # src/MiddleEnd/SymbolTable.cpp (old FrontEnd version) <- DEPRECATED
)

set(BACKEND_SRC
    src/BackEnd/IRGenerator.cpp
)

set(SUPPORT_SRC
    src/Support/Diagnostic.cpp
)

# Tạo file thực thi chính
add_executable(fdlang
    src/main.cpp
    ${FRONTEND_SRC}
    ${MIDDLEEND_SRC}
    ${BACKEND_SRC}
    ${SUPPORT_SRC}
)

# Liên kết với LLVM
target_link_libraries(fdlang PRIVATE ${LLVM_LIBS})

# ==========================================
# PHẦN 3: KIỂM THỬ (TESTING)
# ==========================================
enable_testing()

# ── test_lexer ────────────────────────────────────────────────────────────────
add_executable(test_lexer
    tests/test_lexer.cpp
    src/FrontEnd/Lexer.cpp
)
target_include_directories(test_lexer PRIVATE include)
add_test(NAME LexerTest COMMAND test_lexer)

# ── test_resolver ─────────────────────────────────────────────────────────────
# Cần: Lexer, Parser, SymbolTable, Resolver, DiagnosticEngine
add_executable(test_resolver
    tests/test_resolver.cpp
    src/FrontEnd/Lexer.cpp
    src/FrontEnd/Parser.cpp
    src/MiddleEnd/SymbolTable.cpp
    src/MiddleEnd/Resolver.cpp
    src/Support/Diagnostic.cpp
)
target_include_directories(test_resolver PRIVATE include)
add_test(NAME ResolverTest COMMAND test_resolver)

# ── test_diagnostic ───────────────────────────────────────────────────────────
# Cần: DiagnosticEngine only (no Lexer/Parser/Resolver)
add_executable(test_diagnostic
    tests/test_diagnostic.cpp
    src/Support/Diagnostic.cpp
)
target_include_directories(test_diagnostic PRIVATE include)
add_test(NAME DiagnosticTest COMMAND test_diagnostic)