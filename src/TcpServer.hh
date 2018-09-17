#pragma once

#include <boost/asio.hpp>

#include <memory>

namespace server {

class ClientInterface {
public:
    virtual bool on_read(char* buffer, size_t len) = 0;
    void write(char* buffer, size_t len);
private:

};

template <class T>
class ClientManager {
public:
    using Client = T;
    using ClientPtr = std::shared_ptr<Client>;

    ClientManager(boost::asio::io_service& service, boost::asio::ip::tcp::acceptor& acc, Client&& instance);
private:
    boost::asio::io_service& m_service;

};

namespace detail {

using namespace boost::asio;
using namespace std::placeholders;
using SocketPtr = std::shared_ptr<ip::tcp::socket>;

template<class T, size_t buffer_size>
class ClientImpl :
    public std::enable_shared_from_this<ClientImpl<T, buffer_size>>{
public:
    using Client = T;

    ClientImpl(ClientImpl& other)
        : m_client(other.m_client)
        , m_service(other.m_service)
        , m_socket(other.m_service)
        , m_acc(other.m_acc)
    { };

    ClientImpl(io_service& service, ip::tcp::acceptor& acc, Client&& client)
        : m_client(std::forward<Client>(client))
        , m_service(service)
        , m_socket(service)
        , m_acc(acc)
    { }

    void read() {
        m_socket.async_receive(
                buffer(m_buffer),
                std::bind(&ClientImpl::on_read, this->shared_from_this(), _1, _2)
            );
    }

    void on_read(const boost::system::error_code err, size_t len)
    {
        m_buffer[len] = '\0';
        VLOG(10) << "Read data : [" << m_buffer << "]";
        if (len > 0) {
            m_client.on_read(m_buffer);
        }
        if (err) {
            return;
        } else {
            read();
        }
    }

    void start_accepting(std::function<void(const boost::system::error_code&)> func)
    {
        LOG(INFO) << "Start accepting";
        m_acc.async_accept(m_socket, func);
    }
private:
    Client m_client;
    unsigned char m_buffer[buffer_size];
    io_service& m_service;
    ip::tcp::socket m_socket;
    ip::tcp::acceptor& m_acc;
};

template<class T, size_t buffer_size>
using ClientImplPtr = std::shared_ptr<ClientImpl<T, buffer_size>>;

template <class T, size_t buffer_size, typename... Args>
ClientImplPtr<T, buffer_size> makeClientImpl(Args&& ...args)
{ return std::make_shared<ClientImpl<T, buffer_size>>(std::forward<Args>(args)...); }

template<class T, size_t buffer_size>
void handle_accept(ClientImplPtr<T, buffer_size> client, const boost::system::error_code& err)
{
    LOG(INFO) << "Accepted";
    client->read();
    auto new_client = makeClientImpl<T, buffer_size>(*client);
    new_client->start_accepting(std::bind(handle_accept<T, buffer_size>, new_client, _1));
}

} // namespace detail

template <class Client, size_t buffer_size>
ClientManager<Client, buffer_size>::ClientManager(
            boost::asio::io_service& service,
            boost::asio::ip::tcp::acceptor& acc,
            Client&& instance
        )
        : m_service(service)
    {
        auto client = detail::makeClientImpl<Client, buffer_size>(service, acc, std::forward<Client>(instance));
        client->start_accepting(std::bind(detail::handle_accept<Client, buffer_size>, client,
                    std::placeholders::_1));
    }
} // namespace server
