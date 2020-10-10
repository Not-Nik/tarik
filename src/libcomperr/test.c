// comperr (c) Nikolas Wipper 2020

#include "comperr.h"

int main()
{
    comperr(false, "Test warning", true, "Makefile", 0, 5);
    comperr(false, "Test error 1", false, "Makefile", 1, 3);
    comperr(false, "Test error 2", false, "Makefile", 3, 10);
    comperr(false, "Format test %i. %s", false, "Makefile", 3, 10, 1, "Let's hope it works");
    endfile();
}
