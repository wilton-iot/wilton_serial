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
 * File:   wilton_serial.cpp
 * Author: alex
 *
 * Created on September 10, 2017, 10:41 AM
 */

#include "wilton/wilton_serial.h"

#include <string>

#include "staticlib/config.hpp"

#include "wilton/support/alloc.hpp"
#include "wilton/support/buffer.hpp"
#include "wilton/support/handle_registry.hpp"
#include "wilton/support/logging.hpp"

#include "connection.hpp"
#include "serial_config.hpp"

namespace { // anonymous

const std::string logger = std::string("wilton.Serial");

} // namespace

struct wilton_Serial {
private:
    wilton::serial::connection ser;

public:
    wilton_Serial(wilton::serial::connection&& ser) :
    ser(std::move(ser)) { }

    wilton::serial::connection& impl() {
        return ser;
    }
};

char* wilton_Serial_open(
        wilton_Serial** ser_out,
        const char* conf,
        int conf_len) /* noexcept */ {
    if (nullptr == ser_out) return wilton::support::alloc_copy(TRACEMSG("Null 'ser_out' parameter specified"));
    if (nullptr == conf) return wilton::support::alloc_copy(TRACEMSG("Null 'conf' parameter specified"));
    if (!sl::support::is_uint16_positive(conf_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'conf_len' parameter specified: [" + sl::support::to_string(conf_len) + "]"));
    try {
        auto conf_json = sl::json::load({conf, conf_len});
        auto sconf = wilton::serial::serial_config(conf_json);
        wilton::support::log_debug(logger, "Opening serial connection, port: [" + sconf.port + "]," +
                " timeout: [" + sl::support::to_string(sconf.timeout_millis) + "] ...");
        auto ser = wilton::serial::connection(std::move(sconf));
        wilton_Serial* ser_ptr = new wilton_Serial(std::move(ser));
        wilton::support::log_debug(logger, "Connection opened, handle: [" + wilton::support::strhandle(ser_ptr) + "]");
        *ser_out = ser_ptr;
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Serial_read(
        wilton_Serial* ser,
        int len,
        char** data_out,
        int* data_len_out) /* noexcept */ {
    if (nullptr == ser) return wilton::support::alloc_copy(TRACEMSG("Null 'ser' parameter specified"));
    if (!sl::support::is_uint32_positive(len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'len' parameter specified: [" + sl::support::to_string(len) + "]"));
    if (nullptr == data_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_out' parameter specified"));
    if (nullptr == data_len_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_len_out' parameter specified"));
    try {
        wilton::support::log_debug(logger, std::string("Reading from serial connection,") +
                " handle: [" + wilton::support::strhandle(ser) + "]," +
                " length: [" + sl::support::to_string(len) + "] ...");
        std::string res = ser->impl().read(static_cast<uint32_t>(len));
        wilton::support::log_debug(logger, std::string("Read operation complete,") +
                " bytes read: [" + sl::support::to_string(res.length()) + "]," +
                " data: [" + sl::io::format_plain_as_hex(res) + "]");
        auto buf = wilton::support::make_string_buffer(res);
        *data_out = buf.data();
        *data_len_out = buf.size_int();
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Serial_readline(
        wilton_Serial* ser,
        char** data_out,
        int* data_len_out) /* noexcept */ {
    if (nullptr == ser) return wilton::support::alloc_copy(TRACEMSG("Null 'ser' parameter specified"));
    if (nullptr == data_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_out' parameter specified"));
    if (nullptr == data_len_out) return wilton::support::alloc_copy(TRACEMSG("Null 'data_len_out' parameter specified"));
    try {
        wilton::support::log_debug(logger, std::string("Reading a line from serial connection,") +
                " handle: [" + wilton::support::strhandle(ser) + "] ...");
        std::string res = ser->impl().read_line();
        wilton::support::log_debug(logger, std::string("Read operation complete,") +
                " bytes read: [" + sl::support::to_string(res.length()) + "]," +
                " data: [" + sl::io::format_plain_as_hex(res) + "]");
        auto buf = wilton::support::make_string_buffer(res);
        *data_out = buf.data();
        *data_len_out = buf.size_int();
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }

}

char* wilton_Serial_write(
        wilton_Serial* ser,
        const char* data,
        int data_len,
        int* len_written_out) /* noexcept */ {
    if (nullptr == ser) return wilton::support::alloc_copy(TRACEMSG("Null 'ser' parameter specified"));
    if (nullptr == data) return wilton::support::alloc_copy(TRACEMSG("Null 'data' parameter specified"));
    if (!sl::support::is_uint32_positive(data_len)) return wilton::support::alloc_copy(TRACEMSG(
            "Invalid 'data_len' parameter specified: [" + sl::support::to_string(data_len) + "]"));
    try {
        auto data_src = sl::io::array_source(data, data_len);
        auto hex_sink = sl::io::string_sink();
        {
            auto sink = sl::io::make_hex_sink(hex_sink);
            sl::io::copy_all(data_src, sink);
        }
        wilton::support::log_debug(logger, std::string("Writing data to serial connection,") +
                " handle: [" + wilton::support::strhandle(ser) + "]," +
                " data: [" + sl::io::format_hex(hex_sink.get_string()) +  "],"
                " data_len: [" + sl::support::to_string(data_len) +  "] ...");
        uint32_t written = ser->impl().write({data, data_len});
        wilton::support::log_debug(logger, std::string("Write operation complete,") +
                " bytes written: [" + sl::support::to_string(written) + "]");
        *len_written_out = static_cast<int>(written);
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Serial_close(
        wilton_Serial* ser) /* noexcept */ {
    if (nullptr == ser) return wilton::support::alloc_copy(TRACEMSG("Null 'ser' parameter specified"));
    try {
        wilton::support::log_debug(logger, "Closing serial connection, handle: [" + wilton::support::strhandle(ser) + "] ...");
        delete ser;
        wilton::support::log_debug(logger, "Connection closed");
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

