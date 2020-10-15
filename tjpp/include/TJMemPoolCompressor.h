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

#include <stdexcept>
#include <turbojpeg.h>

#include "SyncQueue.h"
#include "JPEGImage.h"
#include "timing.h"

namespace tjpp {
class TJMemPoolCompressor {
public:
    class JPEGImageWrapper {
    public:
        JPEGImageWrapper(JPEGImage img,
                         std::shared_ptr< SyncQueue< JPEGImage > > sq)
            : img_(img), queue_(sq) {}
        const JPEGImage& Image() const { return img_; }
        operator const JPEGImage&() { return Image(); }
        void Flush() {
            queue_->Push(std::move(img_));
        }
        ~JPEGImageWrapper() {
            Flush();
        }
    private:
        JPEGImage img_;
        std::shared_ptr< SyncQueue< JPEGImage > > queue_;
    };
public:
    TJMemPoolCompressor(int numBuffers = 0,
                        size_t w = 0,
                        size_t h = 0,
                        TJPF pf = TJPF_RGB,
                        TJSAMP ss = TJSAMP_420,
                        int q = 75,
                        int flags = TJFLAG_FASTDCT) :
        memoryPool_(new SyncQueue< JPEGImage >()),
        tjCompressor_(tjInitCompress()) {
        for(int i = 0; i != numBuffers; ++i) {
            memoryPool_->Push(JPEGImage(w, h, pf, ss, q));
        }
    }
    JPEGImageWrapper Compress(const unsigned char* img,
                              int width,
                              int height,
                              TJPF pf,
                              TJSAMP ss,
                              int quality,
                              int offset = 0,
                              int flags = TJFLAG_FASTDCT,
                              int pitch = 0) {

        JPEGImage i;/*(memoryPool_->Pop(
            JPEGImage(width, height, pf, ss, quality)));*/

        if(i.Empty()
            || tjBufSize(width, height, ss) > i.BufferSize()) {
            i.Reset(width, height, pf, ss, quality);
        }
        i.SetParams(width, height, pf, ss, quality);
        size_t jpegSize = int(UncompressedSize(i));
        unsigned char* ptr = i.DataPtr();
#ifdef TIMING__
        Time begin = Tick();
#endif
        if(tjCompress2(tjCompressor_, img + offset, width, pitch, height, pf,
                       &ptr, &jpegSize, ss, quality,
                       flags))
            throw std::runtime_error(tjGetErrorStr());
#ifdef TIMING__
        Time end = Tick();
        std::cout << "tjCompress2: "
                  << toms(end - begin).count()
                  << std::endl;
#endif
        i.SetCompressedSize(jpegSize);
        return JPEGImageWrapper(i, memoryPool_);
    }
    void PutBack(JPEGImage&& im) {
        memoryPool_->Push(std::move(im));
    }
    void PutBack(JPEGImage& im) {
        memoryPool_->Push(std::move(im));
    }
    ~TJMemPoolCompressor() {
        tjDestroy(tjCompressor_);
    }
private:
    std::shared_ptr< SyncQueue< JPEGImage > > memoryPool_;
    tjhandle tjCompressor_;
};
}