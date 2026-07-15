#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <iomanip>
#include <cstdlib>
#include "fdlang/Frontend/Lexer.h"

using namespace fl;

// 1. Hàm đọc toàn bộ nội dung file vào chuỗi siêu tốc
std::string readFile(const std::string& filepath) {
    // Mở file ở chế độ binary và đặt con trỏ ở cuối file (ate = at end) để lấy kích thước
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Loi: Khong the mo file '" << filepath << "'\n";
        std::exit(74); // Mã lỗi chuẩn (EX_IOERR)
    }

    // Lấy kích thước file và cấp phát chuỗi 1 lần duy nhất
    std::streamsize size = file.tellg();
    std::string buffer(size, '\0');

    // Quay lại đầu file và đọc ụp toàn bộ dữ liệu vào buffer
    file.seekg(0, std::ios::beg);
    if (file.read(buffer.data(), size)) {
        return buffer;
    } else {
        std::cerr << "Loi: Khong the doc noi dung file '" << filepath << "'\n";
        std::exit(74);
    }
}

// Hàm in Token (Giữ nguyên như cũ)
const char* tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR:       return "ERROR";
        case TokenType::KW_DEC:      return "KW_DEC";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::EQUAL:       return "EQUAL";
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::SEMI:        return "SEMI";
        default:                     return "OTHER";
    }
}

// 2. Hàm main nay nhận tham số dòng lệnh (Command-line arguments)
int main(int argc, char* argv[]) {
    // Nếu người dùng không truyền tên file vào
    if (argc != 2) {
        std::cerr << "Cach su dung: fdlang <file_path>\n";
        return 64; // Mã lỗi chuẩn (EX_USAGE)
    }

    std::string filepath = argv[1];
    
    // Đọc file và LƯU TRỮ VÀO MỘT BIẾN TỒN TẠI SUỐT HÀM MAIN
    // Nếu biến sourceCode này bị hủy, toàn bộ Lexer sẽ sụp đổ!
    std::string sourceCode = readFile(filepath);

    std::cout << "[FDLANG COMPILER]\n";
    std::cout << "Compiling: " << filepath << "\n\n";

    std::cout << std::left 
              << std::setw(15) << "TOKEN TYPE" 
              << std::setw(10) << "TEXT" 
              << "OFFSET\n";
    std::cout << "-----------------------------------\n";

    // Khởi tạo Lexer, tự động ép kiểu std::string sang std::string_view
    Lexer lexer(sourceCode);
    Token token;

    do {
        token = lexer.nextToken();
        std::cout << std::left 
                  << std::setw(15) << tokenTypeToString(token.type)
                  << std::setw(10) << token.text 
                  << token.byteOffset << "\n";
    } while (token.type != TokenType::END_OF_FILE && token.type != TokenType::ERROR);

    return 0;
}