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

#ifndef SKYWELL_PEERFINDER_LIVECACHE_H_INCLUDED
#define SKYWELL_PEERFINDER_LIVECACHE_H_INCLUDED

#include <network/peerfinder/Manager.h>
#include <network/peerfinder/impl/iosformat.h>
#include <network/peerfinder/impl/Tuning.h>
#include <beast/chrono/chrono_io.h>
#include <boost/intrusive/list.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/unordered_map.hpp>
#include <chrono>
#include <string>

namespace skywell {
namespace PeerFinder {

template <class>
class Livecache;

namespace detail {

class LivecacheBase
{
protected:
    struct Element : boost::intrusive::list_base_hook<>
    {
        Element (Endpoint const& endpoint_)
            : endpoint (endpoint_)
        {
        }

        Endpoint endpoint;
    };

    typedef boost::intrusive::make_list <Element, boost::intrusive::constant_time_size <false>>::type list_type;

public:
    /** A list of Endpoint at the same hops
        This is a lightweight wrapper around a reference to the underlying
        container.
    */
    template <bool IsConst>
    class Hop
    {
    public:
        // Iterator transformation to extract the endpoint from Element
        struct Transform : public std::unary_function<Element, Endpoint>
        {
            Endpoint const& operator() (Element const& e) const
            {
                return e.endpoint;
            }
        };

    public:
        typedef boost::transform_iterator <Transform, typename list_type::const_iterator> iterator;

        typedef iterator const_iterator;

        typedef boost::transform_iterator <Transform, typename list_type::const_reverse_iterator> reverse_iterator;

        typedef reverse_iterator const_reverse_iterator;

        iterator begin () const
        {
            return iterator (m_list.get().cbegin(), Transform());
        }

        iterator cbegin () const
        {
            return iterator (m_list.get().cbegin(), Transform());
        }

        iterator end () const
        {
            return iterator (m_list.get().cend(), Transform());
        }

        iterator cend () const
        {
            return iterator (m_list.get().cend(), Transform());
        }

        reverse_iterator rbegin () const
        {
            return reverse_iterator (m_list.get().crbegin(), Transform());
        }

        reverse_iterator crbegin () const
        {
            return reverse_iterator (m_list.get().crbegin(), Transform());
        }

        reverse_iterator rend () const
        {
            return reverse_iterator (m_list.get().crend(), Transform());
        }

        reverse_iterator crend () const
        {
            return reverse_iterator (m_list.get().crend(), Transform());
        }

        // move the element to the end of the container
        void move_back (const_iterator pos)
        {
            auto& e (const_cast <Element&>(*pos.base()));

            m_list.get().erase (m_list.get().iterator_to (e));
            m_list.get().push_back (e);
        }

    private:

        explicit Hop(typename std::conditional<IsConst,
                                               typename std::remove_const<list_type>::type const,
                                               typename std::remove_const<list_type>::type>::type& list)
            :m_list(list)
        {}

        friend class LivecacheBase;

        std::reference_wrapper<typename std::conditional<IsConst, 
                                                        typename std::remove_const<list_type>::type const,
                                                        typename std::remove_const<list_type>::type>::type>
                              m_list;

    };

protected:
    // Work-around to call Hop's private constructor from Livecache
    template <bool IsConst>
    static Hop<IsConst> make_hop (typename std::conditional<IsConst, 
                                                            typename std::remove_const<list_type>::type const,
                                                            typename std::remove_const<list_type>::type>::type& list)
    {
        return Hop <IsConst> (list);
    }
};

} //details

//------------------------------------------------------------------------------

/** The Livecache holds the short-lived relayed Endpoint messages.

    Since peers only advertise themselves when they have open slots,
    we want these messags to expire rather quickly after the peer becomes
    full.

    Addresses added to the cache are not connection-tested to see if
    they are connectible (with one small exception regarding neighbors).
    Therefore, these addresses are not suitable for persisting across
    launches or for bootstrapping, because they do not have verifiable
    and locally observed uptime and connectibility information.
*/
template <class Allocator = std::allocator <char>>
class Livecache : protected detail::LivecacheBase
{
private:
    beast::Journal m_journal;
    
public:
    typedef Allocator allocator_type;

    /** Create the cache. */
    Livecache (clock_type&    clock,
               beast::Journal journal,
               Allocator      alloc = Allocator());

    struct KeyWrapper 
    {
        Endpoint ep;
        std::chrono::steady_clock::time_point tp;

        bool operator==(const KeyWrapper& key) const
        {
            return ep == key.ep && tp = key.tp;
        }

        bool operator<(const KeyWrapper& key) const
        {
            return ep.address.address().to_string() < key.ep.address.address().to_string();
        }
    };

    class CacheType
    {
    public:
        using umap = boost::unordered_map<KeyWrapper, Element, boost::hash<KeyWrapper>, std::less<KeyWrapper>, Allocator>;
        umap map_;
        
        void touch(const KeyWrapper& key, Element& elem)
        {
            KeyWrapper w;

            w.ep = key.ep;
            w.tp = std::chrono::steady_clock::now();

            map_.emplace(w, elem);
        }

    } m_cache;

    //
    // Iteration by hops
    //
    // The range [begin, end) provides a sequence of list_type
    // where each list contains endpoints at a given hops.
    //

    class hops_t
    {
    private:
        // An endpoint at hops=0 represents the local node.
        // Endpoints coming in at maxHops are stored at maxHops +1,
        // but not given out (since they would exceed maxHops). They
        // are used for automatic connection attempts.
        //
        typedef std::array<int, 1 + Tuning::maxHops + 1> Histogram;
        typedef std::array<list_type, 1 + Tuning::maxHops + 1> lists_type;

        template <bool IsConst>
        struct Transform
            : public std::unary_function <typename lists_type::value_type, Hop <IsConst>>
        {
            Hop<IsConst> operator() (typename std::conditional<IsConst, 
                                                               typename std::remove_const<list_type>::type const,
                                                               typename std::remove_const<list_type>::type>::type& list) const
            {
                return make_hop<IsConst> (list);
            }
        };

    public:
        typedef boost::transform_iterator<Transform <false>, typename lists_type::iterator> iterator;

        typedef boost::transform_iterator<Transform <true>,  typename lists_type::const_iterator> const_iterator;

        typedef boost::transform_iterator<Transform <false>, typename lists_type::reverse_iterator> reverse_iterator;

        typedef boost::transform_iterator<Transform <true>,  typename lists_type::const_reverse_iterator> const_reverse_iterator;

        iterator begin ()
        {
            return iterator(m_lists.begin(), Transform <false>());
        }

        const_iterator begin () const
        {
            return const_iterator(m_lists.cbegin(), Transform <true>());
        }

        const_iterator cbegin () const
        {
            return const_iterator (m_lists.cbegin(), Transform <true>());
        }

        iterator end ()
        {
            return iterator (m_lists.end(), Transform <false>());
        }

        const_iterator end () const
        {
            return const_iterator (m_lists.cend(), Transform <true>());
        }

        const_iterator cend () const
        {
            return const_iterator (m_lists.cend(), Transform <true>());
        }

        reverse_iterator rbegin ()
        {
            return reverse_iterator (m_lists.rbegin(), Transform <false>());
        }

        const_reverse_iterator rbegin () const
        {
            return const_reverse_iterator (m_lists.crbegin(), Transform <true>());
        }

        const_reverse_iterator crbegin () const
        {
            return const_reverse_iterator (m_lists.crbegin(), Transform <true>());
        }

        reverse_iterator rend ()
        {
            return reverse_iterator (m_lists.rend(), Transform <false>());
        }

        const_reverse_iterator rend () const
        {
            return const_reverse_iterator (m_lists.crend(), Transform <true>());
        }

        const_reverse_iterator crend () const
        {
            return const_reverse_iterator (m_lists.crend(), Transform <true>());
        }

        /** Shuffle each hop list. */
        void shuffle ();

        std::string histogram() const;

    private:
        explicit hops_t (Allocator const& alloc);

        void insert (Element& e);

        // Reinsert e at a new hops
        void reinsert (Element& e, int hops);

        void remove (Element& e);

        friend class Livecache;

        lists_type m_lists;
        Histogram m_hist;

    } hops;

    /** Returns `true` if the cache is empty. */
    bool empty () const
    {
        return m_cache.map_.empty ();
    }

    /** Returns the number of entries in the cache. */
    std::size_t size() const
    {
        return m_cache.map_.size();
    }

    /** Erase entries whose time has expired. */
    void expire ();

    /** Creates or updates an existing Element based on a new message. */
    void insert (Endpoint const& ep);

    /** Produce diagnostic output. */
    void dump (beast::Journal::ScopedStream& ss) const;

    /** Output statistics. */
    void onWrite (beast::PropertyStream::Map& map);

/*
    std::size_t hash_value(const KeyWrapper& k)
    {
        std::size_t seed{0};

        boost::hash_combine(seed, k.ep.address.to_string().stoi());
        boost::hash_combine(seed, k.tp);

        return seed;
    }
*/
};

template <typename Allocator = std::allocator <char>>
inline std::size_t hash_value(const typename Livecache<Allocator>::KeyWrapper& k)
{
    std::size_t seed{0};

    boost::hash_combine(seed, std::stoi(k.ep.address.address().to_string()));
    boost::hash_combine(seed, (k.tp).time_since_epoch().count());

    return seed;
}
//------------------------------------------------------------------------------
template <class Allocator>
Livecache <Allocator>::Livecache (clock_type& clock,
                                  beast::Journal journal,
                                  Allocator alloc)
    : m_journal (journal)
    , m_cache ()
    , hops (alloc)
{
}

template <class Allocator>
void
Livecache<Allocator>::expire()
{
    std::size_t n (0);
    typename std::chrono::steady_clock::time_point const expired (std::chrono::steady_clock::now() - Tuning::liveCacheSecondsToLive);

    for (auto iter (m_cache.map_.begin());
        iter != m_cache.map_.end() && (iter->first.tp <= expired);)
    {
        Element e (iter->first.ep);

        hops.remove (e);
        
        iter = m_cache.map_.erase (iter);

        ++n;
    }

    if (n > 0)
    {
        if (m_journal.debug)
            m_journal.debug << skywell::leftw (18) 
                            << "Livecache expired "
                            << n 
                            << ((n > 1) ? " entries" : " entry");
    }
}

template <class Allocator>
void Livecache<Allocator>::insert (Endpoint const& ep)
{
    // The caller already incremented hop, so if we got a
    // message at maxHops we will store it at maxHops + 1.
    // This means we won't give out the address to other peers
    // but we will use it to make connections and hand it out
    // when redirecting.
    //
    assert (ep.hops <= (Tuning::maxHops + 1));

    Livecache<Allocator>::KeyWrapper key;
    key.ep = ep;

    Element el(ep);
    const auto& result (m_cache.map_.emplace (key, el));

    Element e (result.first->second);
    if (result.second)
    {
        hops.insert (e);

        if (m_journal.debug) 
            m_journal.debug << skywell::leftw (18) 
                            << "Livecache insert "
                            << ep.address 
                            << " at hops " 
                            << ep.hops;
        return;
    }
    else if (! result.second && (ep.hops > e.endpoint.hops))
    {
        // Drop duplicates at higher hops
        std::size_t const excess (ep.hops - e.endpoint.hops);

        if (m_journal.trace)
            m_journal.trace << skywell::leftw(18) 
                            << "Livecache drop "
                            << ep.address 
                            << " at hops +" 
                            << excess;
        return;
    }

    
    m_cache.touch (const_cast<Livecache<Allocator>::KeyWrapper&>(result.first->first), e);

    // Address already in the cache so update metadata
    if (ep.hops < e.endpoint.hops)
    {
        hops.reinsert (e, ep.hops);

        if (m_journal.debug)
            m_journal.debug << skywell::leftw (18) 
                            << "Livecache update "
                            << ep.address 
                            << " at hops "
                            << ep.hops;
    }
    else
    {
        if (m_journal.trace) 
            m_journal.trace << skywell::leftw (18) 
                            << "Livecache refresh "
                            << ep.address 
                            << " at hops " 
                            << ep.hops;
    }
}

template <class Allocator>
void
Livecache <Allocator>::dump (beast::Journal::ScopedStream& ss) const
{
    ss << std::endl << std::endl << "Livecache (size " << m_cache.map_.size() << ")";

    for (auto const& entry : m_cache)
    {
        auto const& e (entry.first.second);

        ss << std::endl 
           << e.endpoint.address 
           << ", "
           << e.endpoint.hops
           << " hops";
    }
}

template <class Allocator>
void
Livecache <Allocator>::onWrite (beast::PropertyStream::Map& map)
{
    std::chrono::steady_clock::time_point const expired(std::chrono::steady_clock::now() - Tuning::liveCacheSecondsToLive);

    map ["size"] = size ();
    map ["hist"] = hops.histogram();
    beast::PropertyStream::Set set ("entries", map);

    for (auto iter (m_cache.map_.cbegin()); iter != m_cache.map_.cend(); ++iter)
    {
        auto const& e (iter->second);

        beast::PropertyStream::Map item (set);

        item ["hops"] = e.endpoint.hops;
        item ["address"] = e.endpoint.address.address().to_string ();

        std::stringstream ss;
        ss << iter->first.tp - expired;

        item ["expires"] = ss.str();
    }
}

//------------------------------------------------------------------------------

template <class Allocator>
void
Livecache <Allocator>::hops_t::shuffle()
{
    for (auto& list : m_lists)
    {
        std::vector<std::reference_wrapper <Element>> v;
        v.reserve (list.size());

        std::copy (list.begin(), list.end(), std::back_inserter (v));

        std::random_shuffle (v.begin(), v.end());
        list.clear();

        for (auto& e : v)
            list.push_back (e);
    }
}

template <class Allocator>
std::string
Livecache <Allocator>::hops_t::histogram() const
{
    std::stringstream ss;
    for (typename decltype(m_hist)::size_type i (0);
        i < m_hist.size(); 
        ++i)
    {
        ss << m_hist[i] << ((i < Tuning::maxHops + 1) ? ", " : "");
    }

    return ss.str();
}

template <class Allocator>
Livecache <Allocator>::hops_t::hops_t (Allocator const& alloc)
{
    std::fill (m_hist.begin(), m_hist.end(), 0);
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::insert (Element& e)
{
    assert (e.endpoint.hops >= 0 && e.endpoint.hops <= Tuning::maxHops + 1);

    // This has security implications without a shuffle
    m_lists [e.endpoint.hops].push_front (e);
    ++m_hist [e.endpoint.hops];
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::reinsert (Element& e, int hops)
{
    assert (hops >= 0 && hops <= Tuning::maxHops + 1);

    list_type& list (m_lists [e.endpoint.hops]);
    list.erase (list.iterator_to (e));
    --m_hist [e.endpoint.hops];

    e.endpoint.hops = hops;
    insert (e);
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::remove (Element& e)
{
    --m_hist [e.endpoint.hops];

    list_type& list (m_lists [e.endpoint.hops]);
    list.erase (list.iterator_to (e));
}

}
}

#endif
