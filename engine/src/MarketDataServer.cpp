#include "MarketDataServer.hpp"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <set>
#include <iostream>

namespace beast = boost::beast;
namespace ws = beast::websocket;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

static std::set<std::shared_ptr<ws::stream<beast::tcp_stream>>, 
                std::owner_less<std::shared_ptr<ws::stream<beast::tcp_stream>>>> clients;

static std::mutex clients_mtx;
static std::thread server_thread;
static asio::io_context ioc;

class WSListener : public std::enable_shared_from_this<WSListener> {
public:
    tcp::acceptor acceptor;
    WSListener(asio::io_context& ioc, unsigned short port)
        : acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {}

    void run() { do_accept(); }

private:
    void do_accept() {
        acceptor.async_accept([self = shared_from_this()](beast::error_code ec, tcp::socket socket){
            if (!ec) {
                auto session = std::make_shared<ws::stream<beast::tcp_stream>>(std::move(socket));
                session->accept(ec);

                std::lock_guard<std::mutex> lock(clients_mtx);
                clients.insert(session);
            }
            self->do_accept();
        });
    }
};

void MarketDataServer::start(unsigned short port) {
    server_thread = std::thread([port]() {
        auto listener = std::make_shared<WSListener>(ioc, port);
        listener->run();
        ioc.run();
    });

    std::cerr << "[MarketDataServer] running on ws://localhost:" << port << std::endl;
}

void MarketDataServer::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(clients_mtx);
    for (auto it = clients.begin(); it != clients.end();) {
        auto session = *it;
        if (session) {
            beast::error_code ec;
            session->text(true);
            session->write(asio::buffer(msg), ec);
            if (ec) it = clients.erase(it);
            else ++it;
        } else it = clients.erase(it);
    }
}

void MarketDataServer::stop() {
    ioc.stop();
    if (server_thread.joinable()) server_thread.join();
}
