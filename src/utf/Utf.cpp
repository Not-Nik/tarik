// tarik (c) Nikolas Wipper 2024

#include "Utf.h"

#include <locale>

std::string to_string(const std::u32string &utf_string) {
    return std::wstring_convert<deletable_facet<std::codecvt<char32_t, char, std::mbstate_t>>, char32_t>()
            .to_bytes(utf_string);
}

std::u32string to_utf(const std::string &string) {
    return std::wstring_convert<deletable_facet<std::codecvt<char32_t, char, std::mbstate_t>>, char32_t>()
            .from_bytes(string);
}

void replace_all(std::string &str, const std::string &needle, const std::string &replace) {
    // Add four spaces to the start of every line
    size_t index = 0;
    while (true) {
        // Locate the substring to replace.
        index = str.find(needle, index);
        if (index == std::string::npos)
            break;

        // Make the replacement.
        str.replace(index, 1, replace);

        // Advance index forward so the next iteration doesn't pick it up as well.
        index += replace.size();
    }
}
