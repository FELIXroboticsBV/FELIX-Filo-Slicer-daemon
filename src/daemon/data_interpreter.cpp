#include "data_interpreter.hpp"
#include "connection_handler.hpp"
#include "server.hpp"
#include "boost/log/trivial.hpp"
#include <json_fwd.hpp>

namespace felix_daemon {

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

    if (!pending.empty() && handleMessage(client_fd, pending))
        return next_action::Shutdown;

    return next_action::KeepRuning;
}

bool data_interpreter::send_all(int client_fd, const std::string& msg)
{
    const char* p = msg.data();
    size_t left   = msg.size();
    while (left > 0) {
        ssize_t n = send(client_fd, p, left, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        p += n;
        left -= static_cast<size_t>(n);
    }
    return true;
}

bool data_interpreter::handleMessage(int client_fd, const std::string& msg)
{
    BOOST_LOG_TRIVIAL(info) << "Received: " << msg;

    if (msg == "shutdown") {
        return true;
    }

    nlohmann::json j;
    j["test"]    = "test";
    j["message"] = msg;
    server_response response{1, next_action::KeepRuning, response_code::OK, j};

    if (!send_all(client_fd, response.stringify())) {
        BOOST_LOG_TRIVIAL(error) << "Failed to send response to the user";
    }

    return false;
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

} // namespace felix_daemon