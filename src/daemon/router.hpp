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

#include <boost/filesystem/path.hpp>
#include <cstddef>
#include <deque>
#include <json_fwd.hpp>
#include <optional>
#include <string>
#include <queue>
#include "response.hpp"
#include "orca_response.hpp"
#include "socket.hpp"

namespace felix_daemon {
class orca_session;
class router
{
public:
    router(orca_session& orca_session);

    /**
     * Callback that is called by the server when client is connected.
     * @param  {client} socket       : accepted client socket
     * @return {next_action}         : what the server should next ? (keep running , shutdown)
     */
    next_action process_request(client& socket);

private:
    /**
     * Checks if receive() actually read the data
     * @param  {std::ptrdiff_t} n : value returned by the receive() call
     * @return {bool}             : true if data was read, false on EOF or error
     */
    bool was_read_successful(std::ptrdiff_t n);

    /**
     * Processes the request issued from the client and sends
     * @param  {client} socket       : client socket
     * @param  {std::string} msg     : message that the client sent (json)
     * @return {bool}                : true if the server should be tuned off
     */
    bool handle_message(const client& socket, const std::string& msg);

    /**
     * Safely sends the message, safely means that it ensure that the entire message is written
     * @param  {client} socket       : client socket
     * @param  {std::string} msg     : message (inlined json)
     */
    void safe_send(const client& socket, const std::string& msg);

    /**
     * Forwards the request to the appropriate parts of the orca_session and invokes its calls
     * @param  {client} socket       : client socket
     * @param  {std::string} msg     : message (inlined json)
     */
    void process_request(const client& socket, const std::string& msg);

    /**
     * Creates error server response
     * USAGE: safe_send(socket, error(id, response_code::user_error, e.what()).stringify())
     * @param  {int} id             : id of the message that generated this message
     * @param  {response_code} code : code sent to the client
     * @param  {std::string} what   : error message that you want to send
     * @return {server_response}    : server_response object
     */
    server_response error(int id, response_code code, const std::string& what);

    /**
     * Maps the result code of an orca_session command to the code sent to the client
     */
    static response_code to_response_code(orca_response_code code);

    /**
     *  Creates success / ok server response
     * @param  {int} id           : id of the message that generated this message
     * @param  {std::string} msg  : message text
     * @param  {json} body        : message body / data
     * @return {server_response}  : server response object
     */
    server_response success(int id, const std::string& msg, json& body);

private:
    /**
     * Caches the request for the debugging
     * @param  {json} request : request that was sent to the server
     */
    void cache_request(json& request);

    /**
     * Wries json out the m_requestCache to the dst location
     * @param  {boost::filesystem::path} dst :
     */
    void dump_request_cache(const boost::filesystem::path& params);

private:
    std::deque<nlohmann::json> m_requestQueue;
    std::unordered_map<std::string, orca_cmd> m_commands;
};
} // namespace felix_daemon