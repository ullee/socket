#include <iostream>
#include <cstdlib>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <json/document.h>
#include <json/writer.h>
#include <json/stringbuffer.h>
#include <json/error/en.h>

#include "Crypt.h"

#define MAX_TOKEN_SIZE 32
#define MAX_DEVICE_ID_SIZE 12
#define MAX_DEVICE_NAME_SIZE 6


using namespace boost::property_tree;
namespace net = boost::asio;
namespace json = rapidjson;
using tcp = boost::asio::ip::tcp;

void signal_handler(int signum) {
    std::cout << "Caught signal " << signum << std::endl;
    exit(signum);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "agrument not enough" << std::endl;
        exit(-1);
    }

    // std::string argument = argv[1];
    // std::string ipaddr   = "127.0.0.1";
    std::string ipaddr   = argv[1];

    // if (argv[2]) {
    //     ipaddr = argv[2];
    // }

    int conn_count = 0;

    net::io_context io;
    tcp::socket socket(io);

    try {
        socket.connect(tcp::endpoint(net::ip::address::from_string(ipaddr), 3900));

    } catch (const std::exception &e) {
        std::cerr << "ERROR:" << e.what() << std::endl;
        ++conn_count;
        sleep(1);
        // continue;
    }

    for (;;) {

        // if (conn_count >= 5) {
        //     conn_count = 0;
        //     sleep(5);
        // }

        // net::io_context io;
        // tcp::socket socket(io);

        // try {
        //     socket.connect(tcp::endpoint(net::ip::address::from_string(ipaddr), 3900));

        // } catch (const std::exception &e) {
        //     std::cerr << "ERROR:" << e.what() << std::endl;
        //     ++conn_count;
        //     sleep(1);
        //     continue;
        // }

        // for (;;) {
            // init
            std::stringstream msg;
            // boost::property_tree::ptree init_ptree;

            // if (argument.compare("1") == 0) {
            //     init_ptree.put("action", "INIT");
            //     init_ptree.put("token", "MjBPYVE0VkhxcHVMWDM4YytRbzY2QT09");
            //     init_ptree.put("deviceID", "0205857FEB80");
            //     init_ptree.put("deviceName", "DESKTOP-T4V4AJ7");
            // } else {
            //     init_ptree.put("action", "INIT");
            //     init_ptree.put("token", "MjBPYVE0VkhxcHVMWDM4YytRbzY2QT" + argument);
            //     init_ptree.put("deviceID", "0205857FEB" + argument);
            //     init_ptree.put("deviceName", "DESKTOP-T4V" + argument);
            // }

            // boost::property_tree::json_parser::write_json(msg, init_ptree, false);

            // std::vector<int> a;
            // std::vector<int>::iterator iterA;
            // a.push_back(1);
            // a.push_back(3);
            // a.push_back(2);

            // std::array<int, 3> b = {3,1,2};

            // // for(int i = 0; i < b.size(); i++) {
            // //     b.push_back(b[i]);
            // // }

            // std::string sA = "";
            // std::string sB = "";

            // for(unsigned int i = 0; i < a.size(); i++) {
            //     sA.append(std::to_string(a[i]));
            // }

            // for(unsigned int i = 0; i < b.size(); i++) {
            //     sB.append(std::to_string(b[i]));
            // }

            // std::cout << "sA : " << sA << std::endl;
            // std::cout << "sB : " << sB << std::endl;

            // if(sA.compare(sB)==0) {
            //     std::cout << "success" << std::endl;
            // } else {
            //     std::cout << "fail" << std::endl;
            // }

            char szToken[MAX_TOKEN_SIZE];
            char szDeviceID[MAX_DEVICE_ID_SIZE];
            char szDeviceName[MAX_DEVICE_NAME_SIZE];
            memset (szToken, 0x00, MAX_TOKEN_SIZE);
            memset (szDeviceID, 0x00, MAX_DEVICE_ID_SIZE);
            memset (szDeviceName, 0x00, MAX_DEVICE_NAME_SIZE);
            int i = 0;
            json::Value jAction;
            json::Value jToken;
            json::Value jDeviceID;
            json::Value jDeviceName;
            json::Value jStoreID;

            srand((unsigned int)time(NULL));

            for(i = 0; i < MAX_TOKEN_SIZE; i++) {
                szToken[i] = 'a' + rand() % 26 + 1;
                // szToken[MAX_TOKEN_SIZE] = '\0';                
            }

            for(i = 0; i < MAX_DEVICE_ID_SIZE; i++) {
                szDeviceID[i] = 'a' + rand() % 26 + 1;
                // szDeviceID[MAX_DEVICE_ID_SIZE] = '\0';
            }

            for(i = 0; i < MAX_DEVICE_NAME_SIZE; i++) {
                szDeviceName[i] = 'a' + rand() % 26 + 1;
                // szDeviceName[MAX_DEVICE_NAME_SIZE] = '\0';
            }

            // encrypt send message
            json::StringBuffer buf;
            
            json::Document response_json;
            json::Document::AllocatorType& allocator = response_json.GetAllocator();
            
            // INIT
            jAction.SetString("INIT", 4, allocator);
            jToken.SetString(szToken, MAX_TOKEN_SIZE, allocator);
            jDeviceID.SetString(szDeviceID, MAX_DEVICE_ID_SIZE, allocator);
            jDeviceName.SetString(szDeviceName, MAX_DEVICE_NAME_SIZE, allocator);

            response_json.StartObject();
            response_json.AddMember("action", jAction, allocator);
            response_json.AddMember("token", jToken, allocator);
            response_json.AddMember("deviceID", jDeviceID, allocator);
            response_json.AddMember("deviceName", jDeviceName, allocator);
            response_json.EndObject(4);
            
            buf.Clear();
            json::Writer<json::StringBuffer> writer(buf);
            response_json.Accept(writer);
            msg.clear();
            msg.str("");
            msg << buf.GetString();

            Crypt c(1);
            std::string request_init = c.encrypt_base64(msg.str());
            std::cout << "msg : " << msg.str() << ", enc : " << request_init;
            boost::system::error_code error;
            net::write(socket, net::buffer(request_init), error);

            std::this_thread::sleep_for(std::chrono::seconds(5));

            // CHOICE
            request_init = "";
            jAction.SetString("STORE_CHOICE", sizeof("STORE_CHOICE"), allocator);
            response_json.StartObject();
            response_json.RemoveAllMembers();
            response_json.AddMember("action", jAction, allocator);
            response_json.AddMember("token", jToken, allocator);
            response_json.AddMember("storeID", jStoreID, allocator);
            response_json.AddMember("deviceID", jDeviceID, allocator);
            response_json.AddMember("deviceName", jDeviceName, allocator);
            response_json.EndObject(5);

            buf.Clear();
            writer.Reset(buf);
            response_json.Accept(writer);
            msg.clear();
            msg.str("");
            msg << buf.GetString();
            
            request_init = c.encrypt_base64(msg.str());
            std::cout << "msg : " << msg.str() << ", enc : " << request_init;

            size_t bytes = 0 ;

            bytes = net::write(socket, net::buffer(request_init), error);

            std::this_thread::sleep_for(std::chrono::seconds(5));
            if(error && bytes == 0) {
                std::cout << "error : " << error.message() << ", code : " << error.value();
            }

            // ACTIVE
            request_init = "";
            jAction.SetString("ACTIVE", sizeof("ACTIVE"), allocator);
            response_json.StartObject();
            response_json.RemoveAllMembers();
            response_json.AddMember("action", jAction, allocator);
            response_json.AddMember("token", jToken, allocator);
            response_json.AddMember("storeID", jStoreID, allocator);
            response_json.AddMember("deviceID", jDeviceID, allocator);
            response_json.AddMember("deviceName", jDeviceName, allocator);
            response_json.EndObject(5);

            buf.Clear();
            writer.Reset(buf);
            response_json.Accept(writer);
            msg.clear();
            msg.str("");
            msg << buf.GetString();
            
            request_init= c.encrypt_base64(msg.str());
            std::cout << "msg : " << msg.str() << ", enc : " << request_init;

            bytes = 0 ;

            bytes = net::write(socket, net::buffer(request_init), error);

            if(error && bytes == 0) {
                std::cout << "error : " << error.message() << ", code : " << error.value();
            }

            std::this_thread::sleep_for(std::chrono::seconds(30));

            // if (error) {
            //     if (error != net::error::eof) {
            //         std::cerr << " line(" << __LINE__ << ")" << ": " << error.message() << std::endl;
            //         sleep(1);
            //         break;
            //     } else {
            //         std::cerr << " line(" << __LINE__ << ")" << ": " << error.message() << std::endl;
            //         sleep(1);
            //         break;
            //     }
            // }

            // char buffer[10240] = {0,};

            // size_t n = net::read(socket, net::buffer(buffer), net::transfer_at_least(1), error);
            
            // std::string tmp = buffer;
            // std::string enc_data = tmp.substr(0, n);

            // if (error) {
            //     if (error == net::error::eof) {
            //         std::cerr << " line(" << __LINE__ << ")" << " code:" << error.value() << " msg:" << error.message() << std::endl;
            //         sleep(1);
            //         break;
            //     } else {
            //         std::cerr << " line(" << __LINE__ << ")" << " code:" << error.value() << " msg:" << error.message() << std::endl;
            //         sleep(1);
            //         break;
            //     }
            // }

            // if (enc_data.compare("ping_120000") == 0) {
            //     std::cout << "ping ok!" << std::endl;
            //     std::cout << enc_data << std::endl;
            // } else {
            //     std::string json_data = c.decrypt_base64(enc_data);
                
            //     std::stringstream json_stream(json_data);

            //     std::cout << "encrypt_data    [" << enc_data << "]" << std::endl;
            //     std::cout << "decrypt_data    [" << json_data << "]" << std::endl;
            //     std::cout << "buff_stream_size[" << json_stream.str().length() << "]" << std::endl;

            //     boost::property_tree::ptree request_ptree;
            //     boost::property_tree::read_json(json_stream, request_ptree);

            //     if (request_ptree.get<std::string>("action", "").compare("ORDER") == 0) {
            //         std::cout << "주문이 들어왔습니다." << std::endl;
            //     } else if (request_ptree.get<std::string>("action", "").compare("PRINT") == 0) {
            //         std::cout << "주문서 출력" << std::endl;
            //         std::cout << json_data << std::endl;
            //     } else if (request_ptree.get<std::string>("action", "").compare("ACTIVE") == 0) {
            //         std::cout << "설정버튼 활성화" << std::endl;
            //         std::cout << json_data << std::endl;
            //     } else {
            //         // do nothing.
            //     }

            //     json_stream.str("");
            //     json_stream.clear();
            // }
        // }
    }
    std::cout << "socket close" << std::endl;

    return EXIT_SUCCESS;
}