#ifndef APP_SOCKET_CORE_H
#define APP_SOCKET_CORE_H

#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <unistd.h>
#include <pwd.h>
#include <vector>
#include <map>
#include <fstream>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/beast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/system/error_code.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "Log.h"
#include "Constants.h"
#include "Base.h"
#include "Version.h"
#include "Crypt.h"
#include "Session.h"
#include "Users.h"

using namespace std;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

#define LD_UINT8_MAXLEN		(3 + 1)
#define LD_UINT16_MAXLEN	(5 + 1)
#define LD_UINT32_MAXLEN	(10 + 1)
#define LD_UINT64_MAXLEN	(20 + 1)
#define LD_UINTMAX_MAXLEN	LD_UINT64_MAXLEN

class Core : public Base {
private:

public:
    Core() {};
    ~Core() {};
    bool terminate(Instance *xsock);
    bool force_terminate(Instance *xsock);
    void core_loop(Instance *xsock);
    void read_pidfile(Instance *xsock);
};

#endif //APP_SOCKET_CORE_H