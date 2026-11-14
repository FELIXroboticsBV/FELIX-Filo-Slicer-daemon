// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/**
  ====================================================================================
  socket.hpp
  ====================================================================================

  Platform independent way of using the socket

  Local (AF_UNIX) stream sockets on POSIX and on Windows (Winsock, Windows 10 1803+).
  Platform headers are only included in socket.cpp: <winsock2.h> must not leak into
  other files, it clashes with <windows.h> when that one is included first.
*/

#pragma once

#include <cstddef>
#include <string>

#ifdef _WIN32
#include <cstdint>
#endif

namespace felix_daemon {

#ifdef _WIN32
using socket_t                    = std::uintptr_t; // SOCKET
constexpr socket_t invalid_socket = ~socket_t(0);   // INVALID_SOCKET
#else
using socket_t                    = int;
constexpr socket_t invalid_socket = -1;
#endif

class local_socket
{
public:
    /**
     * Creates the local stream socket (and initializes Winsock on Windows)
     */
    local_socket();

    /**
     * Closes the socket (and releases Winsock on Windows)
     */
    ~local_socket();

    // delete copy constructor
    local_socket(const local_socket&)            = delete;
    local_socket& operator=(const local_socket&) = delete;

    /**
     * Checks if the socket was created successfully
     * @return {bool}  : true if the socket can be used
     */
    bool is_valid() const;

    /**
     * Binds the socket to the path and starts listening on it
     * @param  {std::string} path : path of the socket file
     * @param  {int} backlog      : maximum number of pending connections
     * @return {bool}             : true on success, false otherwise
     */
    bool listen_on(const std::string& path, int backlog);

    /**
     * Blocks until a client connects
     * @return {local_socket}  : connected client, not valid on failure or when interrupted
     */
    local_socket accept_client() const;

    /**
     * Reads the data that arrived on the socket
     * @param  {char*} buf       : buffer to read into
     * @param  {std::size_t} len : size of the buffer
     * @return {std::ptrdiff_t}  : bytes read, 0 if the client closed the connection, < 0 on error
     */
    std::ptrdiff_t receive(char* buf, std::size_t len) const;

    /**
     * Sends the entire message
     * @param  {std::string} msg : message to send
     * @return {bool}            : true if the whole message was sent, false otherwise
     */
    bool send_all(const std::string& msg) const;

    /**
     * Checks if the last failed call was interrupted by a signal (EINTR)
     * @return {bool}  : true if it was interrupted
     */
    static bool interrupted();

    /**
     * Describes the last socket error
     * @return {std::string}  : error message, for logging
     */
    static std::string last_error();

private:
    /**
     * Wraps an already connected handle (used by accept_client)
     * @param  {socket_t} fd : handle returned by accept()
     */
    explicit local_socket(socket_t fd);

private:
    socket_t m_fd;
};

} // namespace felix_daemon
