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
#include <unordered_map>

namespace tjpp {

struct HashTJPF {
    size_t operator()(TJPF n) const {
        return std::hash< int >()(int(n));
    }
};
 
inline int NumComponents(TJPF tjpgPixelFormat) {
    static std::unordered_map< TJPF, int, HashTJPF > tjpfToInt = {
        {TJPF_RGB, 3},
        {TJPF_BGR, 3},
        {TJPF_RGBX, 4},
        {TJPF_BGRX, 4},
        {TJPF_XRGB, 4},
        {TJPF_GRAY, 1},
        {TJPF_RGBA, 4},
        {TJPF_BGRA, 4},
        {TJPF_ABGR, 4},
        {TJPF_ARGB, 4},
        {TJPF_CMYK, 4}
    };
    if(tjpfToInt.find(tjpgPixelFormat) == tjpfToInt.end()) {
        throw std::domain_error("Invalid pixel format "
                                    + std::to_string(tjpgPixelFormat));
    }
    return tjpfToInt[tjpgPixelFormat];
}

}