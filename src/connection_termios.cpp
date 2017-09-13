/* 
 * File:   connection_termios.cpp
 * Author: alex
 *
 * Created on September 12, 2017, 2:10 PM
 */

#include <array>
#include <chrono>

#include "connection.hpp"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include "staticlib/support.hpp"
#include "staticlib/pimpl/forward_macros.hpp"

namespace wilton {
namespace serial {

class connection::impl : public staticlib::pimpl::object::impl {
    uint32_t read_timeout_millis;
    uint32_t write_timeout_millis;

    int fd = -1;
    
public:
    impl(const serial_config& conf) :
    read_timeout_millis(conf.read_timeout_millis), 
    write_timeout_millis(conf.write_timeout_millis) {
        // open port
        this->fd = ::open(conf.port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (this->fd < 0) {
            throw support::exception(TRACEMSG(
                "Serial 'open' error, port: [" + conf.port + "],"
                " error: [" + ::strerror(errno) + "]"));
        }

        // set params
        struct termios tty;
        std::memset(std::addressof(tty), '\0', sizeof(tty));
        load_tty_params(tty);
        set_tty_mode(tty);
        set_baud_rate(tty);
        set_byte_size(tty);
        set_stop_bits(tty);
        set_parity(tty);
        set_flow_control(tty);
        apply_tty_params(tty);
        flush_input_buffer();
    }

    ~impl() STATICLIB_NOEXCEPT {
        close_descriptor(fd);
    };
    
    std::string read(connection&, uint32_t length) {
        uint64_t start = current_time_millis();
        return read_some(start, length, read_timeout_millis);
    }

    std::string read_line(connection&) {
        uint64_t start = current_time_millis();
        uint64_t finish = start + read_timeout_millis;
        uint64_t cur = start;
        std::string res;
        for(;;) {
            uint32_t passed = static_cast<uint32_t> (cur - start);
            auto ch = this->read_some(cur, 1, read_timeout_millis - passed);
            if (ch.empty() || '\n' == ch.at(0)) {
                break;
            }
            res.push_back(ch.at(0));
            cur = current_time_millis();
            if (cur >= finish) {
                break;
            }
        }
        if (res.length() > 0 && '\r' == res.back()) {
            res.pop_back();
        }
        return res;
    }

    // TODO
    uint32_t write(connection&, sl::io::span<const char> data) {
        auto written = ::write(fd, data.data(), data.size());
        if (!sl::support::is_uint32(written)) {
            throw support::exception(TRACEMSG(
                "Serial 'write' error: [" + ::strerror(errno) + "]"));
        }
        return static_cast<uint32_t>(written);
    }

private:
    static void close_descriptor(int fd) STATICLIB_NOEXCEPT {
        if (-1 != fd) {
            ::close(fd);
        }
    }

    static uint64_t current_time_millis() {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now);
        return millis.count();
    }

    std::string read_some(uint64_t start, uint32_t length, uint32_t timeout_millis) {
        uint64_t finish = start + timeout_millis;
        uint64_t cur = start;
        std::string res;
        for (;;) {
            struct pollfd pfd;
            std::memset(std::addressof(pfd), '\0', sizeof(pfd));
            pfd.fd = this->fd;
            pfd.events = POLLIN;
            uint32_t passed = static_cast<uint32_t> (cur - start);
            int ptm = static_cast<int> (timeout_millis - passed);
            auto err = ::poll(std::addressof(pfd), 1, ptm);
            if (err < 0) {
                throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(ptm) + "],"
                    " error: [" + ::strerror(errno) + "]"));
            }
            if (pfd.revents & POLLIN) {
                auto prev_len = res.length();
                res.resize(length);
                auto rlen = length - prev_len;
                auto read = ::read(this->fd, std::addressof(res.front()), rlen);
                if (-1 == read) {
                    throw support::exception(TRACEMSG(""
                        "Serial 'read' error, len: [" + sl::support::to_string(rlen) + "],"
                        " error: [" + ::strerror(errno) + "]"));
                }
                res.resize(prev_len + read);
                if (res.length() >= length) {
                    break;
                }
            } else if (pfd.revents & POLLERR) {
                throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(ptm) + "],"
                    " error: [POLLERR]"));
            } else if (pfd.revents & POLLHUP) {
                throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(ptm) + "],"
                    " error: [POLLHUP]"));
            } else if (pfd.revents & POLLNVAL) {
                throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(ptm) + "],"
                    " error: [POLLNVAL]"));
            }
            
            cur = current_time_millis();
            if (cur >= finish) {
                break;
            }
        }
        return res;
    }

    void load_tty_params(struct termios& tty) {
        auto err = ::tcgetattr(fd, std::addressof(tty));
        if (0 != err) {
            throw support::exception(TRACEMSG(
                "Serial 'tcgetattr' error: [" + ::strerror(errno) + "]"));
        }
    }

    void set_tty_mode(struct termios& tty) {
        tty.c_cflag |= (CLOCAL | CREAD);

        tty.c_lflag &= ~(ICANON | ECHO | ECHOE |
                ECHOK | ECHONL |
                ISIG | IEXTEN); // | ECHOPRT
        tty.c_lflag &= ~ECHOCTL;
        tty.c_lflag &= ~ECHOKE;

        tty.c_oflag &= ~(OPOST | ONLCR | OCRNL);

        tty.c_iflag &= ~(INLCR | IGNCR | ICRNL | IGNBRK);
        tty.c_iflag &= ~IUCLC;
        tty.c_iflag &= ~PARMRK;
    }

    // TODO: select
    void set_baud_rate(struct termios& tty) {
        auto err_o = ::cfsetospeed(std::addressof(tty), B4800);
        if (0 != err_o) {
            throw support::exception(TRACEMSG(
                "Serial 'cfsetospeed' error, baudrate: [" + "TODO" + "]," +
                " error: [" + ::strerror(errno) + "]"));
        }
        auto err_i = ::cfsetispeed(std::addressof(tty), B4800);
        if (0 != err_i) {
            throw support::exception(TRACEMSG(
                "Serial 'cfsetispeed' error, baudrate: [" + "TODO" + "]," +
                " error: [" + ::strerror(errno) + "]"));
        }
    }

    // TODO: select
    void set_byte_size(struct termios& tty) {
        tty.c_cflag |= CS8;
    }

    // TODO: select
    void set_stop_bits(struct termios& tty) {
        tty.c_cflag &= ~(CSTOPB);
    }

    // TODO: select
    void set_parity(struct termios& tty) {
        tty.c_iflag &= ~(INPCK | ISTRIP);
        tty.c_cflag &= ~(PARENB | PARODD | CMSPAR);
    }

    void set_flow_control(struct termios& tty) {
        // setup flow control
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_cflag &= ~(CRTSCTS);

        // buffer
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 0;
    }

    void apply_tty_params(struct termios& tty) {
        auto err = ::tcsetattr(fd, TCSANOW, std::addressof(tty));
        if (0 != err) {
            throw support::exception(TRACEMSG(
                "Serial 'tcsetattr' error: [" + ::strerror(errno) + "]"));
        }
    }
    
    void flush_input_buffer() {
        auto err = ::tcflush(fd, TCIFLUSH);
        if (0 != err) {
            throw support::exception(TRACEMSG(
                "Serial 'tcflush' error: [" + ::strerror(errno) + "]"));
        }
    }

};
PIMPL_FORWARD_CONSTRUCTOR(connection, (const serial_config&), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read, (uint32_t), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read_line, (), (), support::exception)
PIMPL_FORWARD_METHOD(connection, uint32_t, write, (sl::io::span<const char>), (), support::exception)

} // namespace
}
