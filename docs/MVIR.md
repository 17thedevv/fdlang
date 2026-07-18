// ==========================================
// 1. CÁC THÀNH PHẦN CƠ BẢN
// ==========================================
letter      ::= "a" | ... | "z" | "A" | ... | "Z" | "_"
digit       ::= "0" | ... | "9"

global_id   ::= "@" letter (letter | digit)*
local_id    ::= "%" (letter | digit)+
label_id    ::= letter (letter | digit)*
type_id     ::= "%" letter (letter | digit)*  // [MỚI] Tên của struct do user định nghĩa (VD: %Player)

number      ::= digit+
boolean     ::= "true" | "false"
string_lit  ::= '"' (bất_kỳ_ký_tự_nào_trừ_nháy_kép)* '"' // [MỚI] Dành cho biến toàn cục

operand     ::= local_id | number | boolean | global_id

// ==========================================
// 2. HỆ THỐNG KIỂU DỮ LIỆU (TYPE SYSTEM)
// ==========================================
// [MỚI] Thêm array_type, type_id, reference_type vào danh sách kiểu
type           ::= primitive_type | pointer_type | reference_type | array_type | type_id
primitive_type ::= "i8" | "i32" | "i64" | "bool" | "void" // Thêm i8 để làm string (mảng i8)
pointer_type   ::= "*" type
reference_type ::= "&" ("shared" | "mut") type            // Sinh ra từ lệnh borrow
array_type     ::= "[" number "x" type "]"                // VD: [10 x i32]

// ==========================================
// 3. CẤU TRÚC CHƯƠNG TRÌNH (STRUCTURE)
// ==========================================
// [MỚI] Một module giờ chứa các khai báo type, global, extern functions, rồi mới tới function
module      ::= top_level_decl* extern_func_decl* func_decl* EOF

top_level_decl ::= type_decl | global_decl

// [MỚI] Định nghĩa cấu trúc Struct
type_decl   ::= "type" type_id "=" "struct" "{" (type ("," type)*)? "}"

// [MỚI] Định nghĩa chuỗi (String) hoặc hằng số toàn cục
global_decl ::= global_id "=" "global" type string_lit

// [MỚI] Khai báo hàm ngoại lai (FFI)
extern_func_decl ::= "extern" "func" global_id "(" params? ")" "->" type

func_decl   ::= "func" global_id "(" params? ")" "->" type "{" basic_block+ "}"
params      ::= type local_id ("," type local_id)*
basic_block ::= label_id ":" instruction* terminator

// ==========================================
// 4. TẬP LỆNH (INSTRUCTIONS)
// ==========================================
// [MỚI] Thêm cast_inst vào tập lệnh
instruction ::= mem_inst | alu_inst | call_inst | cast_inst

// NHÓM BỘ NHỚ (Memory)
mem_inst    ::= local_id "=" "alloca" type
              | local_id "=" "load" operand
              | "store" operand "," operand
              // Tính toán địa chỉ để truy cập mảng hoặc struct field
              | local_id "=" "get_ptr" operand ("," operand)+ 
              // Lệnh mượn. Đầu vào là một pointer/local, đầu ra là một reference
              | local_id "=" "borrow" ("shared" | "mut") operand 

// NHÓM TOÁN HỌC & SO SÁNH (ALU)
alu_inst    ::= local_id "=" alu_op operand "," operand
alu_op      ::= "add" | "sub" | "mul" | "div" 
              | "eq" | "ne" | "lt" | "le" | "gt" | "ge"

// NHÓM GỌI HÀM (Call)
// [MỚI] Cho phép gọi qua biến operand (Indirect Calls) hỗ trợ Function Pointers và Trait Objects
call_inst   ::= (local_id "=")? "call" operand "(" args? ")"
args        ::= operand ("," operand)*

// [MỚI] NHÓM ÉP KIỂU (Casting - phục vụ từ khóa `as`)
cast_inst   ::= local_id "=" "cast" operand "to" type

// ==========================================
// 5. LỆNH ĐIỀU KHIỂN (TERMINATORS)
// ==========================================
terminator  ::= "jump" label_id
              | "branch" operand "," label_id "," label_id
              // [MỚI] Rẽ nhánh nhiều hướng O(1) phục vụ Match expression
              | "switch" operand "{" (number ":" label_id)* "default" ":" label_id "}"
              | "ret" operand?