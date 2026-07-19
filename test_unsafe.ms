extern fn printf(format: str, ...) -> int_32;

unsafe fn dangerous_op() {
    printf("dangerous_op called\n");
}

export fn main() -> int_32 {
    // 1. Error: call unsafe/extern fn in safe code
    dangerous_op();
    
    return 0;
}
