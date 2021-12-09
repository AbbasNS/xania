/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "Types.hpp"

#include <string_view>

// See FlagFormat.hpp for the flag serialization & formatting routines.

// Properties of an individual status bit. Used by Char, Room, Object and other things.
struct Flag {
    constexpr Flag(const unsigned flag_bit, std::string_view flag_name, const sh_int minimum_level = 0)
        : bit(flag_bit), name(flag_name), min_level(minimum_level) {}
    const unsigned int bit;
    std::string_view name;
    const sh_int min_level;
};
