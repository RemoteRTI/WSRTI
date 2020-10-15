#pragma once
//
// Created by Ugo Varetto on 9/9/16.
//
// libwebsockets based server:
// - forward received data to user-provided callback
// - send data to individual clients or broadcast to all
// - offer option to reuse memory by storing the consumed data buffer into
//   a memory pool accessible from client code
//
#include <string>
#include <vector>
#include <mutex>
#include <cstring>
#include <chrono>
#include <thread>
#include <deque>
#include <utility>
#include <memory>
#include <future>
#include <map>
#include <cassert>
#include <libwebsockets.h>

//Note: use libev if possible

///Cliend id. Matches the void* type received in libwebsockets callback function
using ClientId = const void*;
inline ClientId BroadcastId() { return ClientId(size_t(-1)); }

///Websocket communication status, \c SEND not currently used.
enum class WSSTATE {SEND, RECV, CONNECT, DISCONNECT};
//Websocket message type
enum class WSMSGTYPE {TEXT = LWS_WRITE_TEXT, BINARY = LWS_WRITE_BINARY};
///Websockets server
/// \tparam CBackT callback invoked at connection, disconnection and receive
/// time.
///Data is sent to connected clients through the Push method.
template < typename CBackT >
class WSocketMServer {
private:
    using ByteArray = std::vector< unsigned char >;
    using BAPtr = std::shared_ptr< ByteArray >;
    using ClientQueues = std::map< ClientId, std::deque< BAPtr > >;
public:
    WSocketMServer() = delete;
    WSocketMServer(const WSocketMServer&) = delete;
    WSocketMServer(WSocketMServer&&) = delete;
    WSocketMServer(const std::string& protocolName,
                   int timeout,
                   int port,
                   const CBackT& cb,
                   bool recycleMemory,
                   size_t recvBufferSize = 0x10000,
                   int frameTime = 30, //ms
                   bool atomicMessages = true)
        : protocolName_(protocolName),
          stop(false), cback(cb), frameTime_(frameTime), 
          recycleMemory_(recycleMemory), atomicMessages_(atomicMessages) {
        Init(timeout, port, recvBufferSize);
    }
    const std::string& Name() const { return protocolName_; }
    ~WSocketMServer() {
        Stop();
    }
    ///Return sequence of client ids.
    std::vector< ClientId > Clients() {
        std::lock_guard< std::mutex > l(clientQueueGuard_);
        std::vector< ClientId > cid;
        for(auto& e: clientQueues_) cid.push_back(e.first);
        return cid;
    }
    ///Push data into send queue.
    /// \param d data
    /// \param prePadded \c true if data is already prepadded with \c LWS_PRE
    /// \param id client to send to, if == broadcast id send to all
    /// connected clients
    /// bytes, \c false otherwise
    void Push(const ByteArray& d,
              bool prePadded,
              WSMSGTYPE writeMode = WSMSGTYPE::BINARY,
              ClientId id = BroadcastId()) {
        if(id != BroadcastId() && !ClientInQueue(id))
            throw std::logic_error("Requested client id not valid");
        std::pair< PerSendData, BAPtr > p;
        p.first.writeMode = writeMode;
        if(prePadded) p.second.reset(new ByteArray(d));
        else  {
            ByteArray* v = new ByteArray(PrePaddingSize() + d.size());
            memmove(v->data() + PrePaddingSize(), d.data(), d.size());
            p.second.reset(v);
        } 
        std::lock_guard< std::mutex > l(clientQueueGuard_);
        if(id == BroadcastId()) {
            for(auto& q: clientQueues_) q.second.push_back(p);
        } else {
            clientQueues_[id].push_back(p);
        }
    }
    ///Push prepadded buffer stored into a \c shared_ptr
    ///This is the preferred way of passing data to the object since internally
    ///only \c shared_ptr objects are used.
    void PushPrePaddedPtr(BAPtr ptr, 
                          WSMSGTYPE writeMode = WSMSGTYPE::BINARY,
                          ClientId id = BroadcastId()) {
        if(id != BroadcastId() && !ClientInQueue(id))
            throw std::logic_error("Requested client id not valid");
        std::pair< PerSendData, BAPtr > p;
        p.first.writeMode = writeMode;
        p.second = ptr;
        std::lock_guard< std::mutex > l(clientQueueGuard_);
        if(id == BroadcastId()) {
            for(auto& q: clientQueues_) q.second.push_back(p);
        } else {
            clientQueues_[id].push_back(p);
        }
    }
    ///Return \c shared_ptr pointing to an \c std::vector of the requested size.
    ///The returned object is picked from a pool of objects received through
    ///the Push method or a new one is created if the pool is empty.
    ///Data is already pre-padded and client code shall write at PrePaddingSize
    ///offset.
    ///Using this method to retrieve a buffer allows to avoid unnecessary memory
    ///allocations since the same memory buffer is reused multiple times.
    ///Reusing memory buffers is enabled only when the object is constructed
    ///with a \c recycleMemory flag set to true.
    std::shared_ptr< std::vector< unsigned char > >
    GetConsumedPaddedPtr(size_t sz) {
        std::lock_guard< std::mutex > l(consumedQueueMutex_);
        if(consumedQueue_.empty())
            return std::shared_ptr< std::vector< unsigned char > >(
                new std::vector< unsigned char >(sz)
            );
        std::shared_ptr< std::vector< unsigned char > > p
            = consumedQueue_.front();
        consumedQueue_.pop_front();
        p->resize(sz);
        return p;
    }
    ///Returns \c true if the pool of consumed buffer is empty.
    bool ConsumedQueueEmpty() {
        std::lock_guard< std::mutex > l(consumedQueueMutex_);
        const bool e = consumedQueue_.empty();
        return e;
    }
    ///Minimum time in milliseconds between subsequent sends.
    int FrameTime() const { return frameTime_; }
    ///Number of connected clients. \warning not synchronized
    size_t ConnectedClients() const { return clientQueues_.size(); }
    ///Size of pre-padded region.
    const size_t PrePaddingSize() const { return LWS_PRE; }
private:
    ///Per-session data: created when a client connects, destroyed when
    ///it disconnects.
    struct PerSessionData {
        ///Time of previous send or creation time in case of first send.
        std::chrono::steady_clock::time_point prev;

    };
    ///Per-send data: created every time user pushes to their send queue
    struct PerSendData {
        ///lws write protocol to use when sending
        WSMSGTYPE writeMode;
    };
private:
    ///Check if client id in queue.
    bool ClientInQueue(ClientId id) const {
        return clientQueues_.find(id) != clientQueues_.end();
    }
    ///Add consumed (sent) buffer to memory pool.
    void PushConsumedPaddedPtr(BAPtr p) {
        std::lock_guard< std::mutex > l(consumedQueueMutex_);
        if(p.unique())
            consumedQueue_.push_back(p);
    }
    ///Initialize libwebsockets and start service loop in separate thread
    void Init(int timeout,
              int port,
              size_t recvBufferSize) {
        protocols = {
            //if using http put the http handler as the first entry in the table
            {
                protocolName_.c_str(),	//protocol name
                WSocketMServer::Callback, //callback
                sizeof(PerSessionData),	// per session data size
                recvBufferSize,// , //receive buffer size
                0, //ID
                nullptr
            },
            {
                nullptr, nullptr, 0, 0, 0, nullptr //END
            }
        };
        memset(&info, 0, sizeof info);
        info.port = port;
        info.iface = nullptr;

        info.protocols = &protocols[0];
        info.user = this;

        //warnings if following not set to something
        info.gid = -1;
        info.uid = -1;

        context = lws_create_context(&info);
        wsocketsTask = std::async(std::launch::async, [this, timeout]() {
            while(!this->stop && lws_service(this->context, timeout) >= 0);
        });
    }
    ///Stop service loop.
    void Stop() {
        stop = true;
        wsocketsTask.get();
    }
private:
    ///protocol name
    std::string protocolName_;
    ///libwebsockets context.
    lws_context* context;
    ///libwebsockets context creation data.
    lws_context_creation_info info;
    ///Per client send queue.
    std::map< ClientId, std::deque< std::pair< PerSendData, BAPtr > > > clientQueues_;
    ///Sync access to clientQueues_.
    std::mutex clientQueueGuard_;
    ///Set to \c true to stop service.
    bool stop;
    ///Service task.
    std::future< void > wsocketsTask;
    ///libwebsockets protocols.
    std::vector< lws_protocols > protocols;
    ///Client-provided callback.
    CBackT cback;
    ///Time in ms between subsequent writes.
    int frameTime_;
    ///Pool of consumed memory buffer to be reused by client code.
    std::deque< std::shared_ptr< std::vector< unsigned char > > >
        consumedQueue_;
    ///Guard access to \c consumedQueue_ from different threads.
    std::mutex consumedQueueMutex_;
    ///If set to \c true the send buffer is added into the memory pool
    ///to be reused by client code.
    bool recycleMemory_;
    ///If set to @c true it waits until all frames are received before
    ///invoking the client callback
    bool atomicMessages_;
    ///Per-client receive buffer, used for constructing atomic messages
    std::map< void*, std::vector< char > > clientBuffers_;
    ///Guard access to client buffers
    std::mutex clientBufferGuard_;
private:
    ///libwebsockets callback.
    static int
    Callback(lws *wsi,
             lws_callback_reasons reason,
             void* user,
             void* in,
             size_t len) {
        PerSessionData *pss = reinterpret_cast< PerSessionData* >(user);
        lws_context* context = lws_get_context(wsi);
        WSocketMServer* wso =
            reinterpret_cast< WSocketMServer* >(
                lws_context_user(context));
        switch(reason) {
        case LWS_CALLBACK_SERVER_WRITEABLE: {
            BAPtr p;
            PerSendData psd;
            {
                std::lock_guard< std::mutex > l(wso->clientQueueGuard_);
                if(!wso->ClientInQueue(ClientId(user))) {
                    lws_callback_on_writable(wsi);
                    break;
                }
                std::deque< std::pair< PerSendData, BAPtr > >* q = &wso->clientQueues_[ClientId(user)];
                if(q->empty()) {
                    lws_callback_on_writable(wsi);
                    break;
                }
                psd = q->front().first;
                p = q->front().second;
                q->pop_front();
            }
            lws_write_protocol writeMode = static_cast< lws_write_protocol >(psd.writeMode);
            const int sent =
                lws_write(wsi, p->data() + LWS_PRE, p->size() - LWS_PRE,
                          writeMode);
            if(wso->recycleMemory_)
                wso->PushConsumedPaddedPtr(p);
            if(sent < 0) {
                lwsl_err("ERROR %d writing to socket, hanging up\n", sent);
                return -1;
            }
            if(sent < p->size() - LWS_PRE) {
                lwsl_err("Partial write\n");
                return -1;
            }
            const std::chrono::milliseconds d =
                std::chrono::duration_cast< std::chrono::milliseconds >
                    (std::chrono::steady_clock::now() - pss->prev);
            const auto ft =
                std::chrono::milliseconds(wso->FrameTime());
            if(d < ft) std::this_thread::sleep_for(ft - d);
            pss->prev = std::chrono::steady_clock::now();
            lws_callback_on_writable(wsi);
        }
            break;
        case LWS_CALLBACK_RECEIVE: {
            const bool finalFrag = bool(lws_is_final_fragment(wsi));
            const bool binary = lws_frame_is_binary(wsi);
            if(!finalFrag && wso->atomicMessages_) {
              std::lock_guard< std::mutex > l(wso->clientBufferGuard_);
              std::vector< char >& b = wso->clientBuffers_[user];
              b.reserve(b.size() + len);
              b.insert(b.end(),
                       reinterpret_cast< const char* >(in),
                       reinterpret_cast< const char* >(in) + len);
            } else if(finalFrag && wso->atomicMessages_) {
              std::lock_guard< std::mutex > l(wso->clientBufferGuard_);
              std::vector< char >& b = wso->clientBuffers_[user];
              b.insert(b.end(),
                       reinterpret_cast< const char* >(in),
                       reinterpret_cast< const char* >(in) + len);
              wso->cback(WSSTATE::RECV, user, &b[0], b.size(), true, binary);
              b.resize(0);
            } else {
              wso->cback(WSSTATE::RECV, user,
                         reinterpret_cast< char* >(in), size_t(len),
                         finalFrag,
                         binary);
            }
            lws_callback_on_writable(wsi);
          }
            break;
        case LWS_CALLBACK_ESTABLISHED: {
            std::lock_guard< std::mutex > l(wso->clientQueueGuard_);
            assert(!wso->ClientInQueue(user));
            wso->clientQueues_[user] = std::deque< std::pair< PerSendData, BAPtr > >();
            if(wso->atomicMessages_) {
              wso->clientBuffers_[user] = std::vector< char >();
            }
            wso->cback(WSSTATE::CONNECT, user, nullptr, 0, false, false);
            lws_callback_on_writable(wsi);
        }
            break;
        case LWS_CALLBACK_CLOSED: {
            std::lock_guard< std::mutex > l(wso->clientQueueGuard_);
            assert(wso->ClientInQueue(user));
            wso->clientQueues_.erase(wso->clientQueues_.find(user));
            wso->cback(WSSTATE::DISCONNECT, user, nullptr, 0, false, false);
            if(wso->atomicMessages_) {
              std::lock_guard< std::mutex > l2(wso->clientBufferGuard_);
              wso->clientBuffers_.erase(wso->clientBuffers_.find(user));
            }
        }
            break;
        default:
            break;
        }
        return 0;
    }
};
