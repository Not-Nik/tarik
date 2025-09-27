// tarik (c) Nikolas Wipper 2025

#include "System.h"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
#endif

// Standard-library-only fallback using argv[0] and PATH
std::filesystem::path find_executable(const char *argv0) {
    if (!argv0) {
        return {};
    }

    std::filesystem::path argPath(argv0);

    if (argPath.is_absolute() && std::filesystem::exists(argPath)) {
        return std::filesystem::canonical(argPath);
    }

    // If argv[0] has a parent i.e. an /, it can't come from PATH
    // If it doesn't have a parent, it must come from PATH
    if (argPath.has_parent_path()) {
        std::filesystem::path candidate = std::filesystem::current_path() / argPath;
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::canonical(candidate);
        }
        return {};
    }

    // Case 3: search PATH environment variable
    const char* pathEnv = std::getenv("PATH");
    if (pathEnv) {
        std::string pathStr(pathEnv);
        size_t start = 0;
        while (start < pathStr.size()) {
            size_t end = pathStr.find(':', start);
            if (end == std::string::npos) end = pathStr.size();
            std::filesystem::path dir = pathStr.substr(start, end - start);
            std::filesystem::path candidate = dir / argPath;
            if (std::filesystem::exists(candidate)) {
                return std::filesystem::canonical(candidate);
            }
            start = end + 1;
        }
    }


    return {};
}

std::filesystem::path get_executable_path(const char *argv0) {
    std::filesystem::path path;

#if defined(_WIN32)
    (void) argv0;
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0) {
        return {};
    }
    path = std::string(buffer, len);

#elif defined(__APPLE__)
    (void) argv0;
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) != 0) {
        return {};
    }
    path = std::string(buffer);

#elif defined(__linux__)
    (void) argv0;
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len == -1) {
        return {};
    }
    buffer[len] = '\0';
    path = std::string(buffer);

#else
#warning Your system doesn't have a native way to find the executable path. Falling back to trying to find one manually.
    path = find_executable(argv0);
#endif

    if (exists(path))
        return path;
    else
        return {};
}
