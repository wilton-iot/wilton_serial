/* 
 * File:   wiltoncall_serial.cpp
 * Author: alex
 *
 * Created on September 10, 2017, 10:41 AM
 */


#include <memory>
#include <string>

#include "staticlib/config.hpp"
#include "staticlib/crypto.hpp"
#include "staticlib/io.hpp"
#include "staticlib/json.hpp"
#include "staticlib/support.hpp"
#include "staticlib/utils.hpp"

#include "wilton/wilton.h"
#include "wilton/wiltoncall.h"
#include "wilton/wilton_serial.h"

#include "wilton/support/handle_registry.hpp"
#include "wilton/support/buffer.hpp"
#include "wilton/support/registrar.hpp"

namespace wilton {
namespace serial {

namespace { //anonymous

std::shared_ptr<support::handle_registry<wilton_Serial>> shared_registry() {
    static auto registry = std::make_shared<support::handle_registry<wilton_Serial>>(
        [] (wilton_Serial* conn) STATICLIB_NOEXCEPT {
            wilton_Serial_close(conn);
        });
    return registry;
}

} // namespace

support::buffer open(sl::io::span<const char> data) {
    wilton_Serial* ser = nullptr;
    char* err = wilton_Serial_open(std::addressof(ser), data.data(), static_cast<int>(data.size()));
    if (nullptr != err) support::throw_wilton_error(err, TRACEMSG(err));
    auto reg = shared_registry();
    int64_t handle = reg->put(ser);
    return support::make_json_buffer({
        { "serialHandle", handle}
    });
}

support::buffer close(sl::io::span<const char> data) {
    // json parse
    auto json = sl::json::load(data);
    int64_t handle = -1;
    for (const sl::json::field& fi : json.as_object()) {
        auto& name = fi.name();
        if ("serialHandle" == name) {
            handle = fi.as_int64_or_throw(name);
        } else {
            throw support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw support::exception(TRACEMSG(
            "Required parameter 'serialHandle' not specified"));
    // get handle
    auto reg = shared_registry();
    wilton_Serial* ser = reg->remove(handle);
    if (nullptr == ser) throw support::exception(TRACEMSG(
            "Invalid 'serialHandle' parameter specified"));
    // call wilton
    char* err = wilton_Serial_close(ser);
    if (nullptr != err) {
        reg->put(ser);
        support::throw_wilton_error(err, TRACEMSG(err));
    }
    return support::make_empty_buffer();
}

support::buffer read(sl::io::span<const char> data) {
    // json parse
    auto json = sl::json::load(data);
    int64_t handle = -1;
    int64_t len = -1;
    for (const sl::json::field& fi : json.as_object()) {
        auto& name = fi.name();
        if ("serialHandle" == name) {
            handle = fi.as_int64_or_throw(name);
        } else if ("length" == name) {
            len = fi.as_int64_or_throw(name);
        } else {
            throw support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw support::exception(TRACEMSG(
            "Required parameter 'serialHandle' not specified"));
    if (-1 == len) throw support::exception(TRACEMSG(
            "Required parameter 'length' not specified"));
    // get handle
    auto reg = shared_registry();
    wilton_Serial* ser = reg->remove(handle);
    if (nullptr == ser) throw support::exception(TRACEMSG(
            "Invalid 'serialHandle' parameter specified"));
    // call wilton
    char* out = nullptr;
    int out_len = 0;
    char* err = wilton_Serial_read(ser, static_cast<int>(len),
            std::addressof(out), std::addressof(out_len));
    reg->put(ser);
    if (nullptr != err) support::throw_wilton_error(err, TRACEMSG(err));
    return support::wrap_wilton_buffer(out, out_len);
}

support::buffer readline(sl::io::span<const char> data) {
    // json parse
    auto json = sl::json::load(data);
    int64_t handle = -1;
    for (const sl::json::field& fi : json.as_object()) {
        auto& name = fi.name();
        if ("serialHandle" == name) {
            handle = fi.as_int64_or_throw(name);
        } else {
            throw support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw support::exception(TRACEMSG(
            "Required parameter 'serialHandle' not specified"));
    // get handle
    auto reg = shared_registry();
    wilton_Serial* ser = reg->remove(handle);
    if (nullptr == ser) throw support::exception(TRACEMSG(
            "Invalid 'serialHandle' parameter specified"));
    // call wilton
    char* out = nullptr;
    int out_len = 0;
    char* err = wilton_Serial_readline(ser, std::addressof(out), std::addressof(out_len));
    reg->put(ser);
    if (nullptr != err) support::throw_wilton_error(err, TRACEMSG(err));
    return support::wrap_wilton_buffer(out, out_len);
}

support::buffer write(sl::io::span<const char> data) {
    // json parse
    auto json = sl::json::load(data);
    int64_t handle = -1;
    auto rdatahex = std::ref(sl::utils::empty_string());
    for (const sl::json::field& fi : json.as_object()) {
        auto& name = fi.name();
        if ("serialHandle" == name) {
            handle = fi.as_int64_or_throw(name);
        } else if ("dataHex" == name) {
            rdatahex = fi.as_string_nonempty_or_throw(name);
        } else {
            throw support::exception(TRACEMSG("Unknown data field: [" + name + "]"));
        }
    }
    if (-1 == handle) throw support::exception(TRACEMSG(
            "Required parameter 'serialHandle' not specified"));
    if (rdatahex.get().empty()) throw support::exception(TRACEMSG(
            "Required parameter 'dataHex' not specified"));
    std::string sdata = sl::crypto::from_hex(rdatahex.get());
    // get handle
    auto reg = shared_registry();
    wilton_Serial* ser = reg->remove(handle);
    if (nullptr == ser) throw support::exception(TRACEMSG(
            "Invalid 'serialHandle' parameter specified"));
    // call wilton
    int written_out = 0;
    char* err = wilton_Serial_write(ser, sdata.c_str(), 
            static_cast<int> (sdata.length()), std::addressof(written_out));
    reg->put(ser);
    if (nullptr != err) support::throw_wilton_error(err, TRACEMSG(err));
    return support::make_json_buffer({
        { "bytesWritten", written_out }
    });
}

} // namespace
}

extern "C" char* wilton_module_init() {
    try {
        wilton::support::register_wiltoncall("serial_open", wilton::serial::open);
        wilton::support::register_wiltoncall("serial_close", wilton::serial::close);
        wilton::support::register_wiltoncall("serial_read", wilton::serial::read);
        wilton::support::register_wiltoncall("serial_readline", wilton::serial::readline);
        wilton::support::register_wiltoncall("serial_write", wilton::serial::write);
        return nullptr;
    } catch (const std::exception& e) {
        return wilton::support::alloc_copy(TRACEMSG(e.what() + "\nException raised"));
    }
}
