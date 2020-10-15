#pragma once
///Author: Ugo Varetto
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

//Note: consider using X* or *X pixel format to speed up memory access
#include <memory>
#include <stdexcept>

#include <turbojpeg.h>

#include "pixelformat.h"

namespace tjpp {

inline void TJDeleter(unsigned char* ptr) {
    if(!ptr)
        return;
    tjFree(ptr);
}

class JPEGImage {
public:
    JPEGImage() : width_(0), height_(0), pixelFormat_(TJPF()),
                  subSampling_(TJSAMP()), quality_(50), pitch_(0),
                  compressedSize_(0), bufferSize_(0) {}
    JPEGImage(const JPEGImage&) = default;
    JPEGImage(JPEGImage&& i) {
        Move(i);
    }
    JPEGImage(int w, int h, TJPF pf, TJSAMP s, int q) :
        width_(w), height_(h), pixelFormat_(pf), subSampling_(s), quality_(q),
        data_(tjAlloc(w * h * NumComponents(pf)), TJDeleter) {} //
    JPEGImage& operator=(JPEGImage&& i) {
        Move(i);
        return *this;
    }
    int NumPlanes() const { return NumComponents(pixelFormat_); }
    int Width() const { return width_; }
    int Height() const { return height_; }
    TJPF PixelFormat() const { return pixelFormat_; }
    TJSAMP ChrominanceSubSampling() const { return subSampling_; }
    int Quality() const { return quality_; }
    std::shared_ptr< unsigned char > Data() const { return data_; }
    unsigned char* DataPtr() {
        return data_.get();
    }
    const unsigned char* DataPtr() const {
        return data_.get();
    }
    bool Empty() const {
        return bool(data_);
    }
    void Reset(int w, int h, TJPF pf, TJSAMP s, int quality) {
        SetParams(w, h, pf, s, quality);
        const size_t sz = std::max(tjBufSize(w, h, s),
                                   static_cast< unsigned long >(
                                       w * h * NumComponents(pf)));
        data_.reset(tjAlloc(sz),
                    TJDeleter);
        compressedSize_ = sz;
        bufferSize_ = sz;
    }
    void SetParams(size_t w, size_t h, TJPF pf, TJSAMP ss, int q) {
        width_ = w;
        height_ = h;
        pixelFormat_ = pf;
        subSampling_ = ss;
        quality_ = q;
    }
    void SetCompressedSize(size_t s) { compressedSize_ = s; }
    size_t CompressedSize() const { return compressedSize_; }
    // Size of buffer allocated by tjBuf
    void SetBufferSize(size_t s) { bufferSize_ = s; }
    size_t BufferSize() const { return bufferSize_; }
    bool operator!() const { return Empty(); }
private:
    void Move(JPEGImage& i) {
        width_ = i.width_;
        height_ = i.height_;
        pixelFormat_ = i.pixelFormat_;
        subSampling_ = i.subSampling_;
        quality_ = i.quality_;
        pitch_ = i.pitch_;
        compressedSize_ = i.compressedSize_;
        bufferSize_ = i.bufferSize_;
        data_ = std::move(i.data_);
        i.data_.reset();
        i.width_ = 0;
        i.height_ = 0;
    }
private:
    int width_;
    int height_;
    TJPF pixelFormat_;
    TJSAMP subSampling_;
    int quality_;
    int pitch_;
    size_t compressedSize_;
    size_t bufferSize_;
    std::shared_ptr< unsigned char > data_;
};

inline size_t UncompressedSize(size_t width, size_t height, TJPF pf) {
    return width * height * NumComponents(pf);
}

inline size_t UncompressedSize(const JPEGImage& i) {
    return size_t(i.Width() * i.Height() * NumComponents(i.PixelFormat()));
}
}