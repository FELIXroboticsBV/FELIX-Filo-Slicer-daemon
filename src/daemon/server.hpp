
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
#include "connection_handler.hpp"
#include <boost/filesystem/path.hpp>
#include <sys/socket.h>
#include <sys/un.h>

namespace felix_daemon {

class server
{
public:
    server(connection_handler handler);

    void start();

    ~server();

private:
    bool createSocket();
    bool bindSocket();

    bool send_all(int df, const std::string& msg);

private:
    struct sockaddr_un _addr;

    // socket file descriptor
    unsigned int _server_fd;

    // path to the socket file
    boost::filesystem::path _socketPath;

    connection_handler _handler;

    bool _ready = false;
    std::atomic<bool> _running{false};
};

} // namespace felix_daemon
