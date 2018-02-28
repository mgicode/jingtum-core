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

#ifndef SKYWELL_APP_PATHS_SKYWELLSRUST_H_INCLUDED
#define SKYWELL_APP_PATHS_SKYWELLSRUST_H_INCLUDED

#include <transaction/book/Types.h>
#include <protocol/STAmount.h>
#include <cstdint>
#include <common/misc/Utility.h>

namespace skywell {

//
// A skywell line's state.
// - Isolate ledger entry format.
//

class SkywellTrust
{
public:
    typedef std::shared_ptr <SkywellTrust> pointer;

public:
//    SkywellTrust () = delete;

//    virtual SkywellTrust () { }

    static SkywellTrust::pointer makeItem (
        Account const& accountID, STLedgerEntry::ref ledgerEntry);

    LedgerEntryType getType ()
    {
        return ltTrust_STATE;
    }
	
    std::uint32_t getRelationType () const
    {
        return mRelationType;
    }

    Account const& getAccountID () const
    {
        return  mViewLowest ? mLowID : mHighID;
    }

    Account const& getAccountIDPeer () const
    {
        return !mViewLowest ? mLowID : mHighID;
    }

    STAmount const& getBalance () const
    {
        return mBalance;
    }

    STAmount const& getLimit () const
    {
        return  mViewLowest ? mLowLimit : mHighLimit;
    }

    STAmount const& getLimitPeer () const
    {
        return !mViewLowest ? mLowLimit : mHighLimit;
    }

    std::uint32_t getQualityIn () const
    {
        return ((std::uint32_t) (mViewLowest ? mLowQualityIn : mHighQualityIn));
    }

    std::uint32_t getQualityOut () const
    {
        return ((std::uint32_t) (mViewLowest ? mLowQualityOut : mHighQualityOut));
    }

    STLedgerEntry::pointer getSLE ()
    {
        return mLedgerEntry;
    }

    const STLedgerEntry& peekSLE () const
    {
        return *mLedgerEntry;
    }

    STLedgerEntry& peekSLE ()
    {
        return *mLedgerEntry;
    }

    Json::Value getJson (int);

    Blob getRaw () const;

private:
    SkywellTrust (
        STLedgerEntry::ref ledgerEntry,
        Account const& viewAccount);

private:
    STLedgerEntry::pointer  mLedgerEntry;

    bool                            mViewLowest;

    std::uint32_t                   mFlags;
    std::uint32_t                   mRelationType;

    STAmount const&                 mLowLimit;
    STAmount const&                 mHighLimit;

    Account const&                  mLowID;
    Account const&                  mHighID;

    std::uint64_t                   mLowQualityIn;
    std::uint64_t                   mLowQualityOut;
    std::uint64_t                   mHighQualityIn;
    std::uint64_t                   mHighQualityOut;

    STAmount                        mBalance;
};

std::vector <SkywellTrust::pointer>
getSkywellTrustItems (
    Account const& accountID,
    Ledger::ref ledger);

} // skywell

#endif
