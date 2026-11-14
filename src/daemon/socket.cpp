// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <afunix.h> // AF_UNIX, sockaddr_un
#include <algorithm>
#include <climits>
#else
#include <cerrno>
#include <sys/socket.h>
#include <sys/un.h> // sockaddr_un
#include <unistd.h> // close
#endif

#include "socket.hpp"

#include <cstring>

namespace felix_daemon {

#ifdef _WIN32
static_assert(sizeof(socket_t) == sizeof(SOCKET), "socket_t must hold a SOCKET");
#endif

local_socket::local_socket() : m_fd(invalid_socket)
{
#ifdef _WIN32
    // reference counted, every socket initializes and releases it once
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return;
#endif
    m_fd = static_cast<socket_t>(socket(AF_UNIX, SOCK_STREAM, 0));
}

local_socket::local_socket(socket_t fd) : m_fd(fd)
{
#ifdef _WIN32
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
#endif
}

local_socket::~local_socket()
{
#ifdef _WIN32
    if (m_fd != invalid_socket)
        closesocket(m_fd);
    WSACleanup();
#else
    if (m_fd != invalid_socket)
        close(m_fd);
#endif
}

bool local_socket::is_valid() const { return m_fd != invalid_socket; }

bool local_socket::listen_on(const std::string& path, int backlog)
{
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (path.size() >= sizeof(addr.sun_path))
        return false;
    std::memcpy(addr.sun_path, path.c_str(), path.size() + 1);
    // bind the socket
    bool bindSuccessful = bind(m_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;

    // listen on the socket
    bool listenSuccessful = listen(m_fd, backlog) == 0;

    return bindSuccessful && listenSuccessful;
}

local_socket local_socket::accept_client() const { return local_socket(static_cast<socket_t>(accept(m_fd, nullptr, nullptr))); }

std::ptrdiff_t local_socket::receive(char* buf, std::size_t len) const
{
#ifdef _WIN32
    return recv(m_fd, buf, static_cast<int>(std::min<std::size_t>(len, INT_MAX)), 0);
#else
    return recv(m_fd, buf, len, 0);
#endif
}

bool local_socket::send_all(const std::string& msg) const
{
    const char* p    = msg.data();
    std::size_t left = msg.size();
    while (left > 0) {
#ifdef _WIN32
        std::ptrdiff_t n = send(m_fd, p, static_cast<int>(std::min<std::size_t>(left, INT_MAX)), 0);
#elif defined(MSG_NOSIGNAL)
        std::ptrdiff_t n = send(m_fd, p, left, MSG_NOSIGNAL);
#else
        std::ptrdiff_t n = send(m_fd, p, left, 0); // macOS: SIGPIPE has to be ignored by the caller
#endif
        if (n < 0) {
            if (interrupted())
                continue;
            return false;
        }
        p += n;
        left -= static_cast<std::size_t>(n);
    }
    return true;
}

bool local_socket::interrupted()
{
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

std::string local_socket::last_error()
{
#ifdef _WIN32
    return "WSA error " + std::to_string(WSAGetLastError());
#else
    return std::strerror(errno);
#endif
}

int local_socket::get_fd() { return m_fd; }

} // namespace felix_daemon
