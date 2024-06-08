# While

`while` statements conditionally keep executing statements within their scope. Conditions can either be booleans or any 
type that can be compared to zero.

## Example

```
# ... in a function
while a_bool {
    # executes while a_bool is set to true
}

while some_int {
    # executes while some_int is set to a value not equal to zero
}

while some_value == 4 {
    # executes while the expression is true
}
```

## Syntax

```
<while> ::= "while" <expression> <scope>
```
