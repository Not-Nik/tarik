# If

`if` statements conditionally execute statements within their scope. Conditions can either be booleans or any type that
can be compared to zero.

## Example

```
# ... in a function
if a_bool {
    # executes if a_bool is set to true
}

if some_int {
    # executes if some_int is set to a value not equal to zero
}

if some_value == 4 {
    # executes if the expression is true
}
```

## Else

The scope of an `else` statement is executed if the preceding `if` statement isn't.

```
# ... in a function
if false {
    # this obviously doesn't execute
} else {
    # but this does
}
```

## Syntax

```
<if> ::= "if" <expression> <scope> [<else>]
<else> ::= "else" <scope>
```
