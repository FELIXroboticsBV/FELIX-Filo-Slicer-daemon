
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
#include <boost/filesystem/path.hpp>
#include <sys/socket.h>
#include <sys/un.h>

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
    server(response_trigger handler);

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
    // address of the local socket
    struct sockaddr_un _addr;

    // socket file descriptor
    unsigned int _server_fd;

    // path to the socket file
    boost::filesystem::path _socketPath;

    response_trigger _handler;

    bool _ready = false;
    std::atomic<bool> _running{false};
};

} // namespace felix_daemon
