// src/daemon/Paths.hpp
#pragma once

#include <cstdlib>
#include <boost/filesystem.hpp>

namespace felix_daemon {

// Platform-standard per-user data directory, resolved at runtime:
//   Linux:   $XDG_CONFIG_HOME/FELIX/orca-daemon  (default ~/.config/FELIX/orca-daemon)
//   Windows: %APPDATA%\FELIX\orca-daemon
//   macOS:   ~/Library/Application Support/FELIX/orca-daemon
inline boost::filesystem::path default_data_directory()
{
    namespace fs       = boost::filesystem;
    const fs::path app = fs::path("FELIX") / "orca-daemon";

#if defined(_WIN32)
    // Wide version so user names with non-ASCII characters work.
    if (const wchar_t* appdata = _wgetenv(L"APPDATA"); appdata && *appdata)
        return fs::path(appdata) / app;
#else
    auto env = [](const char* name) -> const char* {
        const char* v = std::getenv(name);
        return (v && *v) ? v : nullptr;
    };
#if defined(__APPLE__)
    if (const char* home = env("HOME"))
        return fs::path(home) / "Library" / "Application Support" / app;
#else
    if (const char* xdg = env("XDG_CONFIG_HOME"))
        return fs::path(xdg) / app;
    if (const char* home = env("HOME"))
        return fs::path(home) / ".config" / app;
#endif
#endif

    // Last resort if the environment is unusually bare.
    return fs::temp_directory_path() / app;
}

} // namespace felix_daemon