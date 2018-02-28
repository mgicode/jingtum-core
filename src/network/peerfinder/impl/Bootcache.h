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

#ifndef SKYWELL_PEERFINDER_BOOTCACHE_H_INCLUDED
#define SKYWELL_PEERFINDER_BOOTCACHE_H_INCLUDED

#include <network/peerfinder/Manager.h>
#include <network/peerfinder/impl/Store.h>
#include <beast/utility/Journal.h>
#include <beast/utility/PropertyStream.h>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/asio.hpp>
#include <common/misc/Utility.h>

namespace skywell {
namespace PeerFinder {

/** Stores IP addresses useful for gaining initial connections.

    This is one of the caches that is consulted when additional outgoing
    connections are needed. Along with the address, each entry has this
    additional metadata:

    Valence
        A signed integer which represents the number of successful
        consecutive connection attempts when positive, and the number of
        failed consecutive connection attempts when negative.

    When choosing addresses from the boot cache for the purpose of
    establishing outgoing connections, addresses are ranked in decreasing
    order of high uptime, with valence as the tie breaker.
*/
class Bootcache
{
public:
    class Entry
    {
    public:
        Entry (int valence) : m_valence (valence)
        {
        }

        int& valence ()
        {
            return m_valence;
        }

        int valence () const
        {
            return m_valence;
        }

        bool operator==(Entry const& e) const
        {
            return m_valence == e.m_valence;
        }

        friend bool operator< (Entry const& lhs, Entry const& rhs)
        {
            if (lhs.valence() > rhs.valence())
                return true;

            return false;
        }

    public:
        int m_valence;
    };
    
    struct TCPEndpoint
    {
        boost::asio::ip::tcp::endpoint ep;

        TCPEndpoint(boost::asio::ip::tcp::endpoint const& ep)
        {
            this->ep = ep;
        }

        bool operator == (const TCPEndpoint& endpoint) const
        {
            return endpoint.ep == this->ep;
        }
    };

    typedef boost::bimaps::unordered_set_of<TCPEndpoint> left_t;
    typedef boost::bimaps::multiset_of <Entry> right_t;
    typedef boost::bimap <left_t, right_t> map_type;
    typedef map_type::value_type value_type;

    struct Transform : std::unary_function <
            map_type::right_map::const_iterator::value_type const&,
            boost::asio::ip::tcp::endpoint const&>
    {
        boost::asio::ip::tcp::endpoint const& operator() (map_type::right_map::const_iterator::value_type const& v) const
        {
            return v.get_left ().ep;
        }
    };

private:
    map_type m_map;

    Store& m_store;
    clock_type& m_clock;
    beast::Journal m_journal;

    // Time after which we can update the database again
    clock_type::time_point m_whenUpdate;

    // Set to true when a database update is needed
    bool m_needsUpdate;

public:
    typedef boost::transform_iterator <Transform,
                                      map_type::right_map::const_iterator> iterator;

    typedef iterator const_iterator;

    Bootcache (Store& store
             , clock_type& clock
             , beast::Journal journal);

    ~Bootcache ();

    /** Returns `true` if the cache is empty. */
    bool empty() const;

    /** Returns the number of entries in the cache. */
    map_type::size_type size() const;

    /** boost::asio::ip::tcp::endpoint iterators that traverse in decreasing valence. */
    /** @{ */
    const_iterator begin() const;
    const_iterator cbegin() const;
    const_iterator end() const;
    const_iterator cend() const;
    void clear();
    /** @} */

    /** Load the persisted data from the Store into the container. */
    void load ();

    /** Add the address to the cache. */
    bool insert (boost::asio::ip::tcp::endpoint const& endpoint);

    /** Called when an outbound connection handshake completes. */
    void on_success (boost::asio::ip::tcp::endpoint const& endpoint);

    /** Called when an outbound connection attempt fails to handshake. */
    void on_failure (boost::asio::ip::tcp::endpoint const& endpoint);

    /** Stores the cache in the persistent database on a timer. */
    void periodicActivity ();

    /** Write the cache state to the property stream. */
    void onWrite (beast::PropertyStream::Map& map);

private:
    void prune ();
    void update ();
    void checkUpdate ();
    void flagForUpdate ();
};

inline std::size_t hash_value(const Bootcache::Entry& entry)
{
    return boost::hash_value(entry.m_valence);
}

inline std::size_t hash_value(const Bootcache::TCPEndpoint& endpoint)
{
    std::size_t seed{0};
    boost::hash_combine(seed, endpoint.ep.address().to_v4().to_ulong());
    boost::hash_combine(seed, endpoint.ep.port());
    return seed;
}
}
}

#endif
