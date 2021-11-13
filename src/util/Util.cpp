// tarik (c) Nikolas Wipper 2021

#include "Util.h"

#include <comperr.h>

void comp_msg(LexerPos pos, message_kind kind, std::string what, va_list args) {
    vcomperr(what.c_str(), kind, pos.filename.c_str(), pos.l, pos.p, args);
}

void error(LexerPos pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    comp_msg(pos, ERROR, what, args);
    va_end(args);
}

void warning(LexerPos pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    comp_msg(pos, WARNING, what, args);
    va_end(args);
}

void note(LexerPos pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    comp_msg(pos, NOTE, what, args);
    va_end(args);
}

bool iassert(bool cond, LexerPos pos, std::string what, ...) {
    va_list args;
    va_start(args, what);
    iassert(cond, pos, what, args);
    va_end(args);
    return cond;
}

bool iassert(bool cond, LexerPos pos, std::string what, va_list list) {
    if (!cond) vcomperr(what.c_str(), ERROR, pos.filename.c_str(), pos.l, pos.p, list);
    return cond;
}