// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "server.hpp"
#include "response.hpp"
#include "paths.hpp"
#include "socket.hpp"

#include <boost/log/trivial.hpp>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <afunix.h> // AF_UNIX / sockaddr_un on Windows
// note that this is supported from windows 10 (which should be deprecated by this point)
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
server::server(request_trigger handler) : _socketPath(socket_path()), _request_callback(handler), m_socket()
{
    BOOST_LOG_TRIVIAL(info) << "Staring up daemon`s server";

    if (!m_socket.listen_on(_socketPath.string(), cfg.max_pending_connections)) {
        BOOST_LOG_TRIVIAL(error) << "Failed to set address of the socket because the address is too long !";
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
        // when the connection goes out of scope it is closed
        client connection = m_socket.accept_client();
        if (!connection.is_valid())
            continue;

        auto result = _request_callback(connection);
        BOOST_LOG_TRIVIAL(info) << "Client disconnected";

        if (result == next_action::shutdown) {
            BOOST_LOG_TRIVIAL(info) << "Shutdown requested by handler";
            _running = false;
        }
    }

    _running = false;
    BOOST_LOG_TRIVIAL(info) << "Server loop stopped";
}

server::~server()
{
    boost::system::error_code ec;
    boost::filesystem::remove(_socketPath, ec);

    BOOST_LOG_TRIVIAL(info) << "Shuting down daemon`s server";
}

} // namespace felix_daemon