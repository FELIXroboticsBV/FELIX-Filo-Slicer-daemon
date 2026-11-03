#include "data_interpreter.hpp"
#include "connection_handler.hpp"
#include "server.hpp"
#include "boost/log/trivial.hpp"
#include <json_fwd.hpp>

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

data_interpreter::data_interpreter()
{ // register the commands
  // m_commands.emplace("status", [this](int client_fd, const json& params) { return cmd })
}

next_action data_interpreter::operator()(int client_fd)
{
    server_response response;
    // data that will arrive at the server
    std::string pending;
    char buf[4096];

    while (true) {
        // read the data tha thave arived at the server
        ssize_t n = read(client_fd, buf, sizeof(buf));
        if (!wasReadSuccessful(n)) {
            break;
        }

        pending.append(buf, static_cast<size_t>(n));

        std::size_t pos;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            pending.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (handleMessage(client_fd, line)) {
                return next_action::Shutdown;
            }
        }
    }

    auto shouldClose = handleMessage(client_fd, pending);
    if (!pending.empty() && shouldClose)
        return next_action::Shutdown;

    return next_action::KeepRuning;
}

bool data_interpreter::handleMessage(int client_fd, const std::string& msg)
{
    BOOST_LOG_TRIVIAL(info) << "Received: " << msg;

    if (msg == "shutdown") {
        return true;
    }

    process_request(client_fd, msg);

    return false;
}

void data_interpreter::process_request(int client_fd, const std::string& msg)
{
    json src;
    try {
        src = json::parse(msg);
    } catch (const json::parse_error& e) {
        send_all(client_fd, error(-1, e.what()).stringify());
    }

    int id = (src.contains("id") && src["id"].is_number_integer()) ? src["id"].get<int>() : 0;
    try {
        request req(src);
        if (req.cmd == "status") {
            nlohmann::json j;
            j["status"] = "complete !";
            server_response response{id, next_action::KeepRuning, response_code::OK, j};
            send_all(client_fd, response.stringify());
        }
        if (req.cmd == "err") {
            send_all(client_fd, error(id, "Test error").stringify());
        }
        // TODO: here call the handle from the registered commands

    } catch (const std::exception& e) {
        send_all(client_fd, error(id, e.what()).stringify());
    }
}

void data_interpreter::send_all(int client_fd, const std::string& msg)
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

bool data_interpreter::wasReadSuccessful(size_t n)
{
    if (n == 0) {
        BOOST_LOG_TRIVIAL(warning) << "Client have closed the connection ";
        return false;
    }
    if (n < 0) {
        // EINTR here means SIGINT/SIGTERM arrived: return and let the server decide.
        if (errno != EINTR)
            BOOST_LOG_TRIVIAL(error) << "read() failed: " << std::strerror(errno);
        return true;
    }
    return true;
}

server_response data_interpreter::error(int id, const std::string& what)
{
    nlohmann::json resp;
    resp["what"] = what;
    return {id, next_action::KeepRuning, response_code::SERVER_ERROR, resp, what};
}

} // namespace felix_daemon