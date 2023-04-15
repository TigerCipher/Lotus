//  ------------------------------------------------------------------------------
//
//  Lotus
//     Copyright 2023 Matthew Rogers
//
//     This library is free software; you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as
//     published by the Free Software Foundation; either version 3 of the
//     License, or (at your option) any later version.
//
//     This library is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//     Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public
//     License along with this library; if not, see <http://www.gnu.org/licenses/>.
//
//  File Name: Logger.h
//  Date File Created: 04/14/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#pragma once

#include "Core/Types.h"

#include <format>

namespace lotus::logger
{

struct log_level
{
    enum level : u8
    {
        trace,
        debug,
        info,
        warn,
        error,
        fatal
    };
};

namespace detail
{
void output(log_level::level lvl, std::string_view msg);
}

} // namespace lotus::logger