struct Box<T> {
    value: T;
}

impl<T> Box<T> {
    fn get(self: Box<T>) -> T {
        return self.value;
    }
}

export fn main() -> int_32 {
    dec b: Box<int_32> = Box@<int_32>{ value: 42 };
    return b.get();
}
