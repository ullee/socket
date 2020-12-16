#ifndef APP_SOCKET_LOG_H
#define APP_SOCKET_LOG_H

#include <iostream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging  = boost::log;
namespace src      = boost::log::sources;
namespace sinks    = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace attrs    = boost::log::attributes;
namespace expr     = boost::log::expressions;

#define LOG_FILE_LEN      (256)
#define LOG_ROTATION_SIZE (1048576)  // 1 * 1024 * 1024
#define LOG_MAX_SIZE      (20971520) // 20 * 1024 * 1024
#define LOG_DIR "/home/httpd/logs"

class Log {
public:
    static void init(const char *app_name);
};

template<typename ValueType>
ValueType set_get_attrib(const char *name, ValueType value) {
    auto attr = logging::attribute_cast<attrs::mutable_constant<ValueType>>(logging::core::get()->get_global_attributes()[name]);
    attr.set(value);
    return attr.get();
}

inline std::string path_to_filename(std::string path) {
    return path.substr(path.find_last_of("/\\")+1);
}

#define log(lvl)\
    BOOST_LOG_STREAM_WITH_PARAMS(::boost::log::trivial::logger::get(),\
    (set_get_attrib("File", path_to_filename(__FILE__))) \
    (set_get_attrib("Line", __LINE__)) \
    (::boost::log::keywords::severity = ::boost::log::trivial::lvl))

#endif // APP_SOCKET_LOG_H