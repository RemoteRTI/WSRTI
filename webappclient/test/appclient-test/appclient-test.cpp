// Author: Ugo Varetto
//
// test sever for remote websocket app client
//

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <future>
#include <stdexcept>
#include <map>
#include <set>
//use unsigned char buffers
#define ZRF_UNSIGNED_CHAR
//use signed 32 integers for buffer length for compatibility with
//javascript client
#define ZRF_int32_size
#include <Serialize.h>

#include "wslog.h"
#include "WSocketMServer.h"


enum class ClientEventId { MOUSE_DOWN = 1, MOUSE_UP = 2, MOUSE_DRAG = 3,
                           KEYDOWN = 4, KEYUP = 5, MOUSE_WHEEL = 6, RESIZE = 7,
                           CMDSTRING = 8, CMDID = 9};
enum class ServerEventId {PRINT = 1,
                          RESIZE = 2,
                          FILE_DOWNLOAD = 3,
                          HELP = 4};
enum class ServerCommandId {HELP = 1, SET = 2, GET = 3,
                            ADD = 4, SAVE = 5, UPLOAD = 6};

enum class SpecialKeys {SHIFT = 16,
                        CTRL = 17,
                        ALT = 18,
                        META = 91,
                        ARROW_LEFT = 37,
                        ARROW_UP = 38,
                        ARROW_RIGHT = 39,
                        ARROW_DOWN = 40,
                        PAGE_UP = 33,
                        PAGE_DOWN = 34,
                        HOME = 36,
                        END = 35,
                        BACKSPACE = 8,
                        DELETE = 46,
                        ENTER = 13,
                        CLEAR = 12,
                        CAPS_LOCK = 20,
                        ESCAPE = 27};
                        //F1 == 112, F2 == 113 etc
enum class MouseButtons{LEFT = 0x0, MIDDLE = 0x1, RIGHT = 0x2};
std::string ToKeyString(char k, bool special) {
  using K = SpecialKeys;
  static std::map< K, std::string > k2s = {
    {K::SHIFT, "Shift"},
    {K::CTRL, "Ctrl"},
    {K::ALT, "Alt"},
    {K::META, "Meta"},
    {K::ARROW_LEFT, "Arrow left"},
    {K::ARROW_UP, "Arrow up"},
    {K::ARROW_RIGHT, "Arrow right"},
    {K::ARROW_DOWN, "Arrow down"},
    {K::PAGE_UP, "Page up"},
    {K::PAGE_DOWN, "Page down"},
    {K::HOME, "Home"},
    {K::END, "End"},
    {K::BACKSPACE, "Backspace"},
    {K::DELETE, "Delete"},
    {K::ENTER, "Enter"},
    {K::CLEAR, "Clear"},
    {K::CAPS_LOCK, "Caps lock"}
  };

  if(special && (k2s.find(K(k)) != k2s.end())) return k2s[K(k)];
  return std::string(&k, &k + 1);
}

bool Modifier(int code) {
  using S = SpecialKeys;
  static std::set< SpecialKeys > sk = {S::SHIFT, S::CTRL, S::ALT, S::META};
  return sk.find(SpecialKeys(code)) != sk.end();
}


namespace {
volatile bool forceExit = false;
}

void forceQuit(int) {
    forceExit = true;
}

using namespace std;

string EventToStr(ClientEventId id) {
    using C = ClientEventId;
    static map< ClientEventId, string > e2s = {
        {C::MOUSE_DOWN, "mouse down"},
        {C::MOUSE_UP,   "mouse up"},
        {C::MOUSE_DRAG, "mouse drag"},
        {C::KEYDOWN,    "key down"},
        {C::KEYUP,      "key up"},
        {C::MOUSE_WHEEL,"mouse wheel"},
        {C::RESIZE,     "resize window"},
        {C::CMDSTRING,  "command text"},
        {C::CMDID,      "command id"}
    };
    return e2s[id];
}

string CommandToStr(ServerCommandId id) {
    using S = ServerCommandId;
    static map< ServerCommandId, string > c2s = {
        {S::SAVE, "save"},
        {S::UPLOAD, "upload"},
    };
    return c2s[id];
}

size_t FileSize(const string& fname) {
    ifstream file(fname);
    assert(file);
    file.ignore(std::numeric_limits< std::streamsize >::max());
    std::streamsize length = file.gcount();
    //file.clear();   //  Since ignore will have set eof.
    return length; //closing on exit, no need to clear
}

int main(int argc, char** argv) {
    if(argc < 4) {
        cerr << "usage: " << argv[0] << " <prefix> <num images> <suffix> [fps]"
             << endl;
        cerr << "E.g.: '>" << argv[0] << " frame 10 .jpg 30' loops over "
             << "frame0.jpg to frame9.jpg sending 30 pictures per second"
             << endl;
        return EXIT_FAILURE;
    }
    //comment following line to enable log to stdout/err
    InitWSThrowErrorHandler();
    //copy images to memory
    const string prefix = argv[1];
    const int numfiles = int(stol(argv[2]));
    const string suffix = argv[3];
    const int fps = argc == 5 ? stol(argv[4]) : 30;
    const int frameDigits = int(floor(log10(numfiles))) + 1;
    using Image = vector< unsigned char >;
    using ImageArray = vector< shared_ptr< Image > >;
    ImageArray imgs;
    for(int i = 0; i != numfiles; ++i) {
        const size_t padding = i == 0 ? frameDigits - 1 :
            frameDigits - (int(floor(log10(i))) + 1);
        const string fname =
            prefix
            + string(padding, '0')
            + to_string(i)
            + suffix;
        cout << fname << endl;
        ifstream is(fname, ios::binary);
        assert(is);
        const size_t fileSize = FileSize(fname);
        assert(fileSize);
        shared_ptr< Image > img(new Image(fileSize + LWS_PRE));
        is.read(reinterpret_cast< char* >(img->data() + LWS_PRE), fileSize);
        imgs.push_back(img);
    }
    bool sendFile = false;
    ClientId sendFileClient = ClientId(0);
    //callback invoked each time data is received
    auto controlStreamCBack =
      [&sendFile, &sendFileClient]
      (WSSTATE s, ClientId cid, const char* in, size_t len,
        bool /*isFinalFragment*/, bool /*isBinary*/) {
        if(s == WSSTATE::CONNECT) {
            cout << cid << "> Control service connected" << endl;
            return;
        } else if(s == WSSTATE::DISCONNECT) {
            cout << cid << "> Control service disconnected" << endl;
            return;
        }
        const int* p = reinterpret_cast< const int* >(in);
        const string es = EventToStr(ClientEventId(*p));
        if(es.empty()) return;
        cout << cid << "> " << es;
        using C = ClientEventId;
        switch(ClientEventId(*p)) {
        case C::MOUSE_DOWN:
        case C::MOUSE_UP:
        case C::MOUSE_DRAG: {
            const int* pos = ++p;
            //first byte is mouse buttons
            const char& buttons = *reinterpret_cast< const char* >(pos + 2);
            //second byte is keyboard modifiers
            const char& modifiers =
              *(reinterpret_cast< const char* >(pos + 2) + 1);
            cout << " x: " << *pos << " y: " << *(pos + 1)
                 << " buttons: " << int(buttons)
                 << " modifiers: ";
            if(0x01 & modifiers) cout << " Alt";
            if(0x02 & modifiers) cout << " Ctrl";
            if(0x04 & modifiers) cout << " Meta";
            if(0x08 & modifiers) cout << " Shift";
        }
        break;
        case C::RESIZE: {
            const int* pos = ++p;
            cout << " width: " << *pos << " height: " << *(pos + 1) << endl;
        }
            break;
        case C::KEYDOWN:
        case C::KEYUP: {
            const int* pos = ++p;
            const int modifier = *(pos + 1);
            const string key = ToKeyString(char(*pos), bool(0x20 & modifier));

            cout << " key: " << key << " ";
            string location = "";

            //if special and modifier
            if((0x20 & modifier) && Modifier(*pos))  {
              if(0x10 & modifier) location = " Right ";
              else location =  " Left ";
            }
            cout << location;
            if(0x01 & modifier) cout << "Alt";
            if(0x02 & modifier) cout << "Ctrl";
            if(0x04 & modifier) cout << "Meta";
            if(0x08 & modifier) cout << "Shift";

            cout << endl;
        }
        break;
        case C::MOUSE_WHEEL: {
            const int* pos = ++p;
            cout << " wheel: " << *pos << endl;
        }
        break;
        case C::CMDSTRING: {
            const int length = *(p + 1);
            const char* begin = reinterpret_cast< const char* >(p + 2);
            string text(begin, begin + length);
            cout << ": " << text << endl;
        }
        break;
        case C::CMDID: {
          const int id = *(p + 1);
          cout << ": " << CommandToStr(ServerCommandId(id)) << endl;
          if(id == int(ServerCommandId::UPLOAD)) {
            const int length = *(p + 2);
            const char* buf = reinterpret_cast< const char* >(in)
                              + 3 * sizeof(int);
            const vector< char > data(buf, buf + length);
            const char* downloadFilePath = "tmp-download";
            ofstream os(downloadFilePath, ios::binary);
            os.write(&data[0], data.size());
            cout << "\nFile received - "
                 << length << " bytes - saved to " << downloadFilePath
                 << endl;
          } else if(id == int(ServerCommandId::SAVE)) {
              sendFile = true;
              sendFileClient = cid;
          }
        }
        break;
        default:
            break;
        }
      };
    auto imageStreamCBack =
      [](WSSTATE s, ClientId cid, const char* in, size_t len,
         bool /*isFinalFragment*/, bool /*isBinary*/) {
        if(s == WSSTATE::CONNECT) {
            cout << cid << "> Image streaming service connected" << endl;
            return;
        } else if(s == WSSTATE::DISCONNECT) {
            cout << cid << "> Image streaming service disconnected" << endl;
            return;
        }
      };

    const chrono::microseconds sendInterval (int(1E6) / fps); //us/fps
    //websockets

    //configuration
    const string wshost = "localhost";
    const string wsImageStreamProto = "appclient-image-stream-protocol";
    const string wsControlStreamProto = "appclient-control-protocol";
    const int wsImageStreamPort = 8881;
    const int wsControlStreamPort = 8882;
    const bool recycleMemoryOption = false;
    try {
        WSocketMServer< decltype(controlStreamCBack) >
          controlService(wsControlStreamProto, //protocol name
                         1000, //timeout: will spend this time to process
                               //websocket traffic, the higher the better
                         wsControlStreamPort, //port
                         controlStreamCBack, //callback
                         recycleMemoryOption, //recycle memory
                         0x1000, //input buffer size
                         //min interval between sends in ms
                         sendInterval.count() / 1000);
        WSocketMServer< decltype(imageStreamCBack) >
          imageStreamService(wsImageStreamProto, //protocol name
                             1000, //timeout: will spend this time to process
                             //websocket traffic, the higher the better
                             wsImageStreamPort, //port
                             imageStreamCBack, //  callback
                             recycleMemoryOption, //recycle memory
                             0x1000, //input buffer size
                             //min interval between sends in ms
                             sendInterval.count() / 1000);
        signal(SIGINT, forceQuit);
        //send images from main thread
        int count = 0;
        const vector< unsigned char > resize
          = srz::Pack(ServerEventId::RESIZE, 960, 540);
        chrono::high_resolution_clock::time_point start
          = chrono::high_resolution_clock::now();
        chrono::high_resolution_clock::time_point msgStart = start;
        chrono::high_resolution_clock::time_point resizeStart = start;
        const chrono::seconds textMessageInterval(5); //5s
        const chrono::seconds resizeMessageInterval(10); //10s
        while (!forceExit) {
            const auto now = chrono::high_resolution_clock::now();
            const chrono::seconds msgElapsed =
              chrono::duration_cast< chrono::seconds >(now - msgStart);
            const chrono::seconds resizeElapsed =
              chrono::duration_cast< chrono::seconds >(now - resizeStart);
            //auto const elapsed = now - start;
            this_thread::sleep_until(start + sendInterval);
            if(imageStreamService.ConnectedClients() == 0) continue;
            const bool prePaddingOption = true;
            imageStreamService.PushPrePaddedPtr(imgs[count % imgs.size()]);
            ++count;
            //send a text message
            if(msgElapsed >= textMessageInterval) {
              const vector< unsigned char > msg
                = srz::Pack(ServerEventId::PRINT, to_string(count));
              controlService.Push(msg, false);
              msgStart = now;
            }
            //send a resize message evety ~10 seconds
            if(resizeElapsed >= resizeMessageInterval) {
              controlService.Push(resize, false);
              resizeStart = now;
            }
            start = now;
            if(sendFile) {
              const string fileContent = "This is the file content";
              //client expects a buffer in the form: ID|SIZE|DATA where
              // ID and SIZE are 32 bit signed integers and DATA is a byte
              // array
              auto f = srz::Pack(ServerEventId::FILE_DOWNLOAD, fileContent);
              controlService.Push(f, false, sendFileClient);
              sendFile = false;
              sendFileClient = ClientId(0);
            }
        }
    } catch(const exception& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
