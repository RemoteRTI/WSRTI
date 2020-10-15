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
#include <vector>
#include "pixelformat.h"

namespace tjpp {
class Image {
public:
    Image() : width_(0), height_(0), pixelFormat_(PixelFormat()) {}
    Image(const std::vector< unsigned char >& data,
          size_t width, size_t height, TJPF pf) :
        data_(data), width_(width), height_(height), pixelFormat_(pf) {}
    Image(const Image&) = default;
    Image& operator=(const Image&) = default;
    Image(Image&& i) {
        width_ = i.width_;
        height_ = i.height_;
        pixelFormat_ = i.pixelFormat_;
        data_ = std::move(i.data_);
        i.width_ = 0;
        i.height_ = 0;
    }
    Image& operator=(Image&& i) {
        width_ = i.width_;
        height_ = i.height_;
        pixelFormat_ = i.pixelFormat_;
        data_ = std::move(i.data_);
        i.width_ = 0;
        i.height_ = 0;
        return *this;
    }
    size_t Width() const { return width_; }
    size_t Height() const { return height_; }
    TJPF PixelFormat() const { return pixelFormat_; }
    std::vector< unsigned char > Data() const { return data_; }
    const unsigned char* DataPtr() const { return data_.data(); }
    unsigned char* DataPtr() { return data_.data(); }
    int NumPlanes() const { return NumComponents(pixelFormat_); }
    size_t Size() const { return width_ * height_ * NumPlanes(); }
    size_t AllocatedSize() const { return data_.size(); }
    void Allocate(size_t sz) {
        data_.resize(sz);
    }
    void SetParameters(size_t w, size_t h, TJPF pf) {
        width_ = w;
        height_ = h;
        pixelFormat_ = pf;
    }
private:
    size_t width_;
    size_t height_;
    TJPF pixelFormat_;
    std::vector< unsigned char > data_;
};
}
