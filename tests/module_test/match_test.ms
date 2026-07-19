fn main() -> int_32 {
    dec x: int_32 = 2;
    dec result: int_32 = match x {
        1 -> 10,
        2 -> 20,
        _ -> 30
    };
    return result;
}
