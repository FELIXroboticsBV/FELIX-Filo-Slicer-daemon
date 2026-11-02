#include "server.hpp"
#include <boost/log/trivial.hpp>

namespace felix_daemon {
server::server() { BOOST_LOG_TRIVIAL(info) << "Staring up daemon`s server"; }
server::~server() { BOOST_LOG_TRIVIAL(info) << "Shuting down daemon`s server"; }

} // namespace felix_daemon