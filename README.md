# tarik

tarik is a minimal, compiled language, designed for low-level programming.

## Hello World

```rust
fn puts(u8 *s) i32;

fn main() i32 {
    puts("Hello, World!");

    return 0;
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

To build tarik you will need a C++ compiler, LLVM 18.1 and CMake >= 3.17.

Then the build procedure is as usual with cmake:

```shell
mkdir build && cd build
cmake ..
cmake --build .
```
