#ifndef APP_WEBSOCKET_SESSION_H
#define APP_WEBSOCKET_SESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <time.h>
#include "Constants.h"
#include "Users.h"
#include "Crypt.h"
#include "Log.h"
#include "Base.h"
#include "Version.h"
#include "base64.h"

#include <json/document.h>
#include <json/writer.h>
#include <json/stringbuffer.h>
#include <json/error/en.h>


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace json = rapidjson;
using tcp = boost::asio::ip::tcp;

#define WEBSOCKET_BUFFER_MAXSIZE 65535

class Session : public Base {
private:

public:
    Session() {};
    ~Session() {};

    void run(std::shared_ptr<websocket::stream<tcp::socket>> websocket, std::vector<Users> &users, unsigned int userLimit);
    bool send(std::shared_ptr<websocket::stream<tcp::socket>> websocket, std::stringstream &message);
    bool send_encrypt(std::shared_ptr<websocket::stream<tcp::socket>> websocket, std::stringstream &message);
    void ping(std::vector<Users> &users);
};

#endif