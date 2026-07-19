struct Point { x: int_32; y: int_32; }

fn main() -> int_32 {
    dec p: Point = Point { x: 10, y: 20 };
    return p.x + p.y;
}
