#ifndef SKYWELL_SSL_BUNDLE_H_
#define SKYWELL_SSL_BUNDLE_H_

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <memory>
#include <utility>

struct sslbundle
{
    using socket_type = boost::asio::ip::tcp::socket;
    using stream_type = boost::asio::ssl::stream<socket_type&>;
    using shared_context = std::shared_ptr<boost::asio::ssl::context>;

    template <typename... Args>
    sslbundle(shared_context const& context_, Args&&... args);

    template <typename... Args>
    sslbundle(boost::asio::ssl::context& context_, Args&&... args);

    shared_context context;
    socket_type    socket;
    stream_type    stream;
};

template <typename... Args>
sslbundle::sslbundle(const shared_context& context_, Args&&... args)
                : socket(std::forward<Args>(args)...)
                , stream(socket, *context_)
{}

template <typename... Args>
sslbundle::sslbundle(boost::asio::ssl::context& context_, Args&&... args)
                : socket(std::forward<Args>(args)...)
                , stream(socket, context_)
{}




#endif
