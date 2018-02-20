/* 
 * File:   connection_termios.cpp
 * Author: alex
 *
 * Created on September 12, 2017, 2:10 PM
 */

#include "connection.hpp"

#include <array>
#include <chrono>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include "staticlib/crypto.hpp"
#include "staticlib/support.hpp"
#include "staticlib/pimpl/forward_macros.hpp"
#include "staticlib/utils.hpp"

namespace wilton {
namespace serial {

class connection::impl : public staticlib::pimpl::object::impl {
    serial_config conf;

    int fd = -1;
    
public:
    impl(serial_config&& conf) :
    conf(std::move(conf)) {
        // open port
        this->fd = ::open(this->conf.port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (this->fd < 0) {
            throw support::exception(TRACEMSG(
                "Serial 'open' error, port: [" + this->conf.port + "],"
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
        uint64_t start = sl::utils::current_time_millis_steady();
        auto res = read_some(start, length, conf.timeout_millis);
        return sl::crypto::to_hex(res);
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
        return sl::crypto::to_hex(res);
    }

    uint32_t write(connection&, sl::io::span<const char> data) {
        uint64_t start = sl::utils::current_time_millis_steady();
        uint64_t finish = start + conf.timeout_millis;
        uint64_t cur = start;
        size_t written = 0;
        for(;;) {
            struct pollfd pfd;
            std::memset(std::addressof(pfd), '\0', sizeof(pfd));
            pfd.fd = this->fd;
            pfd.events = POLLOUT;
            uint32_t passed = static_cast<uint32_t> (cur - start);
            int ptm = static_cast<int> (conf.timeout_millis - passed);
            auto err = ::poll(std::addressof(pfd), 1, ptm);
            check_poll_err(pfd, err, "", ptm);
            if (pfd.revents & POLLOUT) {
                auto wr = ::write(this->fd, data.data() + written, data.size() - written);
                if (-1 == wr) {
                    throw support::exception(TRACEMSG(
                            "Serial 'write' error, written: [" + sl::support::to_string(written) + "],"
                            " error: [" + ::strerror(errno) + "]"));
                }
                written += wr;
                if (written >= data.size()) {
                    break;
                }
            }
            cur = sl::utils::current_time_millis_steady();
            if (cur >= finish) {
                break;
            }
        }
        return static_cast<uint32_t>(written);
    }

private:
    static void close_descriptor(int fd) STATICLIB_NOEXCEPT {
        if (-1 != fd) {
            ::close(fd);
        }
    }

    static void check_poll_err(struct pollfd& pfd, int err, const std::string& res, int timeout) {
        if (err < 0) {
            throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(timeout) + "],"
                    " current res: [" + res + "]" +
                    " error: [" + ::strerror(errno) + "]"));
        }
        if (pfd.revents & POLLERR) {
            throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(timeout) + "],"
                    " current res: [" + res + "]" +
                    " error: [POLLERR]"));
        } else if (pfd.revents & POLLHUP) {
            throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(timeout) + "],"
                    " current res: [" + res + "]" +
                    " error: [POLLHUP]"));
        } else if (pfd.revents & POLLNVAL) {
            throw support::exception(TRACEMSG(
                    "Serial 'poll' error, timeout: [" + sl::support::to_string(timeout) + "],"
                    " current res: [" + res + "]" +
                    " error: [POLLNVAL]"));
        }
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
            check_poll_err(pfd, err, res, ptm);
            if (pfd.revents & POLLIN) {
                auto prev_len = res.length();
                res.resize(length);
                auto rlen = length - prev_len;
                auto read = ::read(this->fd, std::addressof(res.front()) + prev_len, rlen);
                if (-1 == read) {
                    throw support::exception(TRACEMSG(""
                        "Serial 'read' error, len: [" + sl::support::to_string(rlen) + "],"
                        " error: [" + ::strerror(errno) + "]"));
                }
                res.resize(prev_len + read);
                if (res.length() >= length) {
                    break;
                }
            }
            
            cur = sl::utils::current_time_millis_steady();
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

    void set_baud_rate(struct termios& tty) {
        speed_t rate = B0;
        switch (conf.baud_rate) {
        case 0: rate = B0; break;
        case 50: rate = B50; break;
        case 75: rate = B75; break;
        case 110: rate = B110; break;
        case 134: rate = B134; break;
        case 150: rate = B150; break;
        case 200: rate = B200; break;
        case 300: rate = B300; break;
        case 600: rate = B600; break;
        case 1200: rate = B1200; break;
        case 1800: rate = B1800; break;
        case 2400: rate = B2400; break;
        case 4800: rate = B4800; break;
        case 9600: rate = B9600; break;
        case 19200: rate = B19200; break;
        case 38400: rate = B38400; break;
        case 57600: rate = B57600; break;
        case 115200: rate = B115200; break;
        case 230400: rate = B230400; break;
        case 460800: rate = B460800; break;
        case 500000: rate = B500000; break;
        case 576000: rate = B576000; break;
        case 921600: rate = B921600; break;
        default: throw support::exception(TRACEMSG(
                "Invalid 'baudRate' specified: [" + sl::support::to_string(conf.baud_rate) + "]"));
        }
        auto err_o = ::cfsetospeed(std::addressof(tty), rate);
        if (0 != err_o) {
            throw support::exception(TRACEMSG(
                "Serial 'cfsetospeed' error, baudrate: [" + sl::support::to_string(conf.baud_rate) + "]," +
                " error: [" + ::strerror(errno) + "]"));
        }
        auto err_i = ::cfsetispeed(std::addressof(tty), rate);
        if (0 != err_i) {
            throw support::exception(TRACEMSG(
                "Serial 'cfsetispeed' error, baudrate: [" + sl::support::to_string(conf.baud_rate) + "]," +
                " error: [" + ::strerror(errno) + "]"));
        }
    }

    void set_byte_size(struct termios& tty) {
        switch(conf.byte_size) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        case 8: tty.c_cflag |= CS8; break;
        default: throw support::exception(TRACEMSG(
                "Invalid 'byteSize' specified: [" + sl::support::to_string(conf.byte_size) + "]"));
        }
    }

    void set_stop_bits(struct termios& tty) {
        switch(conf.stop_bits_count) {
        case 1: tty.c_cflag &= ~(CSTOPB); break;
        case 2: tty.c_cflag |= (CSTOPB); break;
        default: throw support::exception(TRACEMSG(
                "Invalid 'stopBitsCount' specified: [" + sl::support::to_string(conf.byte_size) + "]"));
        }
    }

    void set_parity(struct termios& tty) {
        tty.c_iflag &= ~(INPCK | ISTRIP);
        switch(conf.parity) {
        case parity_type::none: {
            tty.c_cflag &= ~(PARENB | PARODD | CMSPAR);
            break;
        }
        case parity_type::even: {
            tty.c_cflag &= ~(PARODD | CMSPAR);
            tty.c_cflag |= (PARENB);
            break;
        }
        case parity_type::odd: {
            tty.c_cflag &= ~CMSPAR;
            tty.c_cflag |= (PARENB | PARODD);
            break;
        }
        case parity_type::mark: {
            tty.c_cflag |= (PARENB | CMSPAR | PARODD);
            break;
        }
        case parity_type::space: {
            tty.c_cflag |= (PARENB | CMSPAR);
            tty.c_cflag &= ~(PARODD);
            break;
        }
        default: throw support::exception(TRACEMSG(
                "Invalid 'parity' specified: [" + stringify_parity_type(conf.parity) + "]"));
        }
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
PIMPL_FORWARD_CONSTRUCTOR(connection, (serial_config&&), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read, (uint32_t), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read_line, (), (), support::exception)
PIMPL_FORWARD_METHOD(connection, uint32_t, write, (sl::io::span<const char>), (), support::exception)

} // namespace
}
