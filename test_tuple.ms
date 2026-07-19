extern fn printf(format: str, ...) -> int_32;

export fn main() -> int_32 {
    dec t = (100, 200, 300);
    printf("t.0 = %d\n", t.0);
    printf("t.1 = %d\n", t.1);
    printf("t.2 = %d\n", t.2);
    return 0;
}
