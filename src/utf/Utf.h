// tarik (c) Nikolas Wipper 2024

#ifndef UTF_H
#define UTF_H

#include <string>

template <class Facet>
struct deletable_facet : Facet {
    template <class... Args>
    deletable_facet(Args &&... args)
        : Facet(std::forward<Args>(args)...) {}

    ~deletable_facet() {}
};

std::string to_string(const std::u32string& utf_string);
std::u32string to_utf(const std::string& string);

void replace_all(std::string &str, const std::string &needle, const std::string &replace);

#endif //UTF_H
