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
 * File:   wilton_serial.h
 * Author: alex
 *
 * Created on September 10, 2017, 10:40 AM
 */

#ifndef WILTON_SERIAL_H
#define WILTON_SERIAL_H

#include "wilton/wilton.h"

#ifdef __cplusplus
extern "C" {
#endif

struct wilton_Serial;
typedef struct wilton_Serial wilton_Serial;

char* wilton_Serial_open(
        wilton_Serial** ser_out,
        const char* conf,
        int conf_len);

char* wilton_Serial_read(
        wilton_Serial* ser,
        int len,
        char** data_out,
        int* data_len_out);

char* wilton_Serial_readline(
        wilton_Serial* ser,
        char** data_out,
        int* data_len_len);

char* wilton_Serial_write(
        wilton_Serial* ser,
        const char* data,
        int data_len,
        int* len_written_out);

char* wilton_Serial_close(
        wilton_Serial* ser);

#ifdef __cplusplus
}
#endif

#endif /* WILTON_SERIAL_H */

