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

#ifndef SKYWELL_APP_PEERS_PEERSET_H_INCLUDED
#define SKYWELL_APP_PEERS_PEERSET_H_INCLUDED

#include <common/base/Log.h>
#include <common/core/Job.h>
#include <network/overlay/Peer.h>
#include <beast/chrono/abstract_clock.h>
#include <beast/utility/Journal.h>
#include <boost/asio/deadline_timer.hpp>

namespace skywell {

/** A set of peers used to acquire data.

    A peer set is used to acquire a ledger or a transaction set.
*/
class PeerSet
{
public:
    typedef beast::abstract_clock <std::chrono::steady_clock> clock_type;

    uint256 const& getHash () const
    {
        return mHash;
    }

    bool isComplete () const
    {
        return mComplete;
    }

    bool isFailed () const
    {
        return mFailed;
    }

    int getTimeouts () const
    {
        return mTimeouts;
    }

    bool isActive ();

    void progress ()
    {
        mProgress = true;
        mAggressive = false;
    }

    void clearProgress ()
    {
        mProgress = false;
    }

    bool isProgress ()
    {
        return mProgress;
    }

    void touch ()
    {
        mLastAction = m_clock.now();
    }

    clock_type::time_point getLastAction () const
    {
        return mLastAction;
    }

    //  TODO Rename this to addPeerToSet
    //
    bool peerHas (Peer::ptr const&);

    //  Workaround for MSVC std::function which doesn't swallow return types.
    void peerHasVoid (Peer::ptr const& peer)
    {
        peerHas (peer);
    }

    void badPeer (Peer::ptr const&);

    void setTimer ();

    std::size_t takePeerSetFrom (const PeerSet& s);
    std::size_t getPeerCount () const;
    virtual bool isDone () const
    {
        return mComplete || mFailed;
    }

private:
    static void TimerEntry (std::weak_ptr<PeerSet>, const boost::system::error_code& result);
    static void TimerJobEntry (Job&, std::shared_ptr<PeerSet>);

protected:
    //  TODO try to make some of these private
    typedef SkywellRecursiveMutex LockType;
    typedef std::unique_lock <LockType> ScopedLockType;

    PeerSet (uint256 const& hash, int interval, bool txnData,
        clock_type& clock, beast::Journal journal);
    virtual ~PeerSet () = 0;

    virtual void newPeer (Peer::ptr const&) = 0;
    virtual void onTimer (bool progress, ScopedLockType&) = 0;
    virtual std::weak_ptr<PeerSet> pmDowncast () = 0;

    void setComplete ()
    {
        mComplete = true;
    }
    void setFailed ()
    {
        mFailed = true;
    }
    void invokeOnTimer ();

    void sendRequest (const protocol::TMGetLedger& message);
    void sendRequest (const protocol::TMGetLedger& message, Peer::ptr const& peer);

protected:
    beast::Journal m_journal;
    clock_type& m_clock;

    LockType mLock;

    uint256 mHash;
    int mTimerInterval;
    int mTimeouts;
    bool mComplete;
    bool mFailed;
    bool mAggressive;
    bool mTxnData;
    clock_type::time_point mLastAction;
    bool mProgress;


    //  TODO move the responsibility for the timer to a higher level
    boost::asio::deadline_timer             mTimer;

    //  TODO Verify that these are used in the way that the names suggest.
    typedef Peer::id_t PeerIdentifier;
    typedef int ReceivedChunkCount;
    typedef hash_map <PeerIdentifier, ReceivedChunkCount> PeerSetMap;

    PeerSetMap mPeers;
};

} // skywell

#endif
