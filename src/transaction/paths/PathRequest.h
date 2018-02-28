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

#ifndef SKYWELL_APP_PATHS_PATHREQUEST_H_INCLUDED
#define SKYWELL_APP_PATHS_PATHREQUEST_H_INCLUDED

#include <transaction/paths/SkywellLineCache.h>
#include <common/json/json_value.h>
#include <services/net/InfoSub.h>

namespace skywell {

// A pathfinding request submitted by a client
// The request issuer must maintain a strong pointer

class SkywellLineCache;
class PathRequests;

// Return values from parseJson <0 = invalid, >0 = valid
#define PFR_PJ_INVALID              -1
#define PFR_PJ_NOCHANGE             0
#define PFR_PJ_CHANGE               1

class PathRequest :
    public std::enable_shared_from_this <PathRequest>,
    public CountedObject <PathRequest>
{
public:
    static char const* getCountedObjectName () { return "PathRequest"; }

    typedef std::weak_ptr<PathRequest>    wptr;
    typedef std::shared_ptr<PathRequest>  pointer;
    typedef const pointer&                  ref;
    typedef const wptr&                     wref;

public:
    //  TODO Break the cyclic dependency on InfoSub
    PathRequest (
        std::shared_ptr <InfoSub> const& subscriber,
        int id,
        PathRequests&,
        beast::Journal journal);

    ~PathRequest ();

    bool        isValid ();
    bool        isNew ();
    bool        needsUpdate (bool newOnly, LedgerIndex index);
    void        updateComplete ();
    Json::Value getStatus ();

    Json::Value doCreate (
        const std::shared_ptr<Ledger>&,
        const SkywellLineCache::pointer&,
        Json::Value const&,
        bool&);
    Json::Value doClose (Json::Value const&);
    Json::Value doStatus (Json::Value const&);

    // update jvStatus
    Json::Value doUpdate (const std::shared_ptr<SkywellLineCache>&, bool fast);
    InfoSub::pointer getSubscriber ();

private:
    bool isValid (SkywellLineCache::ref crCache);
    void setValid ();
    void resetLevel (int level);
    int parseJson (Json::Value const&, bool complete);

    beast::Journal m_journal;

    typedef SkywellRecursiveMutex LockType;
    typedef std::lock_guard <LockType> ScopedLockType;
    LockType mLock;

    PathRequests& mOwner;

    std::weak_ptr<InfoSub> wpSubscriber; // Who this request came from

    Json::Value jvId;
    Json::Value jvStatus;                   // Last result

    // Client request parameters
    SkywellAddress raSrcAccount;
    SkywellAddress raDstAccount;
    STAmount saDstAmount;

    std::set<Issue> sciSourceCurrencies;
    std::map<Issue, STPathSet> mContext;

    bool bValid;

    LockType mIndexLock;
    LedgerIndex mLastIndex;
    bool mInProgress;

    int iLastLevel;
    bool bLastSuccess;

    int iIdentifier;

    boost::posix_time::ptime ptCreated;
    boost::posix_time::ptime ptQuickReply;
    boost::posix_time::ptime ptFullReply;
};

} // skywell

#endif
