extern fn printf(format: str, ...) -> int_32;

fn test_if(x: int_32) -> int_32 {
    if x > 10 {
        return 1;
    } else if x < 0 {
        return -1;
    } else {
        return 0;
    }
}

export fn main() -> int_32 {
    printf("test_if(15) = %d\n", test_if(15));
    printf("test_if(-5) = %d\n", test_if(-5));
    printf("test_if(5) = %d\n", test_if(5));
    return 0;
}
