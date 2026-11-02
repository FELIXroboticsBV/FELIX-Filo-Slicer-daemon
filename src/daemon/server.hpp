
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

#ifndef FILO_SLICER_DAEMON_SERVER_HPP
#define FILO_SLICER_DAEMON_SERVER_HPP

#include <sys/socket.h>
#include <sys/un.h>

namespace felix_daemon {

class server
{
public:
    server();
    ~server();

private:
};

} // namespace felix_daemon

#endif // LEPROSY_SERVER_HPP
