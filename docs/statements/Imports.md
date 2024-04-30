# Imports

Using the `import` keyword, one source file can import the contents of another source file. Importing is transparent,
meaning that both the importing and the imported files can see each other's declarations and all declarations in any
imported files. The file extension `.tk` is automatically appended to the import path; `::`s are replaced with the
host systems path delimiters. Imported declarations can be accessed using the import path as a prefix.

## Examples

```
import file;
import directory::file;

# In a function

file::function();
directory::file::type variable;
```

## Syntax

```
<path> ::= ["::"] <local-path>
<local-path> ::= <name> ["::" <local-path>]

<import> ::= "import " <local-path> ";"
```
