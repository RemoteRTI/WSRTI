// Author: Ugo Varetto
//
// websockets server: stream images to client and receive and print events
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
#include <deque>
#include "WSocketMServer.h"

namespace {
volatile bool forceExit = false;
void forceQuit(int) {
    forceExit = true;
}
}

using namespace std;

template< typename T >
class SQueue {
public:
    void Push(T&& d) {
        std::lock_guard< std::mutex > l(m_);
        queue_.push_back(std::forward< T >(d));
    }
    T Pop() {
        std::lock_guard< std::mutex > l(m_);
        T d(std::move(queue_.front()));
        queue_.pop_front();
        return d;
    }
    bool Empty() const {
        std::lock_guard< std::mutex > l(m_);
        const bool e = queue_.empty();
        return e;
    }
private:
    std::deque< T > queue_;
    mutable std::mutex m_;

};

// simple echo service:
// * received data is put into a queue
// * service extracts data from the queue and returns it to the client
int main(int argc, char** argv) {

    //queue shared between send and receive
    SQueue< vector< unsigned char > > queue;

    //callback: add received message into queue
    auto cback = [&queue](WSSTATE s, ClientId cid, void* in, int len, int, int) {
        if(s != WSSTATE::RECV) return;
        const unsigned char* begin =
            reinterpret_cast< const unsigned char* >(in);
        const unsigned char* end =
            reinterpret_cast< const unsigned char* >(in) + len;
        queue.Push(vector< unsigned char >(begin, end));
    };

    //service creation
    WSocketMServer< decltype(cback) > wso("req-rep", //protocol name
                      1000, //timeout: will spend this time to process
                            //websocket traffic, the higher the better
                      7681, //port
                      cback, //callback
                      false, //recycle memory
                      0x1000, //input buffer size
                      29);  //min interval between sends in ms
    //intercept SIGINT
    signal(SIGINT, forceQuit);
    //iterate: extract message from queue and send it back to client
    while(!forceExit) {
        this_thread::sleep_for(chrono::milliseconds(30));
        if(queue.Empty()) continue;
        const bool prePaddedOption = false;
        vector< unsigned char > v = queue.Pop();
        // Send binary
        wso.Push(v, prePaddedOption);
        // Send text
        wso.Push(v, prePaddedOption, WSMSGTYPE::TEXT);
    }
    return EXIT_SUCCESS;
}