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

#include <stdexcept>
#include <turbojpeg.h>

#include "Image.h"
#include "timing.h"

namespace tjpp {
class TJDeCompressor {
public:
    TJDeCompressor(size_t preAllocatedSize = 0) :
        tjDeCompressor_(tjInitDecompress()) {
        if(preAllocatedSize > 0) {
            img_.Allocate(preAllocatedSize);
        }
    }
    //read data from header case
    Image DeCompress(unsigned char* jpgImg,
                     size_t size,
                     int pf,
                     int flags = TJFLAG_FASTDCT,
                     int pitch = 0) {
        int width = -1;
        int height = -1;
        int jpegSubsamp = -1;
        if(tjDecompressHeader2(tjDeCompressor_,
                               jpgImg,
                               size,
                               &width,
                               &height,
                               &jpegSubsamp))
            throw std::runtime_error(tjGetErrorStr());
        const size_t uncompressedSize =
            width * height * NumComponents(TJPF(pf));

        img_.SetParameters(width, height, TJPF(pf));
        if(img_.AllocatedSize() < uncompressedSize) 
            img_.Allocate(uncompressedSize);
        
#ifdef TIMING__
        Time begin = Tick();
#endif
        if(tjDecompress2(tjDeCompressor_, jpgImg, size, img_.DataPtr(),
                         width, pitch, height, pf, flags))
            throw std::runtime_error(tjGetErrorStr());
#ifdef TIMING__
        Time end = Tick();
        std::cout << "tjDecompress2: "
                  << toms(end - begin).count()
                  << " ms\n";
#endif
        return std::move(img_);
    }
    //reuse image
    Image DeCompress(Image&& recycled,
                     unsigned char* jpgImg,
                     size_t size,
                     int pf,
                     int flags = TJFLAG_FASTDCT,
                     int pitch = 0) {
        img_ = std::move(recycled);
        return DeCompress(jpgImg, size, flags, pf, pitch);
    }
    ~TJDeCompressor() {
        tjDestroy(tjDeCompressor_);
    }
private:
    Image img_;
    tjhandle tjDeCompressor_;
};
}