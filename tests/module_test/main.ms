mod math;
use math as m;
mod utils;

extern fn printf(fmt: str, ...) -> int_32;

fn main() -> int_32 {
    dec sum: int_32 = m::add(10, 20);
    printf("10 + 20 = %d\n", sum);

    utils::print_message();

    return 0;
}
