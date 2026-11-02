#include "server.hpp"
#include "paths.hpp"

#include <boost/log/trivial.hpp>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <afunix.h> // AF_UNIX / sockaddr_un on Windows
#else
#include <sys/socket.h>
#include <sys/un.h> // sockaddr_un
#include <unistd.h> // close, read, unlink
#endif

namespace felix_daemon {
server::server() : _socketPath(socket_path())
{
    BOOST_LOG_TRIVIAL(info) << "Staring up daemon`s server";

    _server_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (!createSocket()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to set address of the socket because the address is too long !";
    }

    if (!bindSocket()) {
        BOOST_LOG_TRIVIAL(error) << "Failed to bind socket !";
    }

    BOOST_LOG_TRIVIAL(info) << "Server initialized \n \t path to the socket: " << _socketPath.string();
}

bool server::createSocket()
{
    const std::string p = _socketPath.string();
    std::memset(&_addr, 0, sizeof(_addr));
    _addr.sun_family = AF_UNIX;

    if (p.size() >= sizeof(_addr.sun_path))
        return false;

    std::memcpy(_addr.sun_path, p.c_str(), p.size() + 1);
    return true;
}

bool server::bindSocket()
{
    // first unlink the socket from the previous use
    unlink(_socketPath.c_str());
    int result = bind(_server_fd, (struct sockaddr*) &_addr, sizeof(_addr));

    return result == 0;
}

server::~server()
{
    if (close(_server_fd) != 0) {
        BOOST_LOG_TRIVIAL(error) << "Failed to close file descriptor of the server socket";
    }
    BOOST_LOG_TRIVIAL(info) << "Shuting down daemon`s server";
}

} // namespace felix_daemon