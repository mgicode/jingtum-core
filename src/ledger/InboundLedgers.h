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

#ifndef SKYWELL_APP_LEDGER_INBOUNDLEDGERS_H_INCLUDED
#define SKYWELL_APP_LEDGER_INBOUNDLEDGERS_H_INCLUDED

#include <ledger/InboundLedger.h>
#include <protocol/SkywellLedgerHash.h>
#include <beast/threads/Stoppable.h>
#include <common/misc/Utility.h>

namespace skywell {

/** Manages the lifetime of inbound ledgers.

    @see InboundLedger
*/
class InboundLedgers
{
public:
    typedef beast::abstract_clock <std::chrono::steady_clock> clock_type;

    virtual ~InboundLedgers() = 0;

    //  TODO Should this be called findOrAdd ?
    //
    virtual Ledger::pointer acquire (uint256 const& hash,
                                     std::uint32_t seq, 
                                     InboundLedger::fcReason) = 0;

    virtual InboundLedger::pointer find (LedgerHash const& hash) = 0;

    virtual bool hasLedger (LedgerHash const& ledgerHash) = 0;

    virtual void dropLedger (LedgerHash const& ledgerHash) = 0;

    //  TODO Why is hash passed by value?
    //  TODO Remove the dependency on the Peer object.
    //
    virtual bool gotLedgerData (LedgerHash const& ledgerHash,
                                std::shared_ptr<Peer>,
                                std::shared_ptr<protocol::TMLedgerData>) = 0;

    virtual void doLedgerData (Job&, LedgerHash hash) = 0;

    virtual void gotStaleData (std::shared_ptr<protocol::TMLedgerData> packet) = 0;

    virtual int getFetchCount (int& timeoutCount) = 0;

    virtual void logFailure (uint256 const& h) = 0;

    virtual bool isFailure (uint256 const& h) = 0;

    virtual void clearFailures() = 0;

    virtual Json::Value getInfo() = 0;

    /** Returns the rate of historical ledger fetches per minute. */
    virtual std::size_t fetchRate() = 0;

    /** Called when a complete ledger is obtained. */
    virtual void onLedgerFetched (InboundLedger::fcReason why) = 0;

    virtual void gotFetchPack (Job&) = 0;

    virtual void sweep () = 0;

    virtual void onStop() = 0;
};

std::unique_ptr<InboundLedgers>
make_InboundLedgers (InboundLedgers::clock_type& clock, 
                     beast::Stoppable& parent,
                     beast::insight::Collector::ptr const& collector);


} // skywell

#endif
