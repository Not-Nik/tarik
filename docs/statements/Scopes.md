# Scopes

In any function a scope can be declared using curly brackets. Scopes explicitly limit the lifetime of variables.
Other than that they have no effect. They can contain any number of statements, except for functions and structures.

## Example

```
# ... in a function
{
    i32 some_var = 4;
    some_func(some_var);
}
```

## Syntax

```
<scope> ::= "{" <statement-list> "}"
<statement-list> ::= [<statement> <statement-list>]
```
