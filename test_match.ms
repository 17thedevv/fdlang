enum Option {
    Some(int_32),
    None
}

fn main() -> int_32 {
    dec val: Option = Option::Some(42);
    
    dec result: int_32 = match val {
        Option::Some(x) -> x * 2,
        Option::None -> 0
    };
    
    return result;
}
