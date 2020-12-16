#ifndef APP_SOCKET_USERS_H
#define APP_SOCKET_USERS_H

#include <string>
#include <vector>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>

namespace net = boost::asio;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;
using namespace std;

struct Users {
    std::shared_ptr<tcp::socket> socket_;
    std::shared_ptr<websocket::stream<tcp::socket>> websocket_;
    int memberID;    
    std::vector<int> storeID;
    string deviceID;
    string token;
    string deviceName;
};

#endif // __APP_SOCKET_USERS_H