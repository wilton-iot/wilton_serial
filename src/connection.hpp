/* 
 * File:   serial_termios.hpp
 * Author: alex
 *
 * Created on September 12, 2017, 1:15 PM
 */

#ifndef WILTON_SERIAL_CONNECTION_HPP
#define WILTON_SERIAL_CONNECTION_HPP

#include <string>

#include "staticlib/config.hpp"
#include "staticlib/io.hpp"
#include "staticlib/pimpl.hpp"

#include "serial_config.hpp"

namespace wilton {
namespace serial {

class connection : public sl::pimpl::object {
protected:
    /**
     * implementation class
     */
    class impl;

public:
    /**
     * PIMPL-specific constructor
     * 
     * @param pimpl impl object
     */
    PIMPL_CONSTRUCTOR(connection)

    connection(const serial_config& conf);

    std::string read(uint32_t length);

    std::string read_line();

    uint32_t write(sl::io::span<const char> data);
};

} // namespace
}

#endif /* WILTONL_SERIAL_CONNECTION_HPP */

