#include "MarketDataServer.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <thread>
#include <mutex>
#include <set>
#include <queue>
#include <memory>
#include <iostream>
#include <atomic>
#include <condition_variable>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace ws    = beast::websocket;
using tcp = asio::ip::tcp;

using ws_stream_t = ws::stream<beast::tcp_stream>;
using ws_ptr = std::shared_ptr<ws_stream_t>;

// Sessions set (weak ownership not needed here)
static std::set<ws_ptr> g_sessions;
static std::mutex g_sessions_mtx;

// client→server queue
static std::queue<std::string> g_client_queue;
static std::mutex g_client_queue_mtx;
static std::condition_variable g_client_queue_cv;

// io_context and listener thread
static std::unique_ptr<asio::io_context> g_ioc;
static std::unique_ptr<std::thread> g_thread;
static std::atomic<bool> g_running{false};

static void start_accept_loop(unsigned short port);

namespace MarketDataServerAPI {

    void start(unsigned short port) {
        if (g_running.load()) return;
        g_running.store(true);
        g_ioc = std::make_unique<asio::io_context>(1);

        // run acceptor in background thread
        g_thread = std::make_unique<std::thread>([port]() {
            try {
                asio::io_context& ioc = *g_ioc;
                tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), port));
                std::cerr << "[MarketDataServer] listening on ws://localhost:" << port << std::endl;

                // accept loop
                while (g_running.load()) {
                    beast::error_code ec;
                    tcp::socket socket(ioc);
                    acceptor.accept(socket, ec);
                    if (ec) {
                        if (g_running.load()) std::cerr << "[MarketDataServer] accept error: " << ec.message() << std::endl;
                        continue;
                    }

                    // create ws stream
                    auto session = std::make_shared<ws_stream_t>(std::move(socket));
                    // do synchronous accept handshake (simpler)
                    session->accept(ec);
                    if (ec) {
                        std::cerr << "[MarketDataServer] ws accept error: " << ec.message() << std::endl;
                        continue;
                    }

                    {
                        std::lock_guard<std::mutex> g(g_sessions_mtx);
                        g_sessions.insert(session);
                    }

                    // start async read loop on a detached thread per session to keep code simple
                    std::thread([session]() {
                        beast::flat_buffer buffer;
                        while (true) {
                            beast::error_code ec;
                            session->read(buffer, ec);
                            if (ec) {
                                // remove session
                                std::lock_guard<std::mutex> g(g_sessions_mtx);
                                g_sessions.erase(session);
                                break;
                            }
                            // extract string
                            std::string msg = beast::buffers_to_string(buffer.data());
                            buffer.consume(buffer.size());
                            // push into queue
                            {
                                std::lock_guard<std::mutex> ql(g_client_queue_mtx);
                                g_client_queue.push(msg);
                            }
                            g_client_queue_cv.notify_one();
                        }
                    }).detach();
                }
            } catch (std::exception &e) {
                std::cerr << "[MarketDataServer] thread exception: " << e.what() << std::endl;
            }
        });
    }

    void broadcast(const std::string& msg) {
        // echo to stderr for logs
        std::cerr << msg << std::endl;

        std::lock_guard<std::mutex> g(g_sessions_mtx);
        for (auto it = g_sessions.begin(); it != g_sessions.end();) {
            auto s = *it;
            if (!s) { it = g_sessions.erase(it); continue; }
            beast::error_code ec;
            // text mode
            s->text(true);
            s->write(asio::buffer(msg), ec);
            if (ec) {
                // drop this session
                it = g_sessions.erase(it);
            } else ++it;
        }
    }

    bool try_pop_client_message(std::string &out) {
        std::lock_guard<std::mutex> g(g_client_queue_mtx);
        if (g_client_queue.empty()) return false;
        out = std::move(g_client_queue.front());
        g_client_queue.pop();
        return true;
    }

    void stop() {
        g_running.store(false);
        // stop io_context by creating a connection to break accept() if blocked
        if (g_ioc) {
            try {
                g_ioc->stop();
            } catch(...) {}
        }
        if (g_thread && g_thread->joinable()) {
            g_thread->join();
        }
        // close sessions
        std::lock_guard<std::mutex> g(g_sessions_mtx);
        g_sessions.clear();
        // clear queue
        std::lock_guard<std::mutex> q(g_client_queue_mtx);
        while (!g_client_queue.empty()) g_client_queue.pop();
    }
}
