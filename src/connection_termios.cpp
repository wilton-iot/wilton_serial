/* 
 * File:   connection_termios.cpp
 * Author: alex
 *
 * Created on September 12, 2017, 2:10 PM
 */

#include "connection.hpp"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "staticlib/support.hpp"
#include "staticlib/pimpl/forward_macros.hpp"

namespace wilton {
namespace serial {

class connection::impl : public staticlib::pimpl::object::impl {
    int fd = -1;
    int pipe_read_w = -1;
    int pipe_read_r = -1;
    int pipe_write_w = -1;
    int pipe_write_r = -1;
    
public:
    impl(const serial_config& conf) {
        // open port
        this->fd = ::open(conf.port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (this->fd < 0) throw support::exception(TRACEMSG(
                "Serial 'open' error, port: [" + conf.port + "],"
                " error: [" + ::strerror(errno) + "]"));

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
        // TODO break pipes
    }

    ~impl() STATICLIB_NOEXCEPT {
        close_descriptor(fd);
        close_descriptor(pipe_read_w);
        close_descriptor(pipe_read_r);
        close_descriptor(pipe_write_w);
        close_descriptor(pipe_write_r);
    };
    
    // TODO
    std::string read(connection&, uint32_t length) {
        std::string res;
        res.resize(length);
        auto rd = ::read(fd, std::addressof(res.front()), res.length());
        res.resize(rd);
        return res;
    }

    // TODO
    std::string read_line(connection& self) {
        std::string res;
        for(;;) {
            auto ch = this->read(self, 1);
            if (ch.empty() || '\n' == ch.at(0)) {
                break;
            }
            res.push_back(ch.at(0));
        }
        return res;
    }

    // TODO
    uint32_t write(connection&, sl::io::span<const char> data) {
        auto written = ::write(fd, data.data(), data.size());
        if (!sl::support::is_uint32(written)) throw support::exception(TRACEMSG(
                "Serial 'write' error: [" + ::strerror(errno) + "]"));
        return static_cast<uint32_t>(written);
    }

private:
    static void close_descriptor(int fd) STATICLIB_NOEXCEPT {
        if (-1 != fd) {
            ::close(fd);
        }
    }

    void load_tty_params(struct termios& tty) {
        auto err = ::tcgetattr(fd, std::addressof(tty));
        if (0 != err) throw support::exception(TRACEMSG(
                "Serial 'tcgetattr' error: [" + ::strerror(errno) + "]"));
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
        if (0 != err_o) throw support::exception(TRACEMSG(
                "Serial 'cfsetospeed' error, baudrate: [" + "TODO" + "]," +
                " error: [" + ::strerror(errno) + "]"));

        auto err_i = ::cfsetispeed(std::addressof(tty), B4800);
        if (0 != err_i) throw support::exception(TRACEMSG(
                "Serial 'cfsetispeed' error, baudrate: [" + "TODO" + "]," +
                " error: [" + ::strerror(errno) + "]"));
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
        if (0 != err) throw support::exception(TRACEMSG(
                "Serial 'tcsetattr' error: [" + ::strerror(errno) + "]"));
    }
    
    void flush_input_buffer() {
        auto err = ::tcflush(fd, TCIFLUSH);
        if (0 != err) throw support::exception(TRACEMSG(
                "Serial 'tcflush' error: [" + ::strerror(errno) + "]"));
    }

};
PIMPL_FORWARD_CONSTRUCTOR(connection, (const serial_config&), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read, (uint32_t), (), support::exception)
PIMPL_FORWARD_METHOD(connection, std::string, read_line, (), (), support::exception)
PIMPL_FORWARD_METHOD(connection, uint32_t, write, (sl::io::span<const char>), (), support::exception)

} // namespace
}
