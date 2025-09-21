# Imports

Using the `import` keyword, one source file can import the contents of another source file. Importing is transparent,
meaning that both the importing and the imported files can see each other's declarations and all declarations in any
imported files.

If there exists a file with the given name and the `.tk` extension it is imported as `name` into the current
directory's path. This means that in the root directory (where the main tarik file is), the file is imported
into the root path (`::name`). In any subdirectory, the last file name is removed from the path and the file is
imported into the resulting path, i.e. if a file `file2.tk` is imported from `dir/file1.tk` the path would be
`::dir::file1`, then `file1` would be removed from the path and the file would be imported into `::dir::file2`.

If no file with the given name and the `.tk` extension exists, but there exists a directory with the given name,
`name/name.tk` is imported as `name::name` into the current directory's path, similarly to how files would be imported.

## Examples

```
import file;
import directory;

# In a function

file::function();
directory::another::type variable;
```

## Syntax

```
<import> ::= "import " <name> ";"
```
