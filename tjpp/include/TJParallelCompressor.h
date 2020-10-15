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
//along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

//ADD:
// flag support
// pitch: useful to e.g. split and compress in parallel 4k images

#include <thread>
#include <future>
#include <turbojpeg.h>

#include "JPEGImage.h"
#include "timing.h"

namespace tjpp {
template < typename C >
class TJParallelCompressor {
public:
    TJParallelCompressor(int numCompressors)
        : compressors_(numCompressors), images_(numCompressors) {}
    //UV note: current implementation spawns threads at each call, however after
    //testing with other solutions like creating threads and wait on a condition
    //variable in a loop, there does not seem to be any real gain in doing so.
    std::vector< JPEGImage > Compress(const unsigned char* img,
                                      int stacks,
                                      int width,
                                      int height,
                                      TJPF pf,
                                      TJSAMP ss,
                                      int quality,
                                      int offset = 0,
                                      int flags = TJFLAG_FASTDCT,
                                      int pitch = 0) {
        //won't move/reallocate if size > stacks anyway
        if(pitch != 0)
            throw std::logic_error("Only 0 pitch supported for now");
        compressors_.resize(stacks);
        images_.resize(stacks);
        auto compress = [](C* compressor,
                           const unsigned char* img,
                           int width,
                           int height,
                           TJPF pf,
                           TJSAMP ss,
                           int quality,
                           int offset,
                           int flags,
                           int pitch,
                           JPEGImage* out ) {
            *out = compressor->Compress(img, width, height, pf, ss,
                                        quality, offset, flags, pitch);
        };
        std::vector< std::future< void > > tasks_;
        const int h = height / stacks;
        const int nc = NumComponents(pf);
        for(int s = 0; s != stacks - 1; ++s) {
            const int off = s * h * width * nc;
            tasks_.push_back(std::async(std::launch::async, compress,
                                        &compressors_[s], img,
                                        width, h, pf, ss, quality, off, flags,
                                        pitch, &images_[s]));


        }
        const size_t last = compressors_.size() - 1;
        const int lastOffset = nc * width * (stacks - 1) * h;
        const int lastHeight = height - (stacks - 1) * h;
        tasks_.push_back(
            std::async(std::launch::async, compress,
                       &compressors_[last], img,
                       width,
                       lastHeight,
                       pf, ss, quality, lastOffset, flags, pitch,
                       &(images_.back())));
        for(auto& f: tasks_) f.get();
        return images_;
    }
    //reuse data
    std::vector< JPEGImage > Compress(std::vector< JPEGImage >&& recycled,
                                      const unsigned char* img,
                                      int stacks,
                                      int width,
                                      int height,
                                      TJPF pf,
                                      TJSAMP ss,
                                      int quality,
                                      int offset = 0,
                                      int flags = TJFLAG_FASTDCT,
                                      int pitch = 0) {
        images_ = std::move(recycled);
        return Compress(img, stacks, width, height, pf, ss,
                        quality, offset, flags, pitch);
    }

private:
    std::vector< C > compressors_;
    std::vector< JPEGImage > images_;
};
}