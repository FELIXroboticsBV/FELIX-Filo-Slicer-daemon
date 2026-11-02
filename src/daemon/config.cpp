#include "config.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <cassert>

namespace felix_daemon {

using boost::property_tree::ptree;

void daemon_config::load(const std::string& filename)
{
    assert(loaded == false && "you have allready called load !");

    ptree pt;

    boost::property_tree::read_ini(filename, pt);

    // parse the init
    socketName              = pt.get<std::string>("daemon.socket_name", socketName);
    max_pending_connections = pt.get<int>("daemon.max_pending_connections", max_pending_connections);
}

void daemon_config::save(const std::string& filename)
{
    ptree pt;

    pt.put("daemon.socket_name", socketName);
    pt.put("daemon.max_pending_connections", max_pending_connections);

    boost::property_tree::write_ini(filename, pt);
}

} // namespace felix_daemon