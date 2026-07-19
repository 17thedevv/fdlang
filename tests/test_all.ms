extern fn printf(fmt: *int_8, ...) -> int_32;

mod math_lib;

// 1. Generics
struct Box<T> {
    value: T;
}

impl<T> Box<T> {
    fn get(self: Box<T>) -> T {
        return self.value;
    }
}

// 2. Async Coroutines
async fn do_async_work() -> int_32 {
    return 42;
}

fn main() -> int_32 {
    // 3. Structs and Methods
    dec m = math_lib::create_math_util(10);
    dec res1 = m.compute(5); // 50
    
    // 4. Pattern Matching
    dec res2 = match res1 {
        50 -> 1,
        _ -> 0
    };
    
    // 5. Lambdas and Closures
    dec capture = 100;
    dec func = |v: int_32| -> int_32 {
        return v + capture;
    };
    dec res3 = func(20); // 120
    
    // 6. Unsafe and Raw Pointers
    dec raw_val = 0;
    unsafe {
        {
            dec ptr = &rw raw_val as *rw int_32;
            *ptr = 999;
        }
    }
    
    // 7. Future instantiation (async call)
    dec fut = do_async_work();
    
    // 8. Generics execution
    dec b: Box<int_32> = Box@<int_32>{ value: res3 };
    dec gen = b.get(); // 120
    
    // 9. Compile-time MVIROptimizer test
    dec optimized = 50 * 2 + 10; // 110
    
    unsafe {
        printf("Method call: %d\n" as *int_8, res1);
        printf("Pattern Matching: %d\n" as *int_8, res2);
        printf("Lambda Closure: %d\n" as *int_8, res3);
        printf("Raw Pointer Mod: %d\n" as *int_8, raw_val);
        printf("Generics: %d\n" as *int_8, gen);
        printf("MVIROptimizer: %d\n" as *int_8, optimized);
    }
    
    return 0;
}
