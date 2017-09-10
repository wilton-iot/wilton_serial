/* 
 * File:   wilton_serial.h
 * Author: alex
 *
 * Created on September 10, 2017, 10:40 AM
 */

#ifndef WILTON_SERIAL_H
#define	WILTON_SERIAL_H

#include "wilton/wilton.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wilton_Serial;
typedef struct wilton_Serial wilton_Serial;

char* wilton_Serial_open(
        wilton_Serial** conn_out,
        char* conf,
        int conf_len);

char* wilton_Serial_read(
        wilton_Serial* conn,
        int len,
        char** data_out,
        int* data_out_len);

char* wilton_Serial_readline(
        wilton_Serial* conn,
        char** data_out,
        int* data_out_len);

char* wilton_Serial_write(
        wilton_Serial* conn,
        const char* data,
        int data_len);

char* wilton_Serial_close(
        wilton_Serial* conn);

#ifdef __cplusplus
}
#endif

#endif	/* WILTON_SERIAL_H */

