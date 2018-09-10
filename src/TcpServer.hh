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
class ClientImpl : public T
                 , public std::enable_shared_from_this<ClientImpl<T>>{
public:
    using Client = T;

    ClientImpl(ClientImpl& other)
        : Client(other)
        , m_service(other.m_service)
        , m_socket(other.m_service)
        , m_acc(other.m_acc)
    { };

    ClientImpl(io_service& service, ip::tcp::acceptor& acc, Client&& client)
        : Client(client)
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
        LOG(INFO) << "Start accepting";
        m_acc.async_accept(m_socket, func);
    }
private:
    unsigned char m_buffer[1024];
    io_service& m_service;
    ip::tcp::socket m_socket;
    ip::tcp::acceptor& m_acc;
};

template<class T>
using ClientImplPtr = std::shared_ptr<ClientImpl<T>>;

template <class T, typename... Args>
ClientImplPtr<T> makeClientImpl(Args&& ...args)
{ return std::make_shared<ClientImpl<T>>(std::forward<Args>(args)...); }

template<class T>
void handle_accept(ClientImplPtr<T> client, const boost::system::error_code& err)
{
    LOG(INFO) << "Accepted";
    client->read();
    auto new_client = makeClientImpl<T>(*client);
    new_client->start_accepting(std::bind(handle_accept<T>, new_client, _1));
}

} // namespace detail

template <class Client>
TcpServer<Client>::TcpServer(boost::asio::io_service& service, boost::asio::ip::tcp::acceptor& acc, Client&& instance)
        : m_service(service)
    {
        auto client = detail::makeClientImpl<Client>(service, acc, std::forward<Client>(instance));
        client->start_accepting(std::bind(detail::handle_accept<Client>, client,
                    std::placeholders::_1));
    }
} // namespace server
