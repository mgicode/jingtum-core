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

#ifndef SKYWELL_APP_CONSENSUS_DISPUTEDTX_H_INCLUDED
#define SKYWELL_APP_CONSENSUS_DISPUTEDTX_H_INCLUDED

#include <protocol/UintTypes.h>
#include <protocol/Serializer.h>
#include <common/base/base_uint.h>
#include <memory>

namespace skywell {

/** A transaction discovered to be in dispute during conensus.

    During consensus, a @ref DisputedTx is created when a transaction
    is discovered to be disputed. The object persists only as long as
    the dispute.

    Undisputed transactions have no corresponding @ref DisputedTx object.
*/
class DisputedTx
{
public:
    typedef std::shared_ptr <DisputedTx> pointer;

    DisputedTx (uint256 const& txID,
                Blob const& tx,
                bool ourVote) :
        mTransactionID (txID), mYays (0), mNays (0), mOurVote (ourVote), transaction (tx)
    {
        ;
    }

    uint256 const& getTransactionID () const
    {
        return mTransactionID;
    }

    bool getOurVote () const
    {
        return mOurVote;
    }

    //  TODO make this const
    Serializer& peekTransaction ()
    {
        return transaction;
    }

    void setOurVote (bool o)
    {
        mOurVote = o;
    }

    //  NOTE its not really a peer, its the 160 bit hash of the validator's public key
    //
    void setVote (NodeID const& peer, bool votesYes);
    void unVote (NodeID const& peer);

    bool updateVote (int percentTime, bool proposing);
    Json::Value getJson ();

private:
    uint256 mTransactionID;
    int mYays;
    int mNays;
    bool mOurVote;
    Serializer transaction;

    hash_map <NodeID, bool> mVotes;
};

// How many total extra passes we make
// We must ensure we make at least one non-retriable pass
#define LEDGER_TOTAL_PASSES 3

// How many extra retry passes we
// make if the previous retry pass made changes
#define LEDGER_RETRY_PASSES 1

} // skywell

#endif
