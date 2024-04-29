# Types

## Builtin types

Tarik features twelve builtin in types

| Type name | Description                                     |
|-----------|-------------------------------------------------|
| void      | Empty type                                      |
| bool      | Unsigned 1-bit integer                          |
| i8        | Signed 8-bit integer                            |
| i16       | Signed 16-bit integer                           |
| i32       | Signed 32-bit integer                           |
| i64       | Signed 64-bit integer                           |
| u8        | Unsigned 8-bit integer                          |
| u16       | Unsigned 16-bit integer                         |
| u32       | Unsigned 32-bit integer                         |
| u64       | Unsigned 64-bit integer                         |
| f32       | IEEE 754 single precision floating point number |
| f64       | IEEE 754 double precision floating point number |

## Pointers

Pointers hold addresses of other types and can be declared as usual in other programming languages:

```
<pointer> ::= <type> "*"
```

Pointers do not hold any information about the lifetime of the value they are pointing to. Like in LLVM, accessing a
pointer that points to an invalid piece of memory results in undefined behaviour.

## User types

Aggregate user types can be created using [structs](Structs.md).

## Syntax

```
<type> ::= <builtin-type> | <path> | <pointer>
<pointer> ::= <type> "*"
<builtin-type> ::= "void" | "bool" | "i"("8"|"16"|"32"|"64") | "u"("8"|"16"|"32"|"64") | "f32" | "f64"
```
