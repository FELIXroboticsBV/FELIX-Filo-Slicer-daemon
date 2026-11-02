#include "data_interpreter.hpp"
#include "connection_handler.hpp"
#include "server.hpp"
#include "boost/log/trivial.hpp"

namespace felix_daemon {
HandlerResult data_interpreter::operator()(int client_fd)
{
    std::string pending;
    char buf[4096];

    while (true) {
        ssize_t n = read(client_fd, buf, sizeof(buf));
        if (n == 0)
            BOOST_LOG_TRIVIAL(warning) << "Client have closed the connection ";
        break;
        if (n < 0) {
            // EINTR here means SIGINT/SIGTERM arrived: return and let the server decide.
            if (errno != EINTR)
                BOOST_LOG_TRIVIAL(error) << "read() failed: " << std::strerror(errno);
            return HandlerResult::KeepRuning;
        }

        pending.append(buf, static_cast<size_t>(n));

        std::size_t pos;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string line = pending.substr(0, pos);
            pending.erase(0, pos + 1);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            if (handleMessage(client_fd, line))
                return HandlerResult::Shutdown;
        }
    }

    if (!pending.empty() && handleMessage(client_fd, pending))
        return HandlerResult::Shutdown;

    return HandlerResult::KeepRuning;
}

bool data_interpreter::handleMessage(int client_fd, const std::string& msg)
{
    BOOST_LOG_TRIVIAL(info) << "Received: " << msg;

    if (msg == "shutdown") {
        write(client_fd, "bye\n", 4);
        return true;
    }

    // g-code will go here
    write(client_fd, "ok\n", 3);
    return false;
}

} // namespace felix_daemon