// Copyright (c) 2026 FELIX Robotics BV
// SPDX-License-Identifier: AGPL-3.0-or-later
//
// This file is part of the FELIX fork of OrcaSlicer.
// Licensed under the GNU Affero General Public License v3.0 or later.
// See the LICENSE file in the repository root for details.

#include "router.hpp"
#include "boost/log/trivial.hpp"
#include "orca_session.hpp"
#include <json_fwd.hpp>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace felix_daemon {

struct request
{
    int id = 0;
    std::string cmd;
    json params = json::object();

    explicit request(const json& src)
    {
        if (auto it = src.find("id"); it != src.end() && it->is_number_integer())
            id = it->get<int>();

        auto c = src.find("cmd");

        if (c == src.end() || !c->is_string() || c->get<std::string>().empty())
            throw std::invalid_argument("missing or invalid 'cmd'");
        cmd = c->get<std::string>();

        if (auto p = src.find("params"); p != src.end() && !p->is_null()) {
            if (!p->is_object())
                throw std::invalid_argument("'params' must be an object");
            params = *p;
        }
    }
};

router::router(orca_session& orca_session)
{
    m_commands.emplace("load_model", [s = &orca_session](const json& p) { return s->cmd_load_model(p); });
    m_commands.emplace("add_instance", [s = &orca_session](const json& p) { return s->cmd_instantiate_object(p); });
    m_commands.emplace("set_transform", [s = &orca_session](const json& p) { return s->cmd_set_transform(p); });
    m_commands.emplace("set_printer", [s = &orca_session](const json& p) { return s->cmd_set_printer(p); });
    m_commands.emplace("set_filament", [s = &orca_session](const json& p) { return s->cmd_set_filament(p); });
    m_commands.emplace("set_process", [s = &orca_session](const json& p) { return s->cmd_set_process(p); });
    m_commands.emplace("slice", [s = &orca_session](const json& p) { return s->cmd_slice(p); });
    m_commands.emplace("slice_status", [s = &orca_session](const json& p) { return s->cmd_check_slice_progress(p); });
    m_commands.emplace("get_state", [s = &orca_session](const json& p) { return s->cmd_get_preset_info(p); });
    m_commands.emplace("remove_model", [s = &orca_session](const json& p) { return s->cmd_remove_model(p); });
    m_commands.emplace("remove_instance", [s = &orca_session](const json& p) { return s->cmd_remove_instance(p); });
}

next_action router::operator()(int client_fd)
{
    server_response response;
    // data that will arrive at the server
    std::string pending;
    char buf[4096];

    while (true) {
        // read the data tha thave arived at the server
        ssize_t n = read(client_fd, buf, sizeof(buf));
        if (!was_read_successful(n)) {
            break;
        }

        pending.append(buf, static_cast<size_t>(n));

        std::size_t pos;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            pending.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (handle_message(client_fd, line)) {
                return next_action::shutdown;
            }
        }
    }

    if (!pending.empty() && handle_message(client_fd, pending))
        return next_action::shutdown;

    return next_action::keep_running;
}

bool router::handle_message(int client_fd, const std::string& msg)
{
    BOOST_LOG_TRIVIAL(info) << "Received: " << msg;

    if (msg == "shutdown") {
        return true;
    }

    process_request(client_fd, msg);

    return false;
}

void router::process_request(int client_fd, const std::string& msg)
{
    json src;
    try {
        src = json::parse(msg);
    } catch (const json::parse_error& e) {
        safe_send(client_fd, error(-1, response_code::user_error, e.what()).stringify());
        return;
    }

    int id = (src.is_object() && src.contains("id") && src["id"].is_number_integer()) ? src["id"].get<int>() : 0;
    try {
        request req(src);

        auto action = m_commands.find(req.cmd);
        if (action == m_commands.end()) {
            safe_send(client_fd, error(id, response_code::not_found, "Unknown command: " + req.cmd).stringify());
            return;
        }
        auto response = action->second(req.params);

        if (response.failed) {
            safe_send(client_fd, error(id, to_response_code(response.code), response.message).stringify());
        } else {
            auto body = response.json.value_or({});
            safe_send(client_fd, success(id, response.message, body).stringify());
        }

    } catch (const std::invalid_argument& e) {
        safe_send(client_fd, error(id, response_code::user_error, e.what()).stringify());
    } catch (const std::exception& e) {
        safe_send(client_fd, error(id, response_code::server_error, e.what()).stringify());
    }
}

response_code router::to_response_code(orca_response_code code)
{
    switch (code) {
    case orca_response_code::ok: return response_code::ok;
    case orca_response_code::invalid_parameters: return response_code::user_error;
    case orca_response_code::not_found: return response_code::not_found;
    case orca_response_code::busy: return response_code::busy;
    case orca_response_code::failed: return response_code::server_error;
    }
    return response_code::server_error;
}

void router::safe_send(int client_fd, const std::string& msg)
{
    const char* p = msg.data();
    size_t left   = msg.size();
    while (left > 0) {
        ssize_t n = send(client_fd, p, left, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                BOOST_LOG_TRIVIAL(error) << "Failed to response to the user";
                return;
            }
        }
        p += n;
        left -= static_cast<size_t>(n);
    }
}

bool router::was_read_successful(ssize_t n)
{
    if (n == 0) {
        BOOST_LOG_TRIVIAL(warning) << "Client have closed the connection ";
        return false;
    }
    if (n < 0) {
        // EINTR here means SIGINT/SIGTERM arrived: return and let the server decide.
        if (errno != EINTR)
            BOOST_LOG_TRIVIAL(error) << "read() failed: " << std::strerror(errno);
        return false;
    }
    return true;
}

server_response router::error(int id, response_code code, const std::string& what)
{
    BOOST_LOG_TRIVIAL(error) << what;

    nlohmann::json resp;
    resp["what"] = what;
    return {id, next_action::keep_running, code, resp, what};
}

server_response router::success(int id, const std::string& msg, json& body)
{
    BOOST_LOG_TRIVIAL(info) << "Processed request \n \t end message: " << msg;
    return {id, next_action::keep_running, response_code::ok, body};
}

} // namespace felix_daemon