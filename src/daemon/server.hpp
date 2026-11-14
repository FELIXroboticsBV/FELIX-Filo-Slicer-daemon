
// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/**
  ====================================================================================
  server.hpp
  ====================================================================================

  Class responsible for creating server and sockets
*/
#pragma once

#include "config.hpp"
#include "response.hpp"
#include "socket.hpp"
#include <boost/filesystem/path.hpp>

namespace felix_daemon {

/**
 * Server class is responsible for creating connection on the local UNIX sonet
 * Responses are handled, processed and send to the in the router.hpp
 */
class server
{
public:
    /**
     * server constructor
     *
     * @param  {response_trigger} handler : callback function for this server instance to use
     */
    server(request_trigger handler);

    /**
     * Starts up the server
     */
    void start();

    ~server();

private:
    /**
     * uses socket() syscall to crate local socet
     * @return {bool}  : true if the socket was correctly created false otherwise
     */
    bool createSocket();

    /**
     * Uses bind() / unbind() syscalls to unbind the socket to the kernel and thus make it available
     * @return {bool}  :
     */
    bool bindSocket();

private:
    local_socket m_socket;

    // path to the socket file
    boost::filesystem::path _socketPath;

    request_trigger _request_callback;

    bool _ready = false;
    std::atomic<bool> _running{false};
};

} // namespace felix_daemon
