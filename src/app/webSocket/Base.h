#ifndef APP_SOCKET_BASE_H
#define APP_SOCKET_BASE_H

#include <iostream>
#include <string>
#include <getopt.h>

#include "Log.h"
#include "Constants.h"

#define CONF_DIR "/home/httpd/conf"
#define LOG_DIR "/home/httpd/logs"
#define PID_DIR "/home/httpd/pid"
#define SOCKET_CONF_PATH CONF_DIR "/websocket.ini"
#define SOCKET_LOG_FILE LOG_DIR "/websocket"
#define SOCKET_PID_FILE PID_DIR "/websocket.pid"

using namespace std;

class Instance {
public:
    pid_t      pid;
    int32_t    uid;
    const char *pid_filename;
    unsigned   pidfile:1;
    const char *conf_filename;
};

class Base : public Instance {
public:
    Base() {};
    ~Base() {};
    int show_help    = 0;
    int show_version = 0;
    int show_conf    = 0;
    int reload_conf  = 0;
    int no_daemonize = 0;
    int stop         = 0;
    void set_default_options(Instance *xsock);
    bool get_options(int argc, char **argv, Instance *xsock);
    void show_usage();
};

#endif //APP_SOCKET_BASE_H