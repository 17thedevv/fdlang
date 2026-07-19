mod string;

use string::get_greeting as hi;

extern fn printf(fmt: str, ...) -> int_32;

export fn print_message() {
    printf("%s\n", hi());
}
