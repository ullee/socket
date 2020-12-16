#include "Log.h"

void Log::init(const char *app_name)
{
    logging::core::get()->add_global_attribute("File", attrs::mutable_constant<std::string>(""));
    logging::core::get()->add_global_attribute("Line", attrs::mutable_constant<int>(0));

    // Output message to console
    logging::add_console_log(
        std::cout,
        keywords::format = (expr::stream
                            << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << "]"
                            << "[" << expr::attr<attrs::current_thread_id::value_type>("ThreadID") << "]"
                            << "[" << boost::log::trivial::severity << "]"
                            << '[' << expr::attr<std::string>("File")
                            << ':' << expr::attr<int>("Line") << "]: "
                            << expr::smessage),
        keywords::auto_flush = true
    );

    char logfile[LOG_FILE_LEN];
    memset(logfile, 0x00, sizeof(logfile));
    snprintf(logfile, sizeof(logfile), "%s/%s-%s", LOG_DIR, app_name, "%Y-%m-%d.log");

    logging::add_file_log(
        keywords::file_name = logfile,
        keywords::rotation_size = LOG_ROTATION_SIZE,
        keywords::max_size = LOG_MAX_SIZE,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::format = (expr::stream
                            << "[" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f") << "]"
                            << "[" << expr::attr<attrs::current_thread_id::value_type>("ThreadID") << "]"
                            << "[" << boost::log::trivial::severity << "]"
                            << '[' << expr::attr<std::string>("File")
                            << ':' << expr::attr<int>("Line") << "]: "
                            << expr::smessage),
        keywords::auto_flush = true,
        keywords::open_mode = std::ios_base::out | std::ios_base::app
    );

    logging::add_common_attributes();

    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);
}