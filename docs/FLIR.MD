// ==========================================
// 1. CÁC THÀNH PHẦN CƠ BẢN
// ==========================================
letter      ::= "a" | ... | "z" | "A" | ... | "Z" | "_"
digit       ::= "0" | ... | "9"

global_id   ::= "@" letter (letter | digit)*
local_id    ::= "%" (letter | digit)+
label_id    ::= letter (letter | digit)*

number      ::= digit+
boolean     ::= "true" | "false"

// Toán hạng có thể là một biến, hằng số hoặc tên hàm
operand     ::= local_id | number | boolean | global_id

// ==========================================
// 2. HỆ THỐNG KIỂU DỮ LIỆU (TYPE SYSTEM)
// ==========================================
type           ::= primitive_type | pointer_type
primitive_type ::= "i32" | "i64" | "bool" | "void"
pointer_type   ::= "*" type

// ==========================================
// 3. CẤU TRÚC CHƯƠNG TRÌNH (STRUCTURE)
// ==========================================
module      ::= func_decl* EOF

func_decl   ::= "func" global_id "(" params? ")" "->" type "{" basic_block+ "}"

params      ::= type local_id ("," type local_id)*

basic_block ::= label_id ":" instruction* terminator

// ==========================================
// 4. TẬP LỆNH (INSTRUCTIONS)
// ==========================================
instruction ::= mem_inst | alu_inst | call_inst

// NHÓM BỘ NHỚ (Memory)
mem_inst    ::= local_id "=" "alloca" type
              | local_id "=" "load" operand
              | "store" operand "," operand

// NHÓM TOÁN HỌC & SO SÁNH (ALU)
alu_inst    ::= local_id "=" alu_op operand "," operand
alu_op      ::= "add" | "sub" | "mul" | "div" 
              | "eq" | "ne" | "lt" | "le" | "gt" | "ge"

// NHÓM GỌI HÀM (Call)
call_inst   ::= (local_id "=")? "call" global_id "(" args? ")"
args        ::= operand ("," operand)*

// ==========================================
// 5. LỆNH ĐIỀU KHIỂN (TERMINATORS)
// ==========================================
terminator  ::= "jump" label_id
              | "branch" operand "," label_id "," label_id
              | "ret" operand?