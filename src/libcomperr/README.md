# libcomperr
A simple C library to create GCC-like compiler warnings and errors.

## Simple usage example
```c
#include "comperr.h"

int main()
{
    comperr(false, "Test warning", true, "", 0, 0);
    comperr(false, "Test error 1", false, "", 0, 0);
    comperr(false, "Test error 2", false, "", 0, 0);
    endfile();
}
```

## Internal stuff...
...you might want to know.
* Streams are flushed after each message
* Tabs are replaced with single whitespaces
* ANSI color codes are used so your terminal has to support that

There are three compile flags:
### STDERR_WARN
If defined warnings will go to `stderr` instead of `stdout` just like errors do.
### ONE_ERROR
If defined the library will `exit(1)` the program after one error.
### MANU_EXIT
If defined the library will not `exit(1)` if there has been an error when `endfile()` is called.