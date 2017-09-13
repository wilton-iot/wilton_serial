/* 
 * File:   serial_config.hpp
 * Author: alex
 *
 * Created on September 11, 2017, 11:23 PM
 */

#ifndef WILTON_SERIAL_SERIAL_CONFIG_HPP
#define WILTON_SERIAL_SERIAL_CONFIG_HPP

#include <cstdint>
#include <string>

#include "staticlib/config.hpp"
#include "staticlib/support.hpp"
#include "staticlib/json.hpp"

#include "parity_type.hpp"

namespace wilton {
namespace serial {

class serial_config {
public:
    std::string port;
    uint32_t baud_rate = 9600;
    parity_type parity = parity_type::none;
    uint16_t byte_size = 8;
    uint16_t stop_bits_count = 1;
    uint32_t read_timeout_millis = 500;
    uint32_t write_timeout_millis = 500;

    serial_config(const serial_config&) = delete;

    serial_config& operator=(const serial_config&) = delete;

    serial_config(serial_config&& other) :
    port(std::move(other.port)),
    baud_rate(other.baud_rate),
    parity(other.parity),
    byte_size(other.byte_size),
    stop_bits_count(other.stop_bits_count),
    read_timeout_millis(other.read_timeout_millis),
    write_timeout_millis(other.write_timeout_millis) { }

    serial_config& operator=(serial_config&& other) {
        port = std::move(other.port);
        baud_rate = other.baud_rate;
        parity = other.parity;
        byte_size = other.byte_size;
        stop_bits_count = other.stop_bits_count;
        read_timeout_millis = other.read_timeout_millis;
        write_timeout_millis = other.write_timeout_millis;
        return *this;
    }

    serial_config() { }
    
    serial_config(const sl::json::value& json) {
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("port" == name) {
                this->port = fi.as_string_nonempty_or_throw(name);
            } else if ("baudRate" == name) {
                this->baud_rate = fi.as_uint32_or_throw(name);
            } else if ("parity" == name) {
                this->parity = make_parity_type(fi.as_string_nonempty_or_throw(name));
            } else if ("byteSize" == name) {
                this->byte_size = fi.as_uint16_or_throw(name);
            } else if ("stopBitsCount" == name) {
                this->stop_bits_count = fi.as_uint16_or_throw(name);
            } else if ("readTimeoutMillis" == name) {
                this->read_timeout_millis = fi.as_uint32_or_throw(name);
            } else if ("writeTimeoutMillis" == name) {
                this->write_timeout_millis = fi.as_uint32_or_throw(name);
            } else {
                throw support::exception(TRACEMSG("Unknown 'serial_config' field: [" + name + "]"));
            }
        }
        if (port.empty()) throw support::exception(TRACEMSG(
                "Invalid 'serial.port' field: []"));
    }

    sl::json::value to_json() const {
        return {
            { "port", port },
            { "baudRate", baud_rate },
            { "parity", stringify_parity_type(parity) },
            { "byteSize", byte_size },
            { "stopBitsCount", stop_bits_count },
            { "readTimeoutMillis", read_timeout_millis },
            { "writeTimeoutMillis", write_timeout_millis }
        };
    }
};


} // namespace
}


#endif /* WILTON_SERIAL_SERIAL_CONFIG_HPP */

