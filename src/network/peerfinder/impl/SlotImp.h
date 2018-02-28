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

#ifndef SKYWELL_PEERFINDER_SLOTIMP_H_INCLUDED
#define SKYWELL_PEERFINDER_SLOTIMP_H_INCLUDED

#include <network/peerfinder/Slot.h>
#include <network/peerfinder/Manager.h>
#include <boost/optional.hpp>
#include <atomic>
#include <boost/unordered_map.hpp>
#include <boost/asio.hpp>

namespace skywell {
namespace PeerFinder {

class SlotImp : public Slot
{
public:
    struct IPEndpoint
    {
        boost::asio::ip::tcp::endpoint ep_;
        std::chrono::steady_clock::time_point tp_;
        int hp_;

        IPEndpoint()
        {}

        IPEndpoint(std::chrono::steady_clock::time_point tp)
        {
            tp_ = tp;
        }

        bool operator==(const IPEndpoint& ep) const
        {
            return ep_.address().to_v4().to_ulong() == ep.ep_.address().to_v4().to_ulong() &&
                   ep.ep_.port() == ep_.port();
        }
    };

private:
    typedef boost::unordered_map<IPEndpoint, int> recent_type;
public:
    struct RecentType
    {
        std::chrono::steady_clock::time_point tp_;
        RecentType(clock_type& tp)
        {
            this->tp_ = std::chrono::steady_clock::now();
        }

        static void expire(RecentType& end, std::chrono::seconds sec)
        {
          for(auto it = end.cache_.begin(); it != end.cache_.end();)
          {
                auto tp = it->first.tp_;
                if(sec <= std::chrono::steady_clock::now() - tp)
                {
                    it = end.cache_.erase(it);
                }
           }
        }

        recent_type cache_;

        void touch(const IPEndpoint& ep)
        {
            IPEndpoint end;
            end.ep_ = ep.ep_;
            end.tp_ = tp_;
            end.hp_ = ep.hp_;

            cache_.emplace(end, end.hp_);
        }

        recent_type::iterator
        begin()
        {
            return cache_.begin();
        }

        recent_type::iterator
        end()
        {
            return cache_.end();
        }
    };

public:
    typedef std::shared_ptr <SlotImp> ptr;

    // inbound
    SlotImp (boost::asio::ip::tcp::endpoint const& local_endpoint,
             boost::asio::ip::tcp::endpoint const& remote_endpoint, 
             bool fixed,
             clock_type& clock);

    // outbound
    SlotImp (boost::asio::ip::tcp::endpoint const& remote_endpoint,
             bool fixed, 
             clock_type& clock);

    bool inbound () const
    {
        return m_inbound;
    }

    bool fixed () const
    {
        return m_fixed;
    }

    bool cluster () const
    {
        return m_cluster;
    }

    State state () const
    {
        return m_state;
    }

    boost::asio::ip::tcp::endpoint const& remote_endpoint () const
    {
        return m_remote_endpoint;
    }

    boost::optional <boost::asio::ip::tcp::endpoint> const& local_endpoint () const
    {
        return m_local_endpoint;
    }

    boost::optional <SkywellPublicKey> const& public_key () const
    {
        return m_public_key;
    }

    boost::optional<std::uint16_t> listening_port () const
    {
        std::uint32_t const value = m_listening_port;
        if (value == unknownPort)
            return boost::none;
        return value;
    }

    void set_listening_port (std::uint16_t port)
    {
        m_listening_port = port;
    }

    void local_endpoint (boost::asio::ip::tcp::endpoint const& endpoint)
    {
        m_local_endpoint = endpoint;
    }

    void remote_endpoint (boost::asio::ip::tcp::endpoint const& endpoint)
    {
        m_remote_endpoint = endpoint;
    }

    void public_key (SkywellPublicKey const& key)
    {
        m_public_key = key;
    }

    void cluster (bool cluster_)
    {
        m_cluster = cluster_;
    }

    //--------------------------------------------------------------------------

    void state (State state_);

    void activate (clock_type::time_point const& now);

    // "Memberspace"
    //
    // The set of all recent addresses that we have seen from this peer.
    // We try to avoid sending a peer the same addresses they gave us.
    //
    class recent_t
    {
    public:
        explicit recent_t (clock_type& clock);

        /** Called for each valid endpoint received for a slot.
            We also insert messages that we send to the slot to prevent
            sending a slot the same address too frequently.
        */
        void insert (boost::asio::ip::tcp::endpoint const& ep, int hops);

        /** Returns `true` if we should not send endpoint to the slot. */
        bool filter (boost::asio::ip::tcp::endpoint const& ep, int hops);

    private:
        void expire ();

        friend class SlotImp;
//        recent_type cache;
        RecentType cache;
    } recent;

    void expire()
    {
        recent.expire();
    }

private:
    bool const m_inbound;
    bool const m_fixed;
    bool m_cluster;
    State m_state;
    boost::asio::ip::tcp::endpoint m_remote_endpoint;
    boost::optional<boost::asio::ip::tcp::endpoint> m_local_endpoint;
    boost::optional<SkywellPublicKey> m_public_key;

    static std::int32_t BEAST_CONSTEXPR unknownPort = -1;
    std::atomic <std::int32_t> m_listening_port;

public:
    // DEPRECATED public data members

    // Tells us if we checked the connection. Outbound connections
    // are always considered checked since we successfuly connected.
    bool checked;

    // Set to indicate if the connection can receive incoming at the
    // address advertised in mtENDPOINTS. Only valid if checked is true.
    bool canAccept;

    // Set to indicate that a connection check for this peer is in
    // progress. Valid always.
    bool connectivityCheckInProgress;

    // The time after which we will accept mtENDPOINTS from the peer
    // This is to prevent flooding or spamming. Receipt of mtENDPOINTS
    // sooner than the allotted time should impose a load charge.
    //
    clock_type::time_point whenAcceptEndpoints;
};

inline std::size_t hash_value(const SlotImp::IPEndpoint& ep)
{
    std::size_t seed{0};
    boost::hash_combine(seed, ep.ep_.address().to_v4().to_ulong());
    boost::hash_combine(seed, ep.ep_.port());
    return seed;
}



}
}

#endif
