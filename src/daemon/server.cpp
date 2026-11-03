// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "server.hpp"
#include "connection_handler.hpp"
#include "paths.hpp"

#include <boost/log/trivial.hpp>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <afunix.h> // AF_UNIX / sockaddr_un on Windows
#else
#include <sys/socket.h>
#include <sys/un.h> // sockaddr_un
#include <unistd.h> // close, read, unlink
#endif

#include <csignal>

namespace felix_daemon {

//===========================================================================
// Utility

volatile std::sig_atomic_t g_stopRequested = 0;

void onSignal(int) { g_stopRequested = 1; }

void installSignalHandlers()
{
    struct sigaction sa{};
    sa.sa_handler = onSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);  // Ctrl+C
    sigaction(SIGTERM, &sa, nullptr); // kill <pid>

    std::signal(SIGPIPE, SIG_IGN);
}
//===========================================================================

//============================================================================
// Server implementation
server::server(connection_handler handler) : _socketPath(socket_path()), _handler(handler)
{
    BOOST_LOG_TRIVIAL(info) << "Staring up daemon`s server";

    _server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (!createSocket()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to set address of the socket because the address is too long !";
    }

    if (!bindSocket()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to bind socket !";
    }

    if (listen(_server_fd, cfg.max_pending_connections) != 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to listen on the socket !";
    }

    _ready = true;

    BOOST_LOG_TRIVIAL(info) << "Server initialized \n \t listening on socket: " << _socketPath.string();
}

void server::start()
{
    if (!_ready) {
        BOOST_LOG_TRIVIAL(error) << "Server is not initialized, refusing to start";
        return;
    }

    // makes server listen to the kill process handles
    installSignalHandlers();

    _running = true;

    while (_running && !g_stopRequested) {
        // wait until the client connects
        int client_fd = accept(_server_fd, nullptr, nullptr);
        if (client_fd == 0)
            continue;

        BOOST_LOG_TRIVIAL(info) << "Client connected";

        // handler will process the data and return the result to the user
        // optionally it will also write back to the user
        auto result = _handler(client_fd);

        close(client_fd);
        BOOST_LOG_TRIVIAL(info) << "Client disconnected";

        if (result == next_action::Shutdown) {
            BOOST_LOG_TRIVIAL(info) << "Shutdown requested by handler";
            _running = false;
        }
    }

    _running = false;
    BOOST_LOG_TRIVIAL(info) << "Server loop stopped";
}

bool server::createSocket()
{
    const std::string p = _socketPath.string();
    std::memset(&_addr, 0, sizeof(_addr));
    _addr.sun_family = AF_UNIX;

    if (p.size() >= sizeof(_addr.sun_path))
        return false;

    std::memcpy(_addr.sun_path, p.c_str(), p.size() + 1);
    return true;
}

bool server::bindSocket()
{
    // first unlink the socket from the previous use
    unlink(_socketPath.c_str());
    int result = bind(_server_fd, (struct sockaddr*) &_addr, sizeof(_addr));

    return result == 0;
}

server::~server()
{
    if (close(_server_fd) != 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to close file descriptor of the server socket";
    }

    unlink(_socketPath.c_str());

    BOOST_LOG_TRIVIAL(info) << "Shuting down daemon`s server";
}

} // namespace felix_daemon