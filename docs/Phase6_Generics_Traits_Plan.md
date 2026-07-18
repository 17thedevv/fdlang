# Kế hoạch Triển khai Phase 6: Generics & Traits

Phase 6 là trái tim của sự trừu tượng trong FDLang, giúp ngôn ngữ hỗ trợ đa hình tĩnh (Static Polymorphism thông qua Monomorphization) và đa hình động (Dynamic Dispatch thông qua VTable và `dyn Trait`). Vì chúng ta đã có Module System vững chắc ở Phase 5, sự xung đột tên (Name Collision) khi nhân bản code sẽ không còn là vấn đề!

Dưới đây là kế hoạch chi tiết bạn có thể sử dụng cho phiên chat tiếp theo:

## 1. Mở rộng AST (Cây cú pháp trừu tượng)
Cần bổ sung các cấu trúc dữ liệu để lưu trữ tham số kiểu:
- **`GenericParamNode`**: Đại diện cho tham số kiểu khi khai báo (VD: `T`, `U`).
- **`GenericArgNode`**: Đại diện cho kiểu cụ thể khi gọi hàm/struct (VD: `<int_32>`).
- Cập nhật **`StructDeclNode`**, **`FunctionDeclNode`**, **`EnumDeclNode`** để chứa danh sách `genericParams`.
- Cập nhật **`NamedTypeNode`** để chứa danh sách `genericArgs` (VD: Để biểu diễn `Vec<int_32>`).
- Bổ sung AST Nodes cho Traits: **`TraitDeclNode`** (khai báo trait), **`ImplDeclNode`** (thực thi trait cho struct).

## 2. Nâng cấp Parser
- **Hỗ trợ cặp ngoặc `< >`**: Cập nhật `Lexer` và `Parser` để xử lý `<T>` sau các từ khóa `struct`, `fn`, `enum`. Lưu ý xử lý lỗi cú pháp dịch chuyển bit `>>` trùng với đóng generic.
- **Cú pháp `impl` và `trait`**:
  ```mellis
  trait Printable {
      fn print(self);
  }

  impl Printable for MyStruct {
      fn print(self) { /* ... */ }
  }
  ```

## 3. Kiến trúc Monomorphization (Quan trọng nhất)
Vì FDLang sử dụng LLVM IR (không hỗ trợ generic ở runtime), chúng ta **phải** tạo ra một *Monomorphization Engine*:
- **Bản chất**: Giống hệt C++ Templates hay Rust. Mỗi khi phát hiện lệnh gọi `foo<int_32>()`, trình biên dịch sẽ "copy-paste" cây AST của hàm `foo<T>`, thay thế `T` bằng `int_32`, và tạo thành một hàm nội bộ tên là `foo_int_32`.
- **Name Mangling**: Cần thuật toán đổi tên hàm/struct để LLVM không bị nhầm lẫn (VD: `Vec<int_32>` -> `Vec_int_32`).
- **Lazy Instantiation**: Chỉ nhân bản những hàm/struct có thực sự được gọi (sử dụng) trong code.

## 4. Nâng cấp Resolver và TypeChecker
- **Resolver**: Khi gặp một kiểu generic (VD: `T`), Resolver phải tra cứu xem `T` có nằm trong danh sách `genericParams` của Scope hiện tại hay không. Nếu có, nó là một Type Parameter, không phải là một kiểu thông thường.
- **TypeChecker (Inference)**: Cần nâng cấp thuật toán nội suy (Unification Engine) để tự động nhận diện kiểu khi người dùng không truyền `<...>`:
  ```mellis
  fn identity<T>(x: T) -> T { return x; }
  dec val = identity(42); // TypeChecker phải tự nội suy T = int_32
  ```

## 5. Traits và VTable (Tùy chọn cho nửa sau của Phase 6)
- **Static Dispatch**: Khi gọi method của trait thông qua kiểu cụ thể `MyStruct`, biên dịch trực tiếp thành lời gọi hàm bình thường.
- **Dynamic Dispatch (`dyn Trait`)**: Xây dựng cấu trúc con trỏ béo (Fat Pointer) mang 2 giá trị:
  1. Con trỏ dữ liệu (Data Pointer).
  2. Con trỏ bảng hàm ảo (VTable Pointer).
- Xây dựng hệ thống tự động sinh VTable cho mỗi cặp `Impl Trait for Type` ở tầng LLVM IR.

---

**Hướng dẫn khi bắt đầu Chat mới**:
Hãy copy file này, đính kèm vào chat mới và ra lệnh: 
*"Đây là kế hoạch Phase 6 của FDLang. Hãy đọc kỹ, phân tích và bắt đầu bằng việc mở rộng AST và Parser cho cú pháp Generics `<T>` trước nhé!"*
