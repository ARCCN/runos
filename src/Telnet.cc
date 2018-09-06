#include "Telnet.hh"

#include <iostream>
#include <thread>
#include <boost/asio.hpp>
#include <functional>

#include "Common.hh"

namespace telnet {

using namespace boost::asio;
using namespace std::placeholders;
using socket_ptr = std::shared_ptr<ip::tcp::socket>;

struct Client : public std::enable_shared_from_this<Client> {
    char m_buffer[1024];
    void read();
    void on_read(const boost::system::error_code err, size_t len);
//    void write(char* buf, size_t size);
//    void on_write(const boost::system::error_code err, size_t len);
    ip::tcp::socket& socket() { return m_socket; }
    void start();
    Client(io_service& service, ip::tcp::acceptor& acc)
        : m_service(service)
        , m_socket(service)
        , m_acc(acc) {
        LOG(INFO) << "Client created";
    }
    ~Client() {
        LOG(INFO) << "Client disconnected";
    }
    io_service& m_service;
    ip::tcp::socket m_socket;
    ip::tcp::acceptor& m_acc;

};

void Client::start() {
    read();
}

void Client::read() {
    m_socket.async_receive(buffer(m_buffer), std::bind(&Client::on_read, shared_from_this(), _1, _2));
}

void Client::on_read(const boost::system::error_code err, size_t size) {
    m_buffer[size] = '\0';
    LOG(INFO) << m_buffer;
    if (err) {
        return;
    } else {
        read();
    }
}

using ClientPtr = std::shared_ptr<Client>;

template <typename... Args>
ClientPtr makeClient(Args&& ...args)
{ return std::make_shared<Client>(std::forward<Args>(args)...); }

// struct ClientManager {
//     ClientPtr newClient() {
//         LOG(INFO) << "Start waiting client";
//         auto client = makeClient();
//         return client;
//     }
// };

// void client_session(socket_ptr sock, const boost::system::error_code& err) {
//     if (err) return;
//     while (true) {
//         char data[512];
//         boost::system::error_code error;
//         size_t len = sock->read_some(buffer(data), error);
//         if (error == error::eof) {
//             return;
//         }
//         if (len > 0) {
//             LOG(INFO) << data;
//         }
//     }
//
// }


void handle_accept(ClientPtr client, const boost::system::error_code& err);
void handle_accept(ClientPtr client, const boost::system::error_code& err){
    client->start();
    auto new_client = makeClient(client->m_service, client->m_acc);
    client->m_acc.async_accept(new_client->socket(), std::bind(handle_accept, new_client, _1));
}
struct Telnet::implementation {

    io_service service;
    ip::tcp::endpoint ep;
    ip::tcp::acceptor acc;
    implementation()
        : ep(ip::tcp::v4(), 2001)
        , acc(service, ep)
    { }
    void run() {
        auto client = makeClient(service, acc);
        acc.async_accept(client->socket(), std::bind(handle_accept, client, _1));
        service.run();
    }

};

Telnet::Telnet()
    : m_impl(std::make_unique<implementation>())
{
    std::thread {&implementation::run, m_impl.get()}.detach();
}

Telnet::~Telnet() = default;

} // namespace telnet
