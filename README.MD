# freedomLanguage (fdlang)

![Status](https://img.shields.io/badge/Status-MVP_Completed-success)
![Language](https://img.shields.io/badge/Language-C++17-blue)
![Backend](https://img.shields.io/badge/Backend-LLVM-red)

**freedomLanguage** (`.fl`) là một ngôn ngữ lập trình được thiết kế với mục tiêu xây dựng một công cụ mạnh mẽ, hiệu suất cao, có thể dùng để phát triển **Game Engine** và **Game**. Ngôn ngữ có cú pháp hiện đại mang cảm hứng từ Rust, cùng với khả năng kiểm soát bộ nhớ chặt chẽ (Ownership / Borrowing) trong tương lai.

Dự án hiện đã hoàn thành giai đoạn **MVP (Minimum Viable Product)**, với khả năng biên dịch từ mã nguồn `.fl` sang mã máy (`.exe`) native thông qua LLVM Backend.

## Tính năng nổi bật

- Cú pháp hiện đại, trực quan, thân thiện.
- Biên dịch ra mã máy (Native Machine Code) hoàn toàn độc lập.
- Frontend tự viết tay (Hand-written Recursive Descent Parser).
- MiddleEnd mạnh mẽ với kiến trúc FLIR độc quyền.
- Backend tích hợp **LLVM 18+**.
- Liên kết (Linking) tự động qua Clang driver.

## Kiến trúc Pipeline

1. **FrontEnd**: Lexer, Parser -> AST
2. **MiddleEnd**: Resolver, Type Checker, FLIR Generator -> FLIR (Freedom Language Intermediate Representation)
3. **BackEnd**: LLVM IR Generator -> LLVM IR -> Tối ưu hóa -> Object File -> Liên kết ra file thực thi (.exe)

##  Tài liệu

Vui lòng tham khảo thư mục `docs/` để biết thêm chi tiết:
- `architecture.md`: Chi tiết về thiết kế trình biên dịch.
- `grammar.md`: Ngữ pháp EBNF của fdlang.
- `RoadMap.md`: Lộ trình phát triển dự án.

##  Cài đặt & Build (Dành cho Developer)

**Yêu cầu hệ thống:**
- CMake 3.20+
- Trình biên dịch C++17 (MSVC / GCC / Clang)
- LLVM 18.0.0+ (Phải build sẵn trên máy)

**Các bước build:**
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

##  Trạng thái
Tham khảo `Status.md` để xem trạng thái phát triển chi tiết và công việc tiếp theo.
