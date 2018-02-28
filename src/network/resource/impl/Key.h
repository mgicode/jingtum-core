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

#ifndef SKYWELL_RESOURCE_KEY_H_INCLUDED
#define SKYWELL_RESOURCE_KEY_H_INCLUDED

#include <cassert>
#include <network/resource/impl/Kind.h>
#include <boost/asio.hpp>
#include <beast/hash/hash_append.h>
#include <beast/hash/uhash.h>
#include <common/misc/Utility.h>

namespace skywell {
namespace Resource {

// The consumer key
struct Key
{
    Kind kind;

    struct TCPEndpoint
    {
        boost::asio::ip::tcp::endpoint ep;
        
        TCPEndpoint()
        {}

        TCPEndpoint(const boost::asio::ip::tcp::endpoint& ep)
        {
            this->ep = ep;
        }

        template <typename Hasher>
        friend
        void 
        hash_append(Hasher& h, TCPEndpoint const& endpoint)
        {
            using beast::hash_append;
            std::string hash_string = endpoint.ep.address().to_string() + std::to_string(endpoint.ep.port());
            hash_append(h, hash_string);
        }

        bool operator==(const TCPEndpoint& ep) const
        {
            return this->ep == ep.ep;
        }

        std::string
        to_string()
        {
            return convert_endpoint_to_string(this->ep);
        }
    };

    //boost::asio::ip::tcp::endpoint address;
    TCPEndpoint address;

    std::string name;

    Key () = delete;

    // Constructor for Inbound and Outbound (non-Admin) keys
    Key (Kind k, boost::asio::ip::tcp::endpoint const& addr)
        : kind(k)
        , address(addr)
        , name()
    {
        assert(kind != kindAdmin);
    }

    // Constructor for Admin keys
    Key (Kind k, std::string const& n)
        : kind(k)
        , address()
        , name(n)
    {
        assert(kind == kindAdmin);
    }

    struct hasher
    {
        std::size_t operator() (Key const& v) const
        {
            switch (v.kind)
            {
            case kindInbound:
            case kindOutbound:
                return m_addr_hash (v.address);

            case kindAdmin:
                return m_name_hash (v.name);

            default:
                assert(false);
            };

            return 0;
        }

    private:
        beast::uhash <> m_addr_hash;
        beast::uhash <> m_name_hash;
    };

    struct key_equal
    {
        bool operator() (Key const& lhs, Key const& rhs) const
        {
            if (lhs.kind != rhs.kind)
                return false;

            switch (lhs.kind)
            {
            case kindInbound:
            case kindOutbound:
                return lhs.address == rhs.address;

            case kindAdmin:
                return lhs.name == rhs.name;

            default:
                assert(false);
            };

            return false;
        }

    private:
    };
};

}
}

#endif
