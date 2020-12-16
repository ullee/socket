#include "Base.h"

static struct option long_options[] = {
	{"help",			no_argument,		NULL,	'h'},
	{"version",			no_argument,		NULL,	'v'},
	{"show-conf",		no_argument,		NULL,	'c'},
	{"reload-conf",		no_argument,		NULL,	'r'},
	{"no-daemonize",	no_argument,		NULL,	'n'},
	{"stop",			no_argument,		NULL,	's'},
	{NULL,				0,					NULL,	 0 }
};

static char short_options[] = "hvcrns";

void Base::set_default_options(Instance *xsock)
{
	//xsock->ctx = NULL;
	xsock->conf_filename = SOCKET_CONF_PATH;
	xsock->pid = (pid_t)-1;
	xsock->pid_filename = SOCKET_PID_FILE;
	xsock->pidfile = 1;
}

bool Base::get_options(int argc, char **argv, Instance *xsock)
{
	int c;

	opterr = 0;

	for (;;) {
		c = getopt_long(argc, argv, short_options, long_options, NULL);
		if (c == -1) {
			/* no more options */
			break;
		}

		switch (c) {
			case 'h':
				show_version = 1;
				show_help = 1;
				break;
			
			case 'v':
				show_version = 1;
				break;

			case 'c':
				show_conf = 1;
				break;

			case 'r':
				reload_conf = 1;
				break;

			case 'n':
				no_daemonize = 1;
				break;

			case 's':
				stop = 1;
				break;

			default:
				std::cerr << "socket : invalid option -- '"<< optopt << "'" << endl;
				return false;
		}
	}
	return true;
}

void Base::show_usage()
{
	std::cerr << "Usage: socket [-hvcrns]" << endl;
	std::cerr << "Options:" << endl;
	std::cerr << "  -h, --help             : this help" << endl;
	std::cerr << "  -v, --version          : show version and exit" << endl;
	std::cerr << "  -c, --show-conf        : show config and exit" << endl;
	std::cerr << "  -r, --reload-conf      : reload config and exit" << endl;
	std::cerr << "  -n, --no-daemonize     : run as a no daemon" << endl;
	std::cerr << "  -s, --stop             : stop" << endl;
}