// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

/**
  ====================================================================================
  router.hpp
  ====================================================================================

  Interprets the raw json request and executes different commands that call orca
  specific functionality
*/

#pragma once

#include <json_fwd.hpp>
#include <optional>
#include <string>
#include "response.hpp"
#include "orca_response.hpp"

namespace felix_daemon {
class orca_session;
class router
{
public:
    router(orca_session& orca_session);

    /**
     * Callback that is called by the server when client is connected.
     * @param  {int} client_fd : accepted clinet file descriptor
     * @return {next_action}   : what the server should next ? (keep running , shutdown)
     */
    next_action operator()(int client_fd);

private:
    /**
     * Checks if read() system call actually read the entire message
     * @param  {size_t} n : size that was read by read() command
     * @return {bool}     : true if entire entire request was read
     */
    bool was_read_successful(size_t n);

    /**
     * Processes the request issued from the client and sends
     * @param  {int} client_fd   : file descriptor of the client
     * @param  {std::string} msg : message that the client sent (json)
     * @return {bool}            : true if the server should be tuned off
     */
    bool handle_message(int client_fd, const std::string& msg);

    /**
     * Safely sends the message, safely means that it ensure that the entire message is written
     * @param  {int} clinet_fd   : client's file descriptor
     * @param  {std::string} msg : message (inlined json)
     */
    void safe_send(int clinet_fd, const std::string& msg);

    /**
     * Forwards the request to the appropriate parts of the orca_session and invokes its calls
     * @param  {int} clinet_fd   : client file descriptor
     * @param  {std::string} msg : message (inlined json)
     */
    void process_request(int clinet_fd, const std::string& msg);

    /**
     * Creates error server response
     * USAGE: safe_send(client_fd, error(id, e.what().stringify()))
     * @param  {int} id           : client file descriptor
     * @param  {std::string} what : error message that you want to send
     * @return {server_response}  : server_response object
     */
    server_response error(int id, const std::string& what);

    /**
     *  Creates success / ok server response
     * @param  {int} id           : id of the message that generated this message
     * @param  {std::string} msg  : message text
     * @param  {json} body        : message body / data
     * @return {server_response}  : server response object
     */
    server_response success(int id, const std::string& msg, json& body);

private:
    std::unordered_map<std::string, orca_cmd> m_commands;
};
} // namespace felix_daemon