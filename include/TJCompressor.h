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

//ADD:
// flag support
// pitch: useful to e.g. split and compress in parallel 4k images

#include <turbojpeg.h>

#include "JPEGImage.h"
#include "timing.h"

namespace tjpp {
class TJCompressor {
public:
    TJCompressor() :
        tjCompressor_(tjInitCompress()) {}
    JPEGImage Compress(const unsigned char* img,
                       int width,
                       int height,
                       TJPF pf,
                       TJSAMP ss,
                       int quality,
                       int offset = 0,
                       int flags = TJFLAG_FASTDCT,
                       int pitch = 0) {
        if(img_.Empty()
            || tjBufSize(width, height, ss) > img_.BufferSize()) {
            img_.Reset(width, height, pf, ss, quality);
        }
        img_.SetParams(width, height, pf, ss, quality);
        size_t jpegSize = int(UncompressedSize(img_));
        unsigned char* ptr = img_.DataPtr();
#ifdef TIMING__
        Time begin = Tick();
#endif
        if(tjCompress2(tjCompressor_, img + offset, width, 0, height, pf,
                       &ptr, &jpegSize, ss, quality,
                       flags))
            throw std::runtime_error(tjGetErrorStr());
#ifdef TIMING__
        Time end = Tick();
        std::cout << "tjCompress2: "
                  << toms(end - begin).count()
                  << " ms\n";
#endif
        img_.SetCompressedSize(jpegSize);
        return img_;
    }
    //reuse image
    JPEGImage Compress(JPEGImage&& recycled,
                       const unsigned char* img,
                       int width,
                       int height,
                       TJPF pf,
                       TJSAMP ss,
                       int quality,
                       int offset = 0,
                       int flags = TJFLAG_FASTDCT,
                       int pitch = 0 ) {
        img_ = std::move(recycled);
        return Compress(img, width, height, pf, ss,
                        quality, offset, flags, pitch);

    }
    ~TJCompressor() {
        tjDestroy(tjCompressor_);
    }
private:
    JPEGImage img_;
    tjhandle tjCompressor_;
};
}