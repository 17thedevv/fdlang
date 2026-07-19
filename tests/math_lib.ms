export struct MathUtil {
    x: int_32;
}

export impl MathUtil {
    fn compute(self: MathUtil, val: int_32) -> int_32 {
        return self.x * val;
    }
}

export fn create_math_util(v: int_32) -> MathUtil {
    return MathUtil { x: v };
}
