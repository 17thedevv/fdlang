fn add@<T>(a: T, b: T) -> T {
    return a + b;
}

fn main() -> int_32 {
    dec x: int_32 = add@<int_32>(10, 20);
    return x;
}
