#pragma once

#include <boost/asio.hpp>

#include <memory>

namespace server {

template<class T>
class Factory {
    using ptr = std::shared_ptr<T>;

    ptr operator()(ptr other)
    { return std::make_shared<T>(*other); }

};

template <class T, class U=Factory<T>>
class TcpServer {
public:
    using Client = T;
    using ClientFactory = U;
    using ClientPtr = std::shared_ptr<Client>;

    template<
        std::enable_if<
            std::is_base_of<
                std::enable_shared_from_this<Client>,
                Client
            >::type
        >
    >
    TcpServer(boost::asio::io_service& service, ClientPtr instance)
        : m_service(service)
    {


    }
private:
    boost::asio::io_service& m_service;

};

} // namespace server
