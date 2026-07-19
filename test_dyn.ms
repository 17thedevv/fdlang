extern fn printf(format: str, ...) -> int_32;

trait Animal {
    fn speak(self: &Self);
}

struct Dog {}

impl Animal for Dog {
    fn speak(self: &Self) {
        printf("Woof!\n");
    }
}

fn make_sound(a: &dyn Animal) {
    a.speak();
}

export fn main() -> int_32 {
    dec d = Dog{};
    make_sound(&d as &dyn Animal);
    return 0;
}
