//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2012, 2013 Skywell Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef SKYWELL_OVERLAY_CONNECTATTEMPT_H_INCLUDED
#define SKYWELL_OVERLAY_CONNECTATTEMPT_H_INCLUDED

#include <network/skywell.pb.h>
#include <network/overlay/impl/OverlayImpl.h>
#include <network/overlay/impl/ProtocolMessage.h>
#include <network/overlay/impl/TMHello.h>
#include <network/overlay/impl/Tuning.h>
#include <network/overlay/Message.h>
#include <network/peers/UniqueNodeList.h>
#include <protocol/BuildInfo.h>
#include <protocol/UintTypes.h>
#include <beast/asio/streambuf.h>
#include <beast/http/message.h>
#include <beast/http/parser.h>
#include <beast/utility/WrappedSink.h>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <chrono>
#include <functional>
#include <common/misc/Utility.h>
#include <common/misc/sslbundle.h>

namespace skywell {

/** Manages an outbound connection attempt. */
class ConnectAttempt
    : public OverlayImpl::Child
    , public std::enable_shared_from_this<ConnectAttempt>
{
private:
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;

    std::uint32_t const id_;
    beast::WrappedSink sink_;
    beast::Journal journal_;
    endpoint_type remote_endpoint_;
    Resource::Consumer usage_;
    boost::asio::io_service::strand strand_;
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
    std::unique_ptr<sslbundle> ssl_bundle_;
    sslbundle::socket_type& socket_;
    sslbundle::stream_type& stream_;
    beast::asio::streambuf read_buf_;
    beast::asio::streambuf write_buf_;
    beast::http::message response_;
    beast::asio::streambuf body_;
    beast::http::parser parser_;
    PeerFinder::Slot::ptr slot_;

public:
    ConnectAttempt (boost::asio::io_service& io_service
                , endpoint_type const& remote_endpoint
                , Resource::Consumer usage
                , sslbundle::shared_context const& context
                , std::uint32_t id, PeerFinder::Slot::ptr const& slot
                , beast::Journal journal, OverlayImpl& overlay);

    ~ConnectAttempt();

    void
    stop() override;

    void
    run();

private:
    void close();
    void fail (std::string const& reason);
    void fail (std::string const& name, error_code ec);
    void setTimer();
    void cancelTimer();
    void onTimer (error_code ec);
    void onConnect (error_code ec);
    void onHandshake (error_code ec);
    void onWrite (error_code ec, std::size_t bytes_transferred);
    void onRead (error_code ec, std::size_t bytes_transferred);
    void onShutdown (error_code ec);

    static
    beast::http::message
    makeRequest (bool crawl, boost::asio::ip::address const& remote_address);

    template <class Streambuf>
    void processResponse (beast::http::message const& m
                        , Streambuf const& body);

    template <class = void>
    static
    boost::asio::ip::tcp::endpoint
    parse_endpoint (std::string const& s, boost::system::error_code& ec)
    {
        //s = 1.1.1.1:33
        boost::asio::ip::tcp::endpoint endpoint = endpoint_from_string(s).first;
        if (endpoint == boost::asio::ip::tcp::endpoint{})
        {
            ec = boost::system::errc::make_error_code (boost::system::errc::invalid_argument);

            return boost::asio::ip::tcp::endpoint{};
        }

        return endpoint;
    }
};

}

#endif
