# Code conventions

## Use of auto

The `auto` keyword shall only be used, if the deduced type is obvious or specifying the type is unreasonably complicated.
Specifically that means it may only be used in range-based for loop, or in a variable declaration when the initializer
is an allocation using `new`, an initialisation or is cast. Additionally, if the deduced type would be a pointer, this 
should be specified by using `auto *`. The only exception to this rule is when destructuring a type;

### Example

```c++
// Literals
auto a = 4; // BAD
auto b = "string"; // BAD
auto c = 4.2; // BAD

// User types
auto d = Class(); // Good
auto e = new Class(); // BAD
auto *f = new Class(); // Good
auto g = expression; // BAD
auto *h = expression; // BAD
auto [i, j] = destructurable; // Good

// Casts
auto k = (Class) expression; // Good
auto l = expression.to_class(); // BAD

// range-based for loops
for (auto elem : value_range) {} // Good
for (auto elem : ptr_range) {} // Bad
for (auto *elem : ptr_range) {} // Good
for (auto [key, val] : destructurable_range) {} // Good
```
