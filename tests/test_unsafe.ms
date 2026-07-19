extern fn printf(fmt: *int_8, ...) -> int_32;

fn main() -> int_32 {
    dec x = 42;
    
    unsafe {
        {
            dec ptr = &rw x as *rw int_32;
            *ptr = 100;
        }
        printf("x is now %d\n" as *int_8, x);
    }
    return x;
}
