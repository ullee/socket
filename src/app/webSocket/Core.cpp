#include <csignal>
#include <dirent.h>

#include "Core.h"

bool Core::terminate(Instance *xsock)
{
    try {
        struct passwd *user_pw;
        user_pw = getpwnam("root");
        xsock->uid = user_pw->pw_uid;

        read_pidfile(xsock);

        log(info) << "shutdown websocket";
        kill(xsock->pid, SIGKILL);

        if (unlink(xsock->pid_filename) < 0) {
            throw strerror(errno);
        }

    } catch (std::exception &e) {
        log(error) << __FUNCTION__ << " line(" << __LINE__ << ")" << ": " << e.what();
        return false;
    }
    return true;
}

bool Core::force_terminate(Instance *xsock)
{
    xsock->pid = getpid();

    if (!terminate(xsock)) {
        log(error) << "websocket stop fail";
        return false;
    }
    return true;
}

void Core::core_loop(Instance *xsock)
{
    try
    {
        Session session;
        std::vector<Users> users;
        boost::system::error_code error_code;
        boost::property_tree::ptree configPtree;

        auto const port = static_cast<unsigned short>(4000);
        net::io_context ioc{1};
        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), port));
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), error_code);
        if(error_code) {
            log(error) << "socket acceptor set_option fail. message : " << error_code.message() << ", value : " << error_code.value();
        }
 
        boost::property_tree::ini_parser::read_ini(SOCKET_CONF_PATH, configPtree);        
        const unsigned int userLimit = configPtree.get<unsigned int>("Users.Limit");
        log(info) << "websocket User Limit : " << userLimit;
        
        boost::thread t2 = boost::thread(std::bind(&Session::ping, &session, std::ref(users)));
        t2.detach();

        while(1)
        {
            tcp::socket socket{ioc};
            acceptor.accept(socket, error_code);    // socket accept

            if(error_code) {
                log(error) << "socket accept error : " << error_code.message() << "(" << error_code << ")";
                socket.close();
                continue;
            }

            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket, buffer, req, error_code);
            buffer.clear();

            if(error_code) {
                if(error_code == http::error::end_of_stream) {
                    log(debug) << "socket read : " << net::buffer_cast<const char *>(buffer.data()) << ", error_code :" <<error_code.message() << "(" << error_code << ")";
                    socket.close();
                    continue;    
                } else {
                    log(error) << "socket read error message : " << error_code.message() << "(" << error_code << ")";
                    socket.close();
                    continue;
                }
            }

            std::shared_ptr<websocket::stream<tcp::socket>> websocket_(std::make_shared<websocket::stream<tcp::socket>>(std::move(socket)));        // webSocket 생성

            if(websocket::is_upgrade(req)) {
                websocket_->set_option(websocket::stream_base::decorator(              // websocket handshake를 위한 header 설정
                [](websocket::response_type& res)
                {
                    res.set(http::field::server, std::string("websocket-server-wmpo"));
                }));
                
                websocket_->accept(req, error_code);     // websocket accept

                if(error_code) {
                    log(info) << "web accept error : " << error_code.message() << "(" << error_code << ")";
                    websocket_->close(websocket::close_code::normal);
                    socket.close();
                    continue;
                }
            }
            
            boost::thread t1 = boost::thread(std::bind(&Session::run, &session, websocket_, std::ref(users), userLimit));

            t1.join();
        }
    }
    catch (const std::exception& e)
    {
        log(error) << __FUNCTION__ << " line(" << __LINE__ << ")" << ": " << e.what();
    }
}

void Core::read_pidfile(Instance *xsock)
{
    std::ifstream openFile(SOCKET_PID_FILE);
    if (openFile.is_open()) {
        string line;
        while (getline(openFile, line)) {
            xsock->pid = boost::lexical_cast<pid_t>(line);
        }
        openFile.close();
        log(info) << "pid[" << xsock->pid << "]";
    }
}
