struct Point<T> {
    x: T;
    y: T;
}

fn add_points<T>(p1: Point<T>, p2: Point<T>) -> Point<T> {
    return Point@<T> { x: p1.x + p2.x, y: p1.y + p2.y };
}

fn main() {
    dec p1 = Point@<int_32> { x: 10, y: 20 };
    dec p2 = Point@<int_32> { x: 5, y: 15 };
    dec p3 = add_points@<int_32>(p1, p2);
}
