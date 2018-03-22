/*
 * Copyright 2018, alex at staticlibs.net
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
 * File:   connection_windows.cpp
 * Author: alex
 *
 * Created on September 12, 2017, 2:10 PM
 */

#include "connection.hpp"

#include <array>
#include <chrono>
#include <utility>

#ifndef UNICODE
#define UNICODE
#endif // UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE
#ifndef NOMINMAX
#define NOMINMAX
#endif NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "staticlib/support.hpp"
#include "staticlib/pimpl/forward_macros.hpp"
#include "staticlib/utils.hpp"

namespace wilton {
namespace serial {

class connection::impl : public staticlib::pimpl::object::impl {
    serial_config conf;

    HANDLE handle = nullptr;
 
public:
    impl(serial_config&& conf) :
    conf(std::move(conf)) {
        // oper port
        this->handle = open_com_port();

        // set params
        DCB dcb;
        std::memset(std::addressof(dcb), '\0', sizeof (dcb));
        load_dcb_params(dcb);
        dcb.BaudRate = static_cast<DWORD>(this->conf.baud_rate);
        dcb.ByteSize = static_cast<BYTE>(this->conf.byte_size);
        set_stop_bits(dcb);
        set_parity(dcb);
        apply_dcb_params(dcb);
        flush_input_buffer();
    }

    ~impl() STATICLIB_NOEXCEPT {
        if (nullptr != handle) {
            ::CloseHandle(handle);
        }
    }

    std::string read(connection&, uint32_t length) {
        uint64_t start = sl::utils::current_time_millis_steady();
        return read_some(start, length, conf.timeout_millis);
    }

    std::string read_line(connection&) {
        uint64_t start = sl::utils::current_time_millis_steady();
        uint64_t finish = start + conf.timeout_millis;
        uint64_t cur = start;
        std::string res;
        for(;;) {
            uint32_t passed = static_cast<uint32_t> (cur - start);
            auto ch = this->read_some(cur, 1, conf.timeout_millis - passed);
            if (ch.empty() || '\n' == ch.at(0)) {
                break;
            }
            res.push_back(ch.at(0));
            cur = sl::utils::current_time_millis_steady();
            if (cur >= finish) {
                break;
            }
        }
        if (res.length() > 0 && '\r' == res.back()) {
            res.pop_back();
        }
        return res;
    }

    uint32_t write(connection&, sl::io::span<const char> data) {
        uint64_t start = sl::utils::current_time_millis_steady();
        uint64_t finish = start + conf.timeout_millis;
        uint64_t cur = start;
        size_t written = 0;
        for(;;) {
            // (err, bytes_written)
            auto state = std::pair<DWORD, DWORD>();
            OVERLAPPED overlapped;
            std::memset(std::addressof(overlapped), '\0', sizeof (overlapped)); 
            overlapped.hEvent = static_cast<void*>(std::addressof(state));

            // completion routine
            auto completion = static_cast<LPOVERLAPPED_COMPLETION_ROUTINE> ([](
                    DWORD err, DWORD bytes_written, LPOVERLAPPED overlapped_ptr) {
                auto state_ptr = static_cast<std::pair<DWORD, DWORD>*>(overlapped_ptr->hEvent);
                state_ptr->first = err;
                state_ptr->second = bytes_written;
            });

            // prepare write
            uint32_t passed = static_cast<uint32_t> (cur - start);
            int wtm = static_cast<DWORD> (conf.timeout_millis - passed);
            auto msg = std::string(data.data() + written, data.size() - written);

            // start write
            auto err_write = ::WriteFileEx(
                    this->handle,
                    static_cast<void*> (std::addressof(msg.front())),
                    static_cast<DWORD> (msg.length()),
                    std::addressof(overlapped),
                    completion); 

            if (0 == err_write) throw support::exception(TRACEMSG(
                    "Serial 'WriteFileEx' error, port: [" + this->conf.port + "]," +
                    " bytes left to write: [" + sl::support::to_string(msg.length()) + "]" +
                    " bytes written: [" + sl::support::to_string(written) + "]" +
                    " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

            auto err_wait_written = ::SleepEx(wtm, TRUE);
            if (WAIT_IO_COMPLETION != err_wait_written) {
                // cancel pending operation
                auto err_cancel = ::CancelIo(this->handle);
                if (0 == err_cancel) throw support::exception(TRACEMSG(
                        "Serial 'CancelIo' error, port: [" + this->conf.port + "]," +
                        " bytes left to write: [" + sl::support::to_string(msg.length()) + "]" +
                        " bytes written: [" + sl::support::to_string(written) + "]" +
                        " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
                // wait for operation to be canceled
                auto err_wait_canceled = ::SleepEx(INFINITE, TRUE);
                if (WAIT_IO_COMPLETION != err_wait_canceled) throw support::exception(TRACEMSG(
                        "Serial 'SleepEx' error, port: [" + this->conf.port + "]," +
                        " bytes left to write: [" + sl::support::to_string(msg.length()) + "]" +
                        " bytes written: [" + sl::support::to_string(written) + "]" +
                        " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
            }

            // at this point completion routine must be called
            if (ERROR_SUCCESS == state.first) {
                // check for warnings
                DWORD written_checked = 0;
                overlapped.hEvent = 0;
                auto err_get = ::GetOverlappedResult(
                        this->handle,
                        std::addressof(overlapped),
                        std::addressof(written_checked),
                        TRUE);
                if (0 == err_get) throw support::exception(TRACEMSG(
                        "Serial 'GetOverlappedResult' error, port: [" + this->conf.port + "]," +
                        " bytes left to write: [" + sl::support::to_string(msg.length()) + "]" +
                        " bytes written: [" + sl::support::to_string(written) + "]" +
                        " bytes completion: [" + sl::support::to_string(state.second) + "]" +
                        " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

                written += static_cast<size_t>(written_checked > state.second ? written_checked : state.second);
                // check everything written
                if (written >= data.size()) {
                    break;
                } 
            } else if (ERROR_OPERATION_ABORTED != state.first) throw support::exception(TRACEMSG(
                    "Serial 'FileIOCompletionRoutine' error, port: [" + this->conf.port + "]," +
                    " bytes left to write: [" + sl::support::to_string(msg.length()) + "]" +
                    " bytes written: [" + sl::support::to_string(written) + "]" +
                    " error: [" + sl::utils::errcode_to_string(state.first) + "]"));

            // check timeout
            cur = sl::utils::current_time_millis_steady();
            if (cur >= finish) {
                break;
            }
        }
        return static_cast<uint32_t>(written);
    }

private:

    std::string read_some(uint64_t start, uint32_t length, uint32_t timeout_millis) {
        uint64_t finish = start + timeout_millis;
        uint64_t cur = start;
        std::string res;
        for (;;) {
            // (err, bytes_read)
            auto state = std::pair<DWORD, DWORD>();
            OVERLAPPED overlapped;
            std::memset(std::addressof(overlapped), '\0', sizeof (overlapped)); 
            overlapped.hEvent = static_cast<void*>(std::addressof(state));

            // completion routine
            auto completion = static_cast<LPOVERLAPPED_COMPLETION_ROUTINE> ([](
                    DWORD err, DWORD bytes_read, LPOVERLAPPED overlapped_ptr) {
                auto state_ptr = static_cast<std::pair<DWORD, DWORD>*>(overlapped_ptr->hEvent);
                state_ptr->first = err;
                state_ptr->second = bytes_read;
            });

            // find out how much data is buffered
            DWORD flags = 0;
            COMSTAT comstat;
            std::memset(std::addressof(comstat), '\0', sizeof(comstat));
            auto err_clear = ::ClearCommError(this->handle, std::addressof(flags), std::addressof(comstat));
            if (0 == err_clear) throw support::exception(TRACEMSG(
                    "Serial 'ClearCommError' error, port: [" + this->conf.port + "]," +
                    " bytes to read: [" + sl::support::to_string(length) + "]" +
                    " bytes read: [" + sl::support::to_string(res.length()) + "]" +
                    " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
            size_t avail = static_cast<size_t>(comstat.cbInQue);

            // prepare read
            uint32_t passed = static_cast<uint32_t> (cur - start);
            int rtm = static_cast<int> (timeout_millis - passed);
            auto prev_len = res.length();
            res.resize(length);
            auto rlen = length - prev_len;
            if (avail > 0 && avail < rlen) {
                rlen = avail;
            }

            // start read
            auto err_read = ::ReadFileEx(
                    this->handle,
                    static_cast<void*> (std::addressof(res.front()) + prev_len),
                    static_cast<DWORD> (rlen),
                    std::addressof(overlapped),
                    completion); 

            if (0 == err_read) throw support::exception(TRACEMSG(
                    "Serial 'ReadFileEx' error, port: [" + this->conf.port + "]," +
                    " bytes to read: [" + sl::support::to_string(length) + "]" +
                    " bytes read: [" + sl::support::to_string(res.length()) + "]" +
                    " bytes avail: [" + sl::support::to_string(avail) + "]" +
                    " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

            auto err_wait_read = ::SleepEx(rtm, TRUE);
            if (WAIT_IO_COMPLETION != err_wait_read) {
                // cancel pending operation
                auto err_cancel = ::CancelIo(this->handle);
                if (0 == err_cancel) throw support::exception(TRACEMSG(
                        "Serial 'CancelIo' error, port: [" + this->conf.port + "]," +
                        " bytes to read: [" + sl::support::to_string(length) + "]" +
                        " bytes read: [" + sl::support::to_string(res.length()) + "]" +
                        " bytes avail: [" + sl::support::to_string(avail) + "]" +
                        " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
                // wait for operation to be canceled
                auto err_wait_canceled = ::SleepEx(INFINITE, TRUE);
                if (WAIT_IO_COMPLETION != err_wait_canceled) throw support::exception(TRACEMSG(
                        "Serial 'SleepEx' error, port: [" + this->conf.port + "]," +
                        " bytes to read: [" + sl::support::to_string(length) + "]" +
                        " bytes read: [" + sl::support::to_string(res.length()) + "]" +
                        " bytes avail: [" + sl::support::to_string(avail) + "]" +
                        " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
            }

            // at this point completion routine must be called
            if (ERROR_SUCCESS == state.first) {
                // check for warnings
                DWORD read_checked = 0;
                overlapped.hEvent = 0;
                auto err_get = ::GetOverlappedResult(
                        this->handle,
                        std::addressof(overlapped),
                        std::addressof(read_checked),
                        TRUE);
                if (0 == err_get) throw support::exception(TRACEMSG(
                        "Serial 'GetOverlappedResult' error, port: [" + this->conf.port + "]," +
                        " bytes to read: [" + sl::support::to_string(length) + "]" +
                        " bytes read: [" + sl::support::to_string(res.length()) + "]" +
                        " bytes avail: [" + sl::support::to_string(avail) + "]" +
                        " bytes completion: [" + sl::support::to_string(state.second) + "]" +
                        " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
                
                auto read = static_cast<size_t>(read_checked > state.second ? read_checked : state.second);
                res.resize(prev_len + read);
                if (res.length() >= length) {
                    break;
                }
            } else if (ERROR_OPERATION_ABORTED == state.first) {
                res.resize(prev_len);
            } else throw support::exception(TRACEMSG(
                    "Serial 'FileIOCompletionRoutine' error, port: [" + this->conf.port + "]," +
                    " bytes to read: [" + sl::support::to_string(length) + "]" +
                    " bytes read: [" + sl::support::to_string(res.length()) + "]" +
                    " bytes avail: [" + sl::support::to_string(avail) + "]" +
                    " error: [" + sl::utils::errcode_to_string(state.first) + "]"));

            // check timeout
            cur = sl::utils::current_time_millis_steady();
            if (cur >= finish) {
                break;
            }
        }
        return res;
    }

    HANDLE open_com_port() {
        // open port
        auto wport = sl::utils::widen(this->conf.port);
        HANDLE handle = ::CreateFileW(wport.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                nullptr); 
        if (INVALID_HANDLE_VALUE == handle) throw support::exception(TRACEMSG(
                "Serial 'CreateFileW' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

        // setup a 4k buffer
        auto err_setup = ::SetupComm(handle, 4096, 4096);
        if (0 == err_setup) throw support::exception(TRACEMSG(
                "Serial 'SetupComm' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

        // disable timeouts
        COMMTIMEOUTS timeouts;
        std::memset(std::addressof(timeouts), '\0', sizeof(timeouts));
        auto err_timeouts = ::SetCommTimeouts(handle, std::addressof(timeouts));
        if (0 == err_timeouts) throw support::exception(TRACEMSG(
                "Serial 'SetCommTimeouts' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

        // set events
        auto err_mask = ::SetCommMask(handle, EV_ERR);
        if (0 == err_mask) throw support::exception(TRACEMSG(
                "Serial 'SetCommMask' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));

        return handle;
    }

    void load_dcb_params(DCB& dcb) {
        auto err = ::GetCommState(this->handle, std::addressof(dcb)); 
        if (0 == err) throw support::exception(TRACEMSG(
                "Serial 'GetCommState' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
    }

    void set_stop_bits(DCB& dcb) {
        switch(conf.stop_bits_count) {
        case 1: dcb.StopBits = ONESTOPBIT; break;
        case 2: dcb.StopBits = TWOSTOPBITS; break;
        default: throw support::exception(TRACEMSG(
                "Invalid 'stopBitsCount' specified: [" + sl::support::to_string(conf.byte_size) + "]"));
        }
    }

    void set_parity(DCB& dcb) {
        switch(conf.parity) {
        case parity_type::none: {
            dcb.Parity = NOPARITY;
            dcb.fParity = FALSE;
            break;
        }
        case parity_type::even: {
            dcb.Parity = EVENPARITY;
            dcb.fParity = TRUE;
            break;
        }
        case parity_type::odd: {
            dcb.Parity = ODDPARITY;
            dcb.fParity = TRUE;
            break;
        }
        case parity_type::mark: {
            dcb.Parity = MARKPARITY;
            dcb.fParity = TRUE;
            break;
        }
        case parity_type::space: {
            dcb.Parity = SPACEPARITY;
            dcb.fParity = TRUE;
            break;
        }
        default: throw support::exception(TRACEMSG(
                "Invalid 'parity' specified: [" + stringify_parity_type(conf.parity) + "]"));
        }
    }

    void apply_dcb_params(DCB& dcb) {
        auto err = ::SetCommState(this->handle, std::addressof(dcb));
        if (0 == err) throw support::exception(TRACEMSG(
                "Serial 'SetCommState' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
    }

    void flush_input_buffer() {
        auto err = ::PurgeComm(this->handle,
                PURGE_TXCLEAR | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_RXABORT);
        if (0 == err) throw support::exception(TRACEMSG(
                "Serial 'PurgeComm' error, port: [" + this->conf.port + "],"
                " error: [" + sl::utils::errcode_to_string(::GetLastError()) + "]"));
    }

};
PIMPL_FORWARD_CONSTRUCTOR(connection, (serial_config&&), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read, (uint32_t), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read_line, (), (), support::exception)
PIMPL_FORWARD_METHOD(connection, uint32_t, write, (sl::io::span<const char>), (), support::exception)

} // namespace
}
