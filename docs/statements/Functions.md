# Functions

Using the `fn` keyword, a function can be defined. Functions, like all other declarations, are inherently public,
meaning they can be accessed when the file they are declared in is imported. Functions are always declared globally,
meaning that they can be accessed in any compilation unit that imports the file they are declared int, regardless of
whether they are declared before or after the access. Functions that have a return type need to return a value.

## Example

```
fn my_function(u32 arg1) i32 {
    return as!(arg1, i32);
}

fn no_return() {
}
```

## Member functions

Functions can be declared as members of types, and then accessed using the member access syntax. This works on both 
builtin and user types, regardless of in which file they are declared.

```
fn u32.min(this, u32 other) u32 {
    if this > other {
        return other;
    }
    return this;
}

# ... in a function
u32 an_int = 4;
an_int.min(2); # 2
an_int.min(6); # 4
```

## Syntax

```
<function> ::= "fn" [<type> "."] <name> "(" <argument-list> ")" [<type>] <scope>
<argument-list> ::= <type> <name> ["," <argument-list>]
```
