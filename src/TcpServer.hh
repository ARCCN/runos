#pragma once

#include <boost/asio.hpp>

#include <memory>

namespace server {

template <class T>
class TcpServer {
public:
    using Client = T;
    using ClientPtr = std::shared_ptr<Client>;

    TcpServer(boost::asio::io_service& service, boost::asio::ip::tcp::acceptor& acc, Client&& instance);
private:

    class ClientImpl : public Client {
    public:
        void read();
        void on_read(const boost::system::error_code err, size_t len);
    private:
        unsigned char m_buffer[1024];
        boost::asio::io_service& m_service;

    };

    boost::asio::io_service& m_service;

};

namespace detail {

using namespace boost::asio;
using namespace std::placeholders;
using SocketPtr = std::shared_ptr<ip::tcp::socket>;

//template<
//    class T,
//    std::enable_if<
//        std::is_base_of<
//            std::enable_shared_from_this<T>,
//            T
//        >::type
//    >
//>
//class enable_shared : public T
//{ };

template<class T>
class enable_shared
    : public T
    , public std::enable_shared_from_this<enable_shared<T>>
{ };

template<class T>
class ClientImpl : public enable_shared<T> {
public:
    using Client = T;

    ClientImpl(const ClientImpl& other)
        : enable_shared<T>(other)
        , m_service(other.m_service)
        , m_acc(other.m_acc)
    { };

    ClientImpl(io_service& service, ip::tcp::acceptor& acc, Client&& client)
        : m_service(service)
        , m_acc(acc)
        , enable_shared<T>(client)
    { }

    void read() {
        m_socket.async_receive(
                buffer(m_buffer),
                std::bind(&ClientImpl::on_read, shared_from_this(), _1, _2)
            );
    }

    void on_read(const boost::system::error_code err, size_t len)
    {
        m_buffer[len] = '\0';
        if (len > 0) {
            Client::on_read(m_buffer);
        }
        if (err) {
            return;
        } else {
            read();
        }
    }

    void start_accepting(std::function<void(const boost::system::error_code&)> func)
    {
        m_acc.async_accept(m_socket, func);
    }
private:
    unsigned char m_buffer[1024];
    io_service& m_service;
    ip::tcp::socket m_socket;
    ip::tcp::acceptor& m_acc;
};

using ClientImplPtr = std::shared_ptr<ClientImpl>;
template <typename... Args>
ClientImplPtr makeClientImpl(Args&& ...args)
{ return std::make_shared<Client>(std::forward<Args>(args)...); }

void handle_accept(ClientImplPtr client, const boost::systerm::error_code& err)
{
    client->read();
    auto new_client = makeClientImpl(*client);
    new_client->start_accepting(std::bind(handle_accept, new_client, _1));
}

} // namespace detail

template <class Client>
TcpServer::TcpServer(boost::asio::io_service& service, boost::asio::ip::tcp::acceptor& acc, Client&& instance)
        : m_service(service)
    {
        auto client = detail::makeClient(service, acc std::forward<Client>(instance));
        client->start_accepting(std::bind(detail::handle_accpet, client,
                    std::placeholders::_1))
    }
} // namespace server
