/* 
 * File:   wiltoncall_serial.cpp
 * Author: alex
 *
 * Created on September 10, 2017, 10:41 AM
 */


#include <string>

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/json.hpp"
#include "staticlib/support.hpp"

#include "wilton/wilton.h"
#include "wilton/wiltoncall.h"
#include "wilton/wilton_serial.h"

#include "wilton/support/handle_registry.hpp"
#include "wilton/support/buffer.hpp"
#include "wilton/support/registrar.hpp"

namespace wilton {
namespace serial {

namespace { //anonymous

support::handle_registry<wilton_Serial>& static_registry() {
    static support::handle_registry<wilton_Serial> registry {
        [] (wilton_Serial* conn) STATICLIB_NOEXCEPT {
            wilton_Serial_close(conn);
        }};
    return registry;
}

} // namespace


support::buffer serial_open(sl::io::span<const char> data) {
    (void) data;
    return support::make_empty_buffer();
}

support::buffer serial_close(sl::io::span<const char> data) {
    (void) data;
    return support::make_empty_buffer();
}

support::buffer serial_read(sl::io::span<const char> data) {
    (void) data;
    return support::make_empty_buffer();
}

support::buffer serial_readline(sl::io::span<const char> data) {
    (void) data;
    return support::make_empty_buffer();
}

support::buffer serial_write(sl::io::span<const char> data) {
    (void) data;
    return support::make_empty_buffer();
}

} // namespace
}

extern "C" char* wilton_module_init() {
    try {
        wilton::support::register_wiltoncall("serial_open", wilton::serial::serial_open);
        wilton::support::register_wiltoncall("serial_close", wilton::serial::serial_close);
        wilton::support::register_wiltoncall("serial_read", wilton::serial::serial_read);
        wilton::support::register_wiltoncall("serial_readline", wilton::serial::serial_readline);
        wilton::support::register_wiltoncall("serial_write", wilton::serial::serial_write);
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}
