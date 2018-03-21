/*
 * Copyright 2017, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

#include "wilton/support/exception.hpp"

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
    uint32_t timeout_millis = 500;

    serial_config(const serial_config&) = delete;

    serial_config& operator=(const serial_config&) = delete;

    serial_config(serial_config&& other) :
    port(std::move(other.port)),
    baud_rate(other.baud_rate),
    parity(other.parity),
    byte_size(other.byte_size),
    stop_bits_count(other.stop_bits_count),
    timeout_millis(other.timeout_millis) { }

    serial_config& operator=(serial_config&& other) {
        port = std::move(other.port);
        baud_rate = other.baud_rate;
        parity = other.parity;
        byte_size = other.byte_size;
        stop_bits_count = other.stop_bits_count;
        timeout_millis = other.timeout_millis;
        return *this;
    }

    serial_config() { }
    
    serial_config(const sl::json::value& json) {
        for (const sl::json::field& fi : json.as_object()) {
            auto& name = fi.name();
            if ("port" == name) {
                this->port = fi.as_string_nonempty_or_throw(name);
            } else if ("baudRate" == name) {
                this->baud_rate = fi.as_uint32_positive_or_throw(name);
            } else if ("parity" == name) {
                this->parity = make_parity_type(fi.as_string_nonempty_or_throw(name));
            } else if ("byteSize" == name) {
                this->byte_size = fi.as_uint16_positive_or_throw(name);
            } else if ("stopBitsCount" == name) {
                this->stop_bits_count = fi.as_uint16_positive_or_throw(name);
            } else if ("timeoutMillis" == name) {
                this->timeout_millis = fi.as_uint32_positive_or_throw(name);
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
            { "timeoutMillis", timeout_millis },
        };
    }
};


} // namespace
}


#endif /* WILTON_SERIAL_SERIAL_CONFIG_HPP */

