// tarik (c) Nikolas Wipper 2024

#include "Utf.h"

#include <locale>

std::string to_string(const std::u32string &utf_string) {
    return std::wstring_convert<deletable_facet<std::codecvt<char32_t, char, std::mbstate_t>>, char32_t>()
            .to_bytes(utf_string);
}

std::u32string to_utf(const std::string& string) {
    return std::wstring_convert<deletable_facet<std::codecvt<char32_t, char, std::mbstate_t>>, char32_t>()
            .from_bytes(string);
}
