#include "MarketDataServer.hpp"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <set>
#include <iostream>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace ws    = beast::websocket;
using tcp = asio::ip::tcp;

class WSServerImpl {
public:
    WSServerImpl(unsigned short port)
        : ioc_(1), acceptor_(ioc_), port_(port), running_(false)
    {}

    void run() {
        try {
            asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), port_);
            acceptor_.open(ep.protocol());
            acceptor_.set_option(asio::socket_base::reuse_address(true));
            acceptor_.bind(ep);
            acceptor_.listen(asio::socket_base::max_listen_connections);

            running_.store(true);
            do_accept();

            // Run the io_context in this thread until stopped
            ioc_.run();
        } catch (std::exception& e) {
            std::cerr << "[MarketDataServer] run exception: " << e.what() << std::endl;
        }
    }

    void stop() {
        try {
            running_.store(false);
            // close acceptor to break from accept loop
            asio::post(ioc_, [this](){ acceptor_.close(); });
            // close all websockets
            std::lock_guard<std::mutex> g(sessions_mtx_);
            for (auto& wptr : sessions_) {
                if (auto sp = wptr.lock()) {
                    beast::error_code ec;
                    sp->next_layer().close(ec);
                }
            }
            ioc_.stop();
        } catch(...) {}
    }

    void broadcast_line(const std::string& line) {
        // send line to all sessions (async)
        std::lock_guard<std::mutex> g(sessions_mtx_);
        for (auto it = sessions_.begin(); it != sessions_.end(); ) {
            auto wptr = *it;
            if (auto sp = wptr.lock()) {
                // write synchronously on strand to keep it simple
                asio::post(sp->get_executor(), [sp, line]() {
                    beast::error_code ec;
                    ws::opcode op = ws::opcode::text;
                    sp->text(true);
                    sp->write(asio::buffer(line), ec);
                    if (ec) {
                        // best-effort logging; ignore
                    }
                });
                ++it;
            } else {
                it = sessions_.erase(it);
            }
        }

        // Always echo to stderr for logs
        std::cerr << line << std::endl;
    }

private:
    // session: shared_ptr to websocket stream
    using wsstream_t = ws::stream<beast::tcp_stream>;
    using wsstream_ptr = std::shared_ptr<wsstream_t>;

    void do_accept() {
        acceptor_.async_accept(asio::make_strand(ioc_),
            [this](beast::error_code ec, tcp::socket socket) {
                if (ec) {
                    if (running_.load()) {
                        std::cerr << "[MarketDataServer] accept error: " << ec.message() << std::endl;
                        // continue accepting
                        do_accept();
                    }
                    return;
                }
                // Create ws stream and perform handshake
                auto session = std::make_shared<wsstream_t>(std::move(socket));
                // accept handshake
                session->async_accept(
                    [this, session](beast::error_code ec) {
                        if (ec) {
                            std::cerr << "[MarketDataServer] ws accept error: " << ec.message() << std::endl;
                            return;
                        }
                        // store weak ptr
                        {
                            std::lock_guard<std::mutex> g(sessions_mtx_);
                            sessions_.insert(session);
                        }
                        // (We don't read from client; only broadcast)
                        // Keep session alive; if client disconnects
                        // the shared_ptr will be released later.
                    });
                // continue accepting
                do_accept();
            });
    }

    asio::io_context ioc_;
    tcp::acceptor acceptor_;
    unsigned short port_;
    std::atomic<bool> running_;
    std::thread thread_;
    std::set<std::weak_ptr<wsstream_t>, std::owner_less<std::weak_ptr<wsstream_t>>> sessions_;
    std::mutex sessions_mtx_;
};

// singleton holder
static std::unique_ptr<WSServerImpl> g_server;
static std::thread g_server_thread;

void MarketDataServer::start(unsigned short port) {
    if (g_server) return;
    g_server = std::make_unique<WSServerImpl>(port);
    g_server_thread = std::thread([&]() { g_server->run(); });
    // give it a tiny moment to start (non-blocking)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cerr << "[MarketDataServer] started on port " << port << std::endl;
}

void MarketDataServer::stop() {
    if (!g_server) return;
    g_server->stop();
    if (g_server_thread.joinable()) g_server_thread.join();
    g_server.reset();
    std::cerr << "[MarketDataServer] stopped\n";
}

void MarketDataServer::broadcast(const std::string& json) {
    if (g_server) {
        g_server->broadcast_line(json);
    } else {
        // fallback: just print to stderr
        std::cerr << json << std::endl;
    }
}
