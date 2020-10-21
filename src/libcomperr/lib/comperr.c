// comperr (c) Nikolas Wipper 2020

#include "comperr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_WHITE   "\x1b[97m"

static short errorCount = 0;
static short warnCount = 0;

bool comperr(
        bool condition, const char * message, bool warning,
        const char * fileName, int lineNumber, int row, ...
) {
    va_list args;
    va_start (args, row);
    bool r = vcomperr(condition, message, warning, fileName, lineNumber, row, args);
    va_end(args);
    return r;
}

bool vcomperr(
        bool condition, const char * message, bool warning,
        const char * fileName, int lineNumber, int row, va_list va
) {
    if (!condition) {
        FILE * outputStream = stderr;
#ifndef STDERR_WARN
        if (warning)
            outputStream = stdout;
#endif

        FILE * fp;
        fp = fopen(fileName, "r");
        const char empty[] = "empty";

        if (fp == NULL) {
            fileName = empty;
        }

        char buffer[strlen(message) * 4];
        vsnprintf(buffer, strlen(message) * 4, message, va);

        fprintf(outputStream, "%s%s:%i:%i: %s%s:%s %s\n",
                ANSI_COLOR_WHITE,
                fileName,
                lineNumber + 1,
                row + 1,
                (warning ? ANSI_COLOR_MAGENTA : ANSI_COLOR_RED),
                (warning ? "warning" : "error"),
                ANSI_COLOR_WHITE,
                buffer);

        if (fp != NULL) {
            int lineCount = 0;
            int readChar = ' ';
            for (; lineCount < lineNumber && readChar != EOF;) {
                readChar = fgetc(fp);
                if (readChar == '\n')
                    lineCount++;
            }

            do {
                readChar = fgetc(fp);
                if (readChar == '\t')
                    readChar = ' ';
                fputc(readChar, outputStream);
            } while (readChar != EOF && readChar != '\n');

            for (int i = 0; i < row - 1; i++)
                putc(' ', outputStream);
            fputs("^\n", outputStream);
        }

        fflush(outputStream);
        if (warning) warnCount++;
        else {
            errorCount++;
#ifdef ONE_ERROR
            endfile();
#endif
        }
    }
    return condition;
}

bool endfile() {
    if (errorCount > 0) {
        if (warnCount > 0)
            fprintf(stderr, "%i %s and %i %s generated.", warnCount, warnCount > 1 ? "warnings" : "warning",
                    errorCount, errorCount > 1 ? "errors" : "error");
        else
            fprintf(stderr, "%i %s generated.", errorCount, errorCount > 1 ? "errors" : "error");
        errorCount = 0;
        warnCount = 0;
#ifndef MANU_EXIT
        exit(1);
#else
        return false;
#endif
    } else if (warnCount > 0)
        fprintf(stderr, "%i %s generated.", warnCount, warnCount > 1 ? "warnings" : "warning");
    errorCount = 0;
    warnCount = 0;
    return true;
}
