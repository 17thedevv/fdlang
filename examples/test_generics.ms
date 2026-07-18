struct Vec<T> {
    data: *T;
}

struct Map<K, V> {
    keys: *K;
    values: *V;
}

fn identity<T>(x: T) -> T {
    return x;
}

fn main() {
    dec my_vec: Vec@<int_32> = Vec@<int_32> { data: null };
    dec nested: Vec@<Vec@<int_32>> = Vec@<Vec@<int_32>> { data: null };
    dec val = identity@<int_32>(42);
}
