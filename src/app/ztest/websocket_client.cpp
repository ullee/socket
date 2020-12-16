#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <string>
#include "Crypt.h"
#include "base64.h"
#include <chrono>

#include <json/document.h>
#include <json/writer.h>
#include <json/stringbuffer.h>
#include <json/error/en.h>

#define MAX_TOKEN_SIZE 32
#define MAX_DEVICE_ID_SIZE 12


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace json = rapidjson;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

int main(int argc, char** argv)
{
    try
    {
        if(argc != 2)
        {
            return EXIT_FAILURE;
        }
        std::string host = argv[1];
        auto const port = "4000";
        // auto const text = argv[2];

        net::io_context ioc;
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws(ioc);
        auto const results = resolver.resolve(host, port);
        auto ep = net::connect(ws.next_layer(), results);

        host += ':' + std::to_string(ep.port());

        //websocket handshake 옵션설정
        ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-wmpo");
        }));

        // handshake 요청
        boost::system::error_code ec;
        ws.handshake(host, "/", ec);

        for(;;){ 
            std::stringstream msg;

            char szToken[MAX_TOKEN_SIZE] = "";
            char szDeviceID[MAX_DEVICE_ID_SIZE] = "";
            int i = 0;
            json::Value jAction;
            json::Value jDevice;
            json::Value jToken;
            json::Value jDeviceID;
            json::Value jStoreID;
            json::Value arr_storeID(rapidjson::kArrayType);

            srand((unsigned int)time(NULL));

            for(i = 0; i < MAX_TOKEN_SIZE; i++) {
                szToken[i] = 'a' + rand() % 26 + 1;
                szToken[MAX_TOKEN_SIZE] = '\0';                
            }

            for(i = 0; i < MAX_DEVICE_ID_SIZE; i++) {
                szDeviceID[i] = 'a' + rand() % 26 + 1;
                szDeviceID[MAX_DEVICE_ID_SIZE] = '\0';
            }

            json::StringBuffer buf;
            
            json::Document response_json;
            response_json.SetObject();
            json::Document::AllocatorType& allocator = response_json.GetAllocator();
            
            jDevice.SetString("WEB", 3, allocator);
            jAction.SetString("ORDER", 5, allocator);
            jToken.SetString(szToken, MAX_TOKEN_SIZE, allocator);
            jStoreID.SetString("7451", 4, allocator);
            jDeviceID.SetString(szDeviceID, MAX_DEVICE_ID_SIZE, allocator);
            arr_storeID.PushBack(jStoreID, allocator);

            response_json.AddMember("device", jDevice, allocator);
            response_json.AddMember("action", jAction, allocator);
            response_json.AddMember("token", jToken, allocator);
            response_json.AddMember("storeID", arr_storeID, allocator);
            response_json.AddMember("deviceID", jDeviceID, allocator);
            
            
            buf.Clear();
            json::Writer<json::StringBuffer> writer(buf);
            response_json.Accept(writer);
            msg.clear();
            msg.str("");
            msg << buf.GetString();

            boost::system::error_code error_code;
            // base64 base;
            // std::istringstream in(msg.str());
            // std::ostringstream out(std::ostringstream::out);
            // base.encode(in, out);

            ws.text(ws.got_text());
            // ws.write(net::buffer(out.str()), error_code);
            ws.write(net::buffer(msg.str()), error_code);

            if (error_code) {
                std::cout <<"error : " << error_code.message() << ", (" << error_code << ")";
            }

            beast::flat_buffer buffer;
            ws.read(buffer);

            std::this_thread::sleep_for(std::chrono::minutes(60));
        }
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
