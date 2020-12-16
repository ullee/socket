#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <fstream>

#include "Core.h"

class Daemonize {
private:
    pid_t   pid, sid;
    int     fd, pid_len, no_daemonize;
    char    pid_str[LD_UINTMAX_MAXLEN];
    ssize_t n;

public:
    Daemonize(int no_daemonize) {
        this->no_daemonize = no_daemonize;
    };
    ~Daemonize() {};
    int32_t pre_run(Instance *xsock)
    {
        int32_t rstatus;
        if (this->no_daemonize != 1) {
            rstatus = make_daemonize(1);
            if (rstatus != 0) {
                return rstatus;
            }
        }

        xsock->pid = getpid();

        if (xsock->pid_filename) {
            rstatus = create_pidfile(xsock);
            if (rstatus != 0) {
                return rstatus;
            }
        }

        print_run(xsock);

        return 0;
    }

    void post_run(Instance *xsock)
    {
        if (xsock->pidfile) {
            remove_pidfile(xsock);
        }

        log(info) << "shutdown WebSocketServer";

        return;
    }

    int32_t make_daemonize(int dump_core)
    {
        int32_t rstatus;
        log(info) << __FUNCTION__ << "make parent daemon";
        this->pid = fork();
        switch (this->pid) {
            case -1:
                cout << "fork() failed: " << strerror(errno) << endl;
                return -1;
            case 0:
                break;
            default:
                /* parent terminates */
                _exit(0);
        }

        log(info) << __FUNCTION__ << "make child process";
        /* child continues and becomes the session leader */
        this->sid = setsid();
        if (this->sid < 0) {
            log(error) << "setsid() failed: " << strerror(errno);
            return -1;
        }

        if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
            log(error) << "signal(SIGHUB, SIG_IGN) failed: " << strerror(errno);
            return -1;
        }

        this->pid = fork();
        switch (this->pid) {
            case -1:
                log(error) << "fork() failed: " << strerror(errno);
                return -1;
            case 0:
                break;
            default:
                /* child terminates */
                _exit(0);
        }

        /* child continues */

        /* change working directory */
        if (dump_core == 0) {
            rstatus = chdir("/");
            if (rstatus < 0) {
                log(error) << "chdir(\"/\") failed: " << strerror(errno);
                return -1;
            }
        }

        /* clear file mode creation mask 0666 */
        umask(0);

        this->fd = open("/dev/null", O_RDWR);
        if (this->fd < 0) {
            log(error) << "open(\"/dev/null\") failed: " << strerror(errno);
            return -1;
        }

        rstatus = dup2(this->fd, STDIN_FILENO);
        if (rstatus < 0) {
            log(error) << "dup2(" << this->fd << ", STDIN) failed: " << strerror(errno);
            close(this->fd);
            return -1;
        }

        rstatus = dup2(this->fd, STDOUT_FILENO);
        if (rstatus < 0) {
            log(error) << "dup2(" << this->fd << ", STDOUT) failed: " << strerror(errno);
            close(this->fd);
            return -1;
        }

        rstatus = dup2(this->fd, STDERR_FILENO);
        if (rstatus < 0) {
            log(error) << "dup2(" << this->fd << ", STDERR) failed: " << strerror(errno);
            close(this->fd);
            return -1;
        }

        if (this->fd > STDERR_FILENO) {
            rstatus = close(this->fd);
            if (rstatus < 0) {
                log(error) << "close("<< this->fd << ") failed: " <<  strerror(errno);
                return -1;
            }
        }

        return 0;
    }

    int32_t create_pidfile(Instance *xsock)
    {
        log(info) << "create pidfile";
        this->fd = open(xsock->pid_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (this->fd < 0) {
            log(error) << "opening pid file '"<< xsock->pid_filename << "' failed: " << strerror(errno);
            return -1;
        }
        xsock->pidfile = 1;

        this->pid_len = snprintf(this->pid_str, LD_UINTMAX_MAXLEN, "%d", xsock->pid);

        this->n = write(this->fd, this->pid_str, this->pid_len);
        if (this->n < 0) {
            log(error) << "write to pid file " << xsock->pid_filename << " failed: " << strerror(errno);
            return -1;
        }

        close(this->fd);

        return 0;
    }

    void remove_pidfile(Instance *xsock)
    {
        int32_t rstatus;
        rstatus = unlink(xsock->pid_filename);
        if (rstatus < 0) {
		    log(info) << "unlink of pid file '" << xsock->pid_filename << "' failed, ignored: " << strerror(errno);
        }
	}

    bool check_pidfile(Instance *xsock)
    {
        return (access(xsock->pid_filename, F_OK) != -1);
    }

    void print_run(Instance *xsock)
    {
        int32_t rstatus;
        struct utsname name;

        rstatus = uname(&name);
        
        if (rstatus < 0) {
		    log(info) << "websocket - started on pid " << xsock->pid;
	    } else {
		    log(info) << "websocket - "<< COMPILE_DATE << " " << COMPILE_TIME << " built for " <<
				name.sysname << " " << name.release << " " << name.machine << " started on pid " << xsock->pid;
	    }
        log(info) << "websocket pid filename[" <<  xsock->pid_filename << "]";
    }

    void run(Instance *xsock)
    {
        xsock->pid = getpid();

        try {
            Core core;
            core.core_loop(xsock);
        } catch (exception &e) {
            log(error) << __FUNCTION__ << " :" << e.what();
        }
    }

};

int main(int argc, char **argv)
{
    Instance xsock;
    Base base;
    base.set_default_options(&xsock);

    if (!base.get_options(argc, argv, &xsock)) {
        base.show_usage();
        exit(1);
	}

	if (base.show_version) {
        cout << "websocket built: " << COMPILE_DATE << " " << COMPILE_TIME << endl;
        cout << "websocket version: " << SOCKET_VERSION << endl;
        if (base.show_help) {
            base.show_usage();
        }
        exit(0);
    }

    // init logger
    Log::init(DAEMON_NAME);

    Core core;
    if (base.stop) {
        core.force_terminate(&xsock);
        exit(1);
    }

    log(info) << "init daemon";

    Daemonize daemon(base.no_daemonize);

    if (daemon.check_pidfile(&xsock)) {
        log(error) << "process is running";
        exit(1);
    }

    int32_t rstatus = daemon.pre_run(&xsock);
    if (rstatus != 0) {
        daemon.post_run(&xsock);
        exit(1);
    }

    daemon.run(&xsock);
    daemon.post_run(&xsock);

    exit(1);
    return 0;
}
