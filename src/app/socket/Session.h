#ifndef APP_SOCKET_SESSION_H
#define APP_SOCKET_SESSION_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <functional>
#include <thread>
#include <vector>
#include <map>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/beast.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/pool/pool_alloc.hpp>

#include <boost/serialization/vector.hpp>

#include <json/document.h>
#include <json/writer.h>
#include <json/stringbuffer.h>
#include <json/error/en.h>

#include "Log.h"
#include "Constants.h"
#include "Base.h"
#include "Version.h"
#include "Crypt.h"
#include "Users.h"

namespace net = boost::asio;
namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;
namespace json = rapidjson;

#define SOCKET_BUFFER_MAXSIZE 65535

class Session : public Base {
    friend class boost::serialization::access;
private:

public:
    Session() {};
    ~Session() {};

    void run(std::shared_ptr<tcp::socket> socket, std::vector<Users> &users, Instance *xsock);
    bool send(std::shared_ptr<tcp::socket> socket, std::stringstream &message);
    bool send_encrypt(std::shared_ptr<tcp::socket> socket, std::stringstream &message);
    bool send_encrypt_logout(std::shared_ptr<tcp::socket> socket, std::stringstream &message, std::vector<Users> &users, std::string token);
    void ping(std::vector<Users> &users);
};

#endif //APP_SOCKET_SESSION_H