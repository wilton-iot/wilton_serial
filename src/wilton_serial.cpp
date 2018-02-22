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

#include "connection.hpp"
#include "serial_config.hpp"

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
        auto ser = wilton::serial::connection(std::move(sconf));
        wilton_Serial* ser_ptr = new wilton_Serial(std::move(ser));
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
        std::string res = ser->impl().read(static_cast<uint32_t>(len));
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
        std::string res = ser->impl().read_line();
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
        uint32_t written = ser->impl().write({data, data_len});
        *len_written_out = static_cast<int>(written);
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}

char* wilton_Serial_close(
        wilton_Serial* ser) /* noexcept */ {
    if (nullptr == ser) return wilton::support::alloc_copy(TRACEMSG("Null 'ser' parameter specified"));
    delete ser;
    return nullptr;
}

