#pragma once
#include <string_view>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace fl {

// 1. Khai báo kiểu ID lên trên cùng để các Struct bên dưới có thể sử dụng
using SymbolId = uint32_t;

// 2. Trạng thái mượn của biến
enum class BorrowState {
    None,       // Không ai mượn
    Shared,     // Đang có người mượn để ĐỌC (có thể > 1 người)
    Exclusive   // Đang có 1 người mượn để GHI
};

// 3. Cấu trúc dữ liệu của 1 biến (Đã hợp nhất và dọn dẹp)
struct SymbolInfo {
    SymbolId id;
    std::string_view name;
    
    // --- 1. Dữ liệu ngữ nghĩa cơ bản ---
    bool isMutable;            // Biến let hay let mut
    uint32_t declaredAtOffset; // Vị trí byte để in lỗi đỏ
    int scopeLevel;            // Độ sâu của Scope (0: Global, 1: Hàm, 2: Vòng lặp...) -> Xác định Lifetime

    // --- 2. Hệ thống Ownership (Quyền sở hữu) ---
    bool isMoved;              // Đã bị move đi chỗ khác chưa?
    
    // --- 3. Hệ thống Borrowing (Mượn) ---
    BorrowState borrowState;   // Trạng thái mượn hiện tại
    int sharedBorrowersCount;  // Nếu là Shared, đếm xem bao nhiêu người đang mượn đọc
    
    // Nếu biến này là một con trỏ Reference (người đi mượn), nó mượn của ai?
    std::optional<SymbolId> borrowsFrom; 
    bool isInitialized;
};

class SymbolTable {
private:
    // 1. THE ARENA: Nơi lưu trữ vĩnh viễn mọi biến trong suốt quá trình biên dịch
    std::vector<SymbolInfo> arena;

    // 2. THE SCOPE STACK: Ngăn xếp quản lý tầm nhìn (Visibility)
    using Scope = std::unordered_map<std::string_view, SymbolId>;
    std::vector<Scope> scopes;

public:
    SymbolTable() {
        // Khởi tạo sẵn Scope toàn cục (Global Scope) ở độ sâu 0
        scopes.push_back(Scope{});
    }

    // --- Quản lý Scope ---
    void enterScope() {
        scopes.push_back(Scope{});
    }

    void exitScope() {
        if (scopes.size() > 1) {
            scopes.pop_back(); // Chỉ xóa quyền nhìn thấy, dữ liệu trong 'arena' vẫn còn nguyên!
        }
    }

    // --- Thao tác với Biến ---
    
    // Đăng ký biến mới vào Scope hiện tại (Trả về ID)
    std::optional<SymbolId> declareSymbol(std::string_view name, bool isMut, uint32_t offset, bool isInitialized) {
        auto& currentScope = scopes.back();
        
        // Báo lỗi nếu biến đã tồn tại trong CÙNG MỘT scope
        if (currentScope.find(name) != currentScope.end()) {
            return std::nullopt; 
        }

        SymbolId newId = static_cast<SymbolId>(arena.size());
        
        // Độ sâu của scope hiện tại (Global = 0, vòng lặp đầu tiên = 1...)
        int currentScopeLevel = static_cast<int>(scopes.size() - 1);
        
        // Tạo Symbol mới với các trường được khởi tạo chuẩn xác cho O/B
        arena.push_back({
            newId,                  // id
            name,                   // name
            isMut,                  // isMutable
            offset,                 // declaredAtOffset
            currentScopeLevel,      // scopeLevel
            false,                  // isMoved
            BorrowState::None,      // borrowState
            0,                      // sharedBorrowersCount
            std::nullopt,            // borrowsFrom
            isInitialized            // isInitialized
        });
        
        // Cập nhật tầm nhìn cho Scope hiện tại
        currentScope[name] = newId;
        return newId;
    }

    // Tìm kiếm biến từ Scope trong cùng ra ngoài Global
    // Trả về std::optional<SymbolId> để xử lý trường hợp không tìm thấy
std::optional<SymbolId> lookupId(std::string_view name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second; // Trả về ID (index)
        }
    }
    return std::nullopt; // Không tìm thấy
}
SymbolInfo& getSymbol(SymbolId id) {
    return arena[id]; // Trả về tham chiếu (reference) an toàn
}
};


} // namespace fl