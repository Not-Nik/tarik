# Structures

Structures can be declared using the `struct` keyword. A structure can contain any number of member variables. 
[Functions](Functions.md#member-functions) can also be declared as members of a structure.

## Example

```
struct Vector2f {
    f32 x;
    f32 y;
}
```

## Syntax

```
<structure> ::= "struct" <name> "{" <member-list> "}"
<member-list> ::= [<member> <member-list>]
<member> ::= <type> <name> ";"
```
