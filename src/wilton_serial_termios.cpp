/* 
 * File:   wilton_serial.cpp
 * Author: alex
 *
 * Created on September 10, 2017, 10:41 AM
 */

#include "wilton/wilton_serial.h"


#include <string>

#include "staticlib/config.hpp"

#include "wilton/support/alloc_copy.hpp"
#include "wilton/support/buffer.hpp"

struct wilton_Serial {
private:
    std::nullptr_t tmp;
public:

    wilton_Serial(std::nullptr_t tmp) :
    tmp(tmp) { }

    std::nullptr_t& impl() {
        return tmp;
    }
};

char* wilton_Serial_open(
        wilton_Serial** conn_out,
        char* conf,
        int conf_len) /* noexcept */ {
    if (nullptr == conn_out) return wilton::support::alloc_copy(TRACEMSG("Null 'conn_out' parameter specified"));
    (void) conf;
    (void) conf_len;
    return nullptr;
}

char* wilton_Serial_read(
        wilton_Serial* conn,
        int len,
        char** data_out,
        int* data_out_len) /* noexcept */ {
    if (nullptr == conn) return wilton::support::alloc_copy(TRACEMSG("Null 'conn' parameter specified"));
    (void) len;
    (void) data_out;
    (void) data_out_len;
    return nullptr;

}

char* wilton_Serial_readline(
        wilton_Serial* conn,
        char** data_out,
        int* data_out_len) /* noexcept */ {
    if (nullptr == conn) return wilton::support::alloc_copy(TRACEMSG("Null 'conn' parameter specified"));
    (void) data_out;
    (void) data_out_len;
    return nullptr;

}

char* wilton_Serial_write(
        wilton_Serial* conn,
        const char* data,
        int data_len) /* noexcept */ {
    if (nullptr == conn) return wilton::support::alloc_copy(TRACEMSG("Null 'conn' parameter specified"));
    (void) data;
    (void) data_len;
    return nullptr;

}

char* wilton_Serial_close(
        wilton_Serial* conn) /* noexcept */ {
    if (nullptr == conn) return wilton::support::alloc_copy(TRACEMSG("Null 'conn' parameter specified"));
    return nullptr;

}

