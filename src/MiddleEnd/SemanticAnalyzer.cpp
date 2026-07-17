#include "fdlang/MiddleEnd/Symbol/SymbolTable.h"
#include "fdlang/MiddleEnd/SemanticAnalyzer.h"
#include "fdlang/AST/ASTNode.h"
#include "fdlang/AST/ExprNode.h"
#include "fdlang/AST/StmtNode.h"
#include <iostream>
#include <cstdlib>

namespace fl {

void SemanticAnalyzer::visit(ProgramNode* node) {
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
}

// --- CÁC CÂU LỆNH (STATEMENTS) ---

void SemanticAnalyzer::visit(VarDeclStmt* node) {
    bool isInit = false;

    if (node->initializer) {
        node->initializer->accept(this);
        isInit = true; // Nếu có giá trị khởi tạo -> bật cờ!
    }

    // Truyền cờ isInit vào bảng ký hiệu
    auto newSymbolId = symbolTable.declareSymbol(node->varName, false, 0, isInit); 
    
    if (!newSymbolId.has_value()) {
        std::cerr << "Loi Semantic: Bien '" << node->varName << "' da duoc khai bao truoc do trong scope nay!\n";
        std::exit(65);
    }
}

void SemanticAnalyzer::visit(AssignStmtNode* node) {
    // 1. Kiểm tra vế phải trước
    node->value->accept(this);

    // 2. Tìm ID của biến
    auto id = symbolTable.lookupId(node->varName);
    if (!id.has_value()) {
        std::cerr << "Loi Semantic: Khong the gan gia tri. Bien '" << node->varName << "' chua duoc khai bao!\n";
        std::exit(65);
    }

    // 3. Lấy con trỏ từ Arena an toàn thông qua ID và đánh dấu đã khởi tạo
    SymbolInfo& sym = symbolTable.getSymbol(id.value());
    sym.isInitialized = true;
}

void SemanticAnalyzer::visit(BlockStmtNode* node) {
    symbolTable.enterScope(); // Mở scope mới
    for (auto& stmt : node->statements) {
        stmt->accept(this);
    }
    symbolTable.exitScope(); // Đóng scope
}

void SemanticAnalyzer::visit(IfStmtNode* node) {
    node->condition->accept(this);
    node->thenBranch->accept(this);
    if (node->elseBranch) {
        node->elseBranch->accept(this);
    }
}

void SemanticAnalyzer::visit(WhileStmtNode* node) {
    node->condition->accept(this);
    node->body->accept(this);
}

void SemanticAnalyzer::visit(PrintStmtNode* node) {
    node->value->accept(this);
}

// --- CÁC BIỂU THỨC (EXPRESSIONS) ---

void SemanticAnalyzer::visit(IdentifierExpr* node) {
    // 1. Tìm ID của biến
    auto id = symbolTable.lookupId(node->name);
    
    if (!id.has_value()) {
        std::cerr << "Loi Semantic: Khong tim thay bien '" << node->name << "'.\n";
        std::exit(65);
    }

    // 2. Lấy con trỏ an toàn và kiểm tra trạng thái khởi tạo
    SymbolInfo& sym = symbolTable.getSymbol(id.value());

    // LUẬT CỦA RUST: Đã khai báo nhưng chưa gán giá trị mà dám gọi ra? CHÉM!
    if (!sym.isInitialized) {
        std::cerr << "Loi Semantic: Bien '" << node->name 
                  << "' duoc su dung truoc khi duoc gan gia tri!\n";
        std::exit(65);
    }
}

void SemanticAnalyzer::visit(BinaryExpr* node) {
    node->left->accept(this);
    node->right->accept(this);
}

void SemanticAnalyzer::visit(NumberExpr* node) {
    // Không cần làm gì, số là hợp lệ
}

void SemanticAnalyzer::visit(BooleanExpr* node) {
    // Không cần làm gì, boolean là hợp lệ
}

} // namespace fl