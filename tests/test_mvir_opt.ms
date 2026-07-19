extern fn printf(fmt: *int_8, ...) -> int_32;

fn main() -> int_32 {
    dec a = 100 + 50 * 2;
    
    if (true) {
        unsafe { printf("True branch taken\n" as *int_8); }
    } else {
        unsafe { printf("False branch taken\n" as *int_8); }
    }
    
    dec b = 999 + 1;
    return a;
}
