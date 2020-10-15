#pragma once
//Author: Ugo Varetto
//
// This file is part of tjpp.
//tjpp is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//tjpp is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with tjpp.  If not, see <http://www.gnu.org/licenses/>.

#include <chrono>
namespace tjpp {
using Timer = std::chrono::steady_clock;
using Time = Timer::time_point;
using Duration = Timer::duration;
namespace {
Time Tick() { return Timer::now(); }
inline std::chrono::milliseconds toms(Duration d) {
    return std::chrono::duration_cast< std::chrono::milliseconds >(d);
}
}
}