fn main() -> int_32 {
    dec multiplier = 10;
    dec base = 2;
    dec my_closure = |x: int_32| -> int_32 {
        return (x + base) * multiplier;
    };
    return my_closure(4);
}
