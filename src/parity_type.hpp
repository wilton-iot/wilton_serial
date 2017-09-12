/* 
 * File:   parity_type.hpp
 * Author: alex
 *
 * Created on September 11, 2017, 11:33 PM
 */

#include <string>

#include "staticlib/support.hpp"

#include "wilton/support/exception.hpp"

#ifndef WILTON_SERIAL_PARITY_TYPE_HPP
#define WILTON_SERIAL_PARITY_TYPE_HPP

namespace wilton {
namespace serial {

enum class parity_type {
    none,
    even,
    odd,
    mark,
    space
};

inline std::string stringify_parity_type(parity_type pt) {
    switch (pt) {
    case parity_type::none: return "NONE";
    case parity_type::even: return "EVEN";
    case parity_type::odd: return "ODD";
    case parity_type::mark: return "MARK";
    case parity_type::space: return "SPACE";
    default: return "UNKNOWN";
    }
}

inline parity_type make_parity_type(const std::string& st) {
    if ("NONE" == st) {
        return parity_type::none;
    } else if ("EVEN" == st) {
        return parity_type::even;
    } else if ("ODD" == st) {
        return parity_type::odd;
    } else if ("MARK" == st) {
        return parity_type::mark;
    } else if ("SPACE" == st) {
        return parity_type::space;
    } else throw support::exception(TRACEMSG("Invalid parity type: [" + st + "]"));
}

} // namespace
}

#endif /* WILTON_SERIAL_PARITY_TYPE_HPP */

