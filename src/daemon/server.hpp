
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
#include <boost/filesystem/path.hpp>
#include <sys/socket.h>
#include <sys/un.h>

namespace felix_daemon {

class server
{
public:
    server();

    template<typename T> void start(T& payload) { listen(_server_fd, cfg.max_pending_connections); }

    ~server();

private:
    bool createSocket();
    bool bindSocket();

private:
    struct sockaddr_un _addr;
    // socket file descriptor
    unsigned int _server_fd;

    // path to the socket file
    boost::filesystem::path _socketPath;
};

} // namespace felix_daemon
