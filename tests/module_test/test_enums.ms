enum Option<T> {
    Some(T),
    None
}

fn test_match(opt: Option@<int_32>) -> int_32 {
    return match opt {
        Option::Some(x) -> x,
        Option::None -> -1
    };
}

fn main() -> int_32 {
    dec opt1 = Option@<int_32>::Some(42);
    return test_match(opt1);
}
