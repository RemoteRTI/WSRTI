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
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include "TJCompressor.h"
#include "TJMemPoolCompressor.h"
#include "TJDeCompressor.h"
#include "TJParallelCompressor.h"
#include "TJParallelDeCompressor.h"

#ifdef TIMING__
#include "timing.h"
#endif


using namespace std;
using namespace tjpp;

size_t FileSize(const string& fname) {
    ifstream file(fname);
    assert(file);
    file.ignore( std::numeric_limits<std::streamsize>::max() );
    std::streamsize length = file.gcount();
    file.clear();   //  Since ignore will have set eof.
    //file.seekg( 0, std::ios_base::beg );
    return length;
}


std::vector< JPEGImage >
TestJPGParallelCompressor(const unsigned char* uimg,
                          int width,
                          int height,
                          TJPF pf,
                          TJSAMP ss,
                          int quality,
                          int numStacks) {


    TJParallelCompressor< TJCompressor > mc(numStacks);
#ifdef TIMING__
    Time begin = Tick();
#endif
    std::vector< JPEGImage > images = mc.Compress(uimg, numStacks, width,
                                                  height, pf, ss, quality);
#ifdef TIMING__
    Time end = Tick();
    cout << "multi - compression time: " << toms(end - begin).count() << endl;
#endif
    for(int i = 0; i != images.size(); ++i) {
        const string fname = "mout" + to_string(i) + ".jpg";
        ofstream os(fname, ios::binary);
        assert(os);
        assert(images[i].DataPtr());
        os.write((char*)images[i].DataPtr(), images[i].CompressedSize());
        //assert(images[i].Empty()); //moved!
    }
    return images;
}

//note: very important to pre-allocate memory, especially for 4k images
void TestJPGParallelDeCompressor(const vector< JPEGImage >& imgs) {
    const size_t globalHeight
        = std::accumulate(begin(imgs),
                          end(imgs),
                          size_t(0),
                          [](size_t prev, const JPEGImage& i) {
                             return prev + i.Height();
                          });
    const size_t globalWidth = size_t(imgs.front().Width());
    const int numComp = imgs.front().NumPlanes();
    TJParallelDeCompressor
        mc(imgs.size(), numComp * globalHeight * globalWidth);
#ifdef TIMING__
    Time begin = Tick();
#endif
    Image img = mc.DeCompress(imgs);
#ifdef TIMING__
    Time end = Tick();
    cout << "multi - decompression time: " << toms(end - begin).count() << endl;
#endif
    TJCompressor c;
    JPEGImage jimg = c.Compress(img.DataPtr(),
                                int(img.Width()),
                                int(img.Height()),
                                imgs.front().PixelFormat(),
                                TJSAMP_420,
                                50);
    const string fname = "mout.jpg";
    ofstream os(fname, ios::binary);
    assert(os);
    os.write((char*)jimg.DataPtr(), jimg.CompressedSize());

}

void TestJPGMemPoolCompressor(const unsigned char* uimg,
                              int width,
                              int height,
                              TJPF pf,
                              TJSAMP ss,
                              int quality,
                              int numImages) {
    TJMemPoolCompressor tjc;
    for(int i = 0; i != numImages; ++i) {
//        TJMemPoolCompressor::JPEGImageWrapper iw = tjc.Compress(uimg,
//                                              width, height, pf, ss, quality);
        //Compress returns a image wrapper which gets automatically converted
        //to an image const ref
        JPEGImage img = tjc.Compress(uimg,
                                     width, height, pf, ss, quality);
        const string fname = "out" + to_string(i) + ".jpg";

        //JPEGImage img = iw.Image();
        ofstream os(fname, ios::binary);
        assert(os);
        assert(img.DataPtr());
        os.write((char*)img.DataPtr(), img.CompressedSize());
        //tjc.PutBack(img);
        assert(img.Empty()); //moved!
    }
}


int TestCompressorAndDecompressor(int argc, char** argv) {
    //read
    if(argc < 4 ){
        cerr << "usage: " << argv[0]
             << " <jpeg file> <quality=[0,100]> <num threads>" << endl;
        return EXIT_FAILURE;
    }
    const size_t length = FileSize(argv[1]);
    using Byte = unsigned char;
    vector< Byte > input(length);
    ifstream is(argv[1], ios::binary);
    is.read((char*) input.data(), input.size());
    //Image img;
    //decompress
    TJDeCompressor decomp(1920 * 1080 * 4 * 4); //up to RGBX 4k support
#ifdef TIMING__
    Time begin = Tick();
#endif
    //move semantics for both constructor and assignment operator
    Image img;
    img = decomp.DeCompress(input.data(), input.size(), TJPF_BGR);
#ifdef TIMING__
    Time end = Tick();
#endif
    //compress
    const int quality = strtol(argv[2], nullptr, 10);
    TJCompressor comp;
#ifdef TIMING__
    Time begin2 = Tick();
#endif
    JPEGImage jpegImage =
        comp.Compress(img.DataPtr(), img.Width(), img.Height(),
                      img.PixelFormat(), TJSAMP_420, quality);
#ifdef TIMING__
    Time end2 = Tick();
#endif
    //write
    ofstream os("out.jpg", ios::binary);
    assert(os);
    os.write((const char*) jpegImage.DataPtr(), jpegImage.CompressedSize());
    os.flush();

#ifdef TIMING__
    Duration elapsed = end - begin;
    cout << "Decompression time: "
         << toms(elapsed).count()
         << "ms" << endl;

    Duration elapsed2 = end2 - begin2;
    cout << "Compression time: "
         << toms(elapsed2).count()
         << "ms" << endl;
#endif

//    TestJPGMemPoolCompressor(img.DataPtr(), img.Width(), img.Height(),
//                             FromCS(img.PixelFormat()), TJSAMP_420, 50, 10);

    const int numThreads = strtol(argv[3], nullptr, 10);
    assert(numThreads > 0);
    std::vector< JPEGImage > stacks =
        TestJPGParallelCompressor(img.DataPtr(), img.Width(), img.Height(),
                                  img.PixelFormat(), TJSAMP_420,
                                  quality,
                                  numThreads);
    TestJPGParallelDeCompressor(stacks);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
   return TestCompressorAndDecompressor(argc, argv);

}