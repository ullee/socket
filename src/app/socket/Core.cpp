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

        log(info) << "shutdown socket";
        kill(xsock->pid, SIGKILL);

        if (unlink(xsock->pid_filename) < 0) {
            throw strerror(errno);
        }

    } catch (const std::exception &e) {
        log(error) << e.what();
        return false;
    }
    return true;
}

bool Core::force_terminate(Instance *xsock)
{
    xsock->pid = getpid();

    if (!terminate(xsock)) {
        log(error) << "socket stop fail";
        return false;
    }
    return true;
}

void Core::core_loop(Instance *xsock)
{
    try {
        net::io_context io{1};
        net::signal_set signals(io, SIGINT, SIGTERM);

        //listen for new connection
        //auto const address = net::ip::make_address("127.0.0.1");
        auto const port = static_cast<unsigned short>(3900);

        boost::system::error_code error;

        //tcp::acceptor acceptor_(io, tcp::endpoint(address, port));
        tcp::acceptor acceptor_(io, tcp::endpoint(tcp::v4(), port));
        tcp::acceptor::receive_buffer_size option(SOCKET_BUFFER_MAXSIZE);
        acceptor_.set_option(option, error);

        if (error) {
            log(error) << "code:" << error.value() << " msg:" << error.message();
        }

        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(SOCKET_CONF_PATH, pt);
        xsock->users_limit = pt.get<unsigned int>("Users.limit", 4096);
        log(debug) << "Users.limit : " << xsock->users_limit;

        std::vector<Users> users;

        Session session;
        boost::thread ping_t(boost::bind(&Session::ping, &session, std::ref(users)));
        ping_t.detach();
        log(debug) << "detach ping thread";

        for (;;) {

            std::shared_ptr<tcp::socket> socket_(std::make_shared<tcp::socket>(io));
            
            boost::system::error_code error;
            acceptor_.accept(*socket_, error);
            
            if (error) {
                log(error) << "code:" << error.value() << " msg:" << error.message();
            }

            boost::thread t1(boost::bind(&Session::run, &session, socket_, std::ref(users), std::ref(xsock)));

            t1.join();
            //log(debug) << "join worker thread";
        }


    } catch (const std::exception &e) {
        log(error) << e.what();
    }
}

void Core::read_pidfile(Instance *xsock)
{
    std::ifstream openFile(xsock->pid_filename);
    if (openFile.is_open()) {
        std::string line;
        while (getline(openFile, line)) {
            xsock->pid = boost::lexical_cast<pid_t>(line);
        }
        openFile.close();
        log(info) << "pid[" << xsock->pid << "]";
    }
}
