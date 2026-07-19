# Kế Hoạch Các Công Việc Tiếp Theo (fdlang Compiler)

Dựa trên tình trạng hiện tại của mã nguồn và các định hướng thiết kế cốt lõi của ngôn ngữ (`VISION.MD`), trình biên dịch `fdlang` đã sở hữu một nền tảng khá vững chắc (từ Lexer, Parser, Type Inference, MVIR cho tới LLVM IR). Để biến `fdlang` thành một ngôn ngữ hoàn thiện và thực tiễn, dưới đây là **danh sách chi tiết các công việc cần phải làm tiếp theo** được chia theo từng giai đoạn (Phase).

---

## Phase 10: Traits & Generics Nâng Cao
Hiện tại bộ máy Monomorphization (chuyên biệt hóa generic) mới chỉ hỗ trợ cơ bản cho các hàm (Function) và kiểu cấu trúc (Struct/Enum). Chúng ta cần xử lý nốt các phần phức tạp nhất của Generics và Traits.

### 10.1. Monomorphization cho `ImplDeclNode` (Generic Impls)
- **Vấn đề**: Khi khai báo `impl<T> Trait for MyStruct<T>`, trình biên dịch cần instantiate toàn bộ khối lệnh `impl` này khi `MyStruct<T>` được cụ thể hoá.
- **Giải pháp**: Bổ sung logic vào `MonomorphizationEngine` để tìm và clone các `ImplDeclNode` tương ứng, thay thế kiểu (Substitution) và đăng ký các method cụ thể (concrete methods) vào bảng symbol.

### 10.2. Dynamic Dispatch (Trait Objects)
- **Vấn đề**: Hiện AST đã parse từ khoá `dyn Trait` (`TraitObjectTypeNode`), nhưng hệ thống Type Checker, MVIR và LLVM chưa biết cách xử lý.
- **Cấu trúc Fat Pointer**: Trong LLVM IR, `dyn Trait` phải được biểu diễn bằng một struct gồm 2 con trỏ: `{ void* data, vtable* vptr }`.
- **VTable Generation**: Khi ép kiểu (cast) từ một struct cụ thể sang `dyn Trait`, compiler cần tự động sinh VTable cho struct đó và truyền vào Fat Pointer.
- **MVIR & LLVM IR**: Bổ sung `VirtualCallInst` vào MVIR để tra cứu hàm từ VTable thay vì gọi trực tiếp (Static Call).

---

## Phase 11: Strings & Thư Viện Tiêu Chuẩn Nền Tảng (Core/Std)
Một ngôn ngữ cấp thấp cần có cách tiếp cận bộ nhớ và chuỗi ký tự chuẩn mực. Hiện `fdlang` mới chỉ nhận diện kiểu chuỗi nguyên thuỷ (Primitive `Str`) dưới dạng con trỏ tĩnh.

### 11.1. Chuỗi (Strings) & Bộ Nhớ Động
- **Kiểu dữ liệu `String`**: Implement chuẩn chuỗi (tương tự `String` của Rust/C++) dạng cấu trúc `{ ptr: *u8, len: uint_64, capacity: uint_64 }`.
- **String Literals**: LLVM IR đang xử lý chuỗi bằng Global String Literals. Cần hỗ trợ tự động wrap mảng byte này vào kiểu cấu trúc String.
- **Memory Allocation**: Thiết lập các hàm built-in cơ bản (alloc, free, realloc) liên kết trực tiếp với libc để hỗ trợ cấp phát bộ nhớ động trên Heap.

### 11.2. Standard Library Nền Tảng
- Cấu trúc module hệ thống mặc định (ví dụ: `core::io::print`, `core::mem::sizeof`).
- Định nghĩa các hàm `extern` liên kết với hệ điều hành (ví dụ: `extern fn printf(fmt: *i8, ...) -> i32;` để người dùng không phải tự khai báo).

---

## Phase 12: Module System
Mục tiêu cốt lõi là hỗ trợ biên dịch nhiều file và tổ chức dự án thay vì chỉ dịch một file duy nhất.

### 12.1. Quản Lý Module Hệ Thống
- Xây dựng hệ thống Import/Export module từ mã nguồn trực tiếp: `import std::io;`, phân giải đường dẫn file tới module và nạp Symbol Table tương ứng.

### 12.2. Hỗ trợ Project Build
- Hỗ trợ xây dựng theo dự án (Project build) quét qua toàn bộ các thư mục và mã nguồn thay vì chỉ biên dịch file đơn lẻ.

---

## Phase 13: Xử Lý Lỗi (Error Handling)
Chưa có cơ chế văng lỗi an toàn ở Runtime (ngoài crash).

### 13.1. Các kiểu dữ liệu cốt lõi
- Xây dựng `Enum` chuẩn: `Result<T, E>` và `Option<T>` để xử lý trạng thái.
### 13.2. Cú pháp `?` (Try Operator)
- Cập nhật Lexer, Parser và MVIR để hỗ trợ toán tử `?`. Trả về sớm (Early return) nếu giá trị là dạng lỗi `E`.

---

## Phase 14: Tooling & IDE Support
Trong `VISION.MD`, Tooling là "Công dân hạng nhất" (First-class citizen).

### 14.1. Language Server Protocol (LSP)
- Tách các module Frontend (Lexer, Parser, TypeChecker) ra để có thể chạy dưới dạng một Server ngầm.
- Cung cấp: Báo lỗi trực tiếp trên editor, gợi ý mã (Autocomplete), Hover xem Type, và Go-to definition.

### 14.2. CLI Tích Hợp & Formatter
- Xây dựng các lệnh CLI chuẩn chỉnh: `fdlang build`, `fdlang run`, `fdlang test`.
- Tích hợp công cụ định dạng mã (Code Formatter) thống nhất style code toàn dự án.

---

## User Review Required

Bạn muốn ưu tiên thực hiện nhóm công việc nào tiếp theo?
1. Tiến hành ngay **Phase 10 (Generics Nâng Cao & Trait Objects)** để hoàn chỉnh hệ thống System Typing?
2. Hay chuyển sang **Phase 11 (Chuỗi, Cấp Phát Bộ Nhớ Động & Thư Viện Tiêu Chuẩn)** để việc viết code thực tế dễ dàng hơn?
3. Hay tập trung vào **Module System (`.flo`)** để thiết lập cấu trúc biên dịch nhiều file?
