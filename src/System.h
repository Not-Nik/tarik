// tarik (c) Nikolas Wipper 2025

#ifndef TARIK_SYSTEM_H
#define TARIK_SYSTEM_H

#include <filesystem>

std::filesystem::path get_executable_path(const char *argv0);

#endif //TARIK_SYSTEM_H