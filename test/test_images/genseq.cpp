#include <cstdlib>
#include <string>
#include <iostream>

//imagemagick required and in path
//compile with -std=c++11 option


using namespace std;


int main(int argc, char** argv) {
    if(argc < 4) {
        cout << "usage: " << argv[0] 
             << "<number of frames> <input file> <out prefix>" << endl;
        return 0;
    }
    const int numFrames = stoi(argv[1]);
    int numDigits = 0;
    int n = numFrames;
    while(n > 0) {
        n /= 10;
        ++numDigits;
    }
    for(int i = 0; i != numFrames; ++i) {
        const string frame = to_string(i);
        const int zerosToAdd = numDigits - frame.size();
        string fname = argv[3];
        for(int z = 0; z != zerosToAdd; ++z) fname += "0";
        fname += frame + ".jpg";
        const string command = "convert " 
                     + string(argv[2]) 
                     + string(" -pointsize 128 -draw \"gravity center fill yellow text 0,0 ") 
                     + "'" + frame + "'"
                     + string("\" -append ")
                     + fname;
        const int err =system(command.c_str());
        if(err != 0) return err;
    }
    return 0;
}    
