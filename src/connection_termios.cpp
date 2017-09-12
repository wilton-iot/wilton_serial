/* 
 * File:   connection_termios.cpp
 * Author: alex
 *
 * Created on September 12, 2017, 2:10 PM
 */


#include "connection.hpp"

#include "staticlib/pimpl/forward_macros.hpp"

namespace wilton {
namespace serial {

class connection::impl : public staticlib::pimpl::object::impl {
    
public:
    impl(const serial_config& conf) {
        (void) conf;
    }

    ~impl() STATICLIB_NOEXCEPT {};
    
    std::string read(connection&, uint32_t length) {
        (void) length;
        return "";
    }

    std::string read_line(connection&) {
        return "";
    }

    uint32_t write(connection&, sl::io::span<const char> data) {
        (void) data;
        return 0;
    }
};
PIMPL_FORWARD_CONSTRUCTOR(connection, (const serial_config&), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read, (uint32_t), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read_line, (), (), support::exception)
PIMPL_FORWARD_METHOD(connection, uint32_t, write, (sl::io::span<const char>), (), support::exception)

} // namespace
}
