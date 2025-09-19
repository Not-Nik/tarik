# tarik

tarik is a minimal, compiled language, designed for low-level programming.

## Hello World

```rust
fn main() i32 {
    i32 integer = 1 + 2 * 3 % 4 / 3;
    i32 another = integer.add(42);
    
    if integer > another {
        return -1;
    }
    
    i32 counter = 0;
    
    while another > integer {
        another = another - 2;
        counter = counter + 1;
    }
    
    f32 float = counter.as!(f32);
    
    float = float / 0.7;
    
    if float.as!(u32) == 3 {
        return 0;
    } else {
        return 1;
    }
}

fn i32.add(*this, i32 other) {
    return *this + other;
}
```

```shell
tarik hello.tk
gcc hello.o -o hello
./hello
```

## Inspiration

tarik is inspired by

- C
- C++
- [Rust](https://github.com/rust-lang/rust)
- [oak](https://github.com/adam-mcdaniel/oakc)

## Building

To build tarik you will need a C++ compiler, LLVM 21.1 and CMake >= 3.17.

Then the build procedure is as usual with cmake:

```shell
mkdir build && cd build
cmake ..
cmake --build .
```
