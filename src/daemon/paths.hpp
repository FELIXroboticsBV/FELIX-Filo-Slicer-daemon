// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/**
  ====================================================================================
  paths.hpp
  ====================================================================================

  Cross platform path resolutions
*/

#pragma once

#include "config.hpp"
#include <boost/dll/runtime_symbol_info.hpp>
#include <cstdlib>
#include <boost/filesystem.hpp>

namespace felix_daemon {

/**
 * Platform-standard per-user data directory, where this program can write into
 *   Linux:   $XDG_CONFIG_HOME/FELIX/orca-daemon  (default ~/.config/FELIX/orca-daemon)
 *   Windows: %APPDATA%\FELIX\orca-daemon
 *   macOS:   ~/Library/Application Support/FELIX/orca-daemon
 * During debug build this will be index
 * @return {boost::filesystem::path}  :
 */
inline boost::filesystem::path default_data_directory()
{
    namespace fs       = boost::filesystem;
    const fs::path app = fs::path("FELIX") / cfg.socketName;

#if defined(DEBUG)
    return fs::path("./src/daemon");
#endif

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

inline boost::filesystem::path config_path()
{
    namespace fs        = boost::filesystem;
    const fs::path file = default_data_directory() / "config.ini";

    if (!fs::exists(file)) {
        fs::create_directories(file.parent_path());
        felix_daemon::daemon_config{}.save(file.string());
    }
    return file;
}

/**
 * Orca's read-only resources folder (profiles, shaders, images, ...)
 *   Debug:   <orca repo>/resources, from CMake (ORCA_RESOURCES_DIR)
 *   Windows: <exe dir>/resources
 *   macOS:   <exe dir>/../Resources
 *   Linux:   <exe dir>/../resources
 * @return {boost::filesystem::path}  : path to the resources folder
 */
inline boost::filesystem::path resources_directory()
{
    namespace fs = boost::filesystem;

#if defined(ORCA_RESOURCES_DIR)
    fs::path dir = ORCA_RESOURCES_DIR;
#else
    const fs::path exeDir = boost::dll::program_location().parent_path();
#if defined(_WIN32)
    fs::path dir = exeDir / "resources";
#elif defined(__APPLE__)
    fs::path dir = exeDir / ".." / "Resources";
#else
    fs::path dir = exeDir / ".." / "resources";
#endif
#endif

    dir = fs::weakly_canonical(dir);
    if (!fs::exists(dir / "profiles"))
        throw std::runtime_error("orca resources not found: " + dir.string());
    return dir;
}

/**
 * Creates a socket with a name provided by the felix-slicerd.ini
 * @return {boost::filesystem::path}  : path to the socket file
 */
inline boost::filesystem::path socket_path()
{
    namespace fs           = boost::filesystem;
    const std::string name = cfg.socketName;

#if defined(__linux__)
    if (const char* runtime = std::getenv("XDG_RUNTIME_DIR"); runtime && *runtime)
        return fs::path(runtime) / name;
    return fs::temp_directory_path() / (name + "-" + std::to_string(::getuid()) + ".sock");
#else
    // macOS ($TMPDIR) and Windows (GetTempPathW) both return a per-user folder.
    return fs::temp_directory_path() / name;
#endif
}

} // namespace felix_daemon