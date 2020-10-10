// comperr (c) Nikolas Wipper 2020

#ifndef COMPERR_COMPERR_H
#define COMPERR_COMPERR_H

#include <stdarg.h>

#ifndef __cplusplus

#include <stdbool.h>

#else
extern "C" {
#endif

bool comperr(
        bool condition, const char * message, bool warning,
        const char * fileName, int lineNumber, int row, ...
);

bool vcomperr(
        bool condition, const char * message, bool warning,
        const char * fileName, int lineNumber, int row, va_list va
);

void endfile();

#ifdef __cplusplus
}
#endif

#endif //COMPERR_COMPERR_H
