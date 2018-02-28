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

#ifndef SKYWELL_APP_PATHS_SKYWELLLINECACHE_H_INCLUDED
#define SKYWELL_APP_PATHS_SKYWELLLINECACHE_H_INCLUDED

#include <transaction/paths/SkywellState.h>
#include <common/base/hardened_hash.h>
#include <cstddef>
#include <memory>
#include <vector>

namespace skywell {

// Used by Pathfinder
class SkywellLineCache
{
public:
    typedef std::vector <SkywellState::pointer> SkywellStateVector;
    typedef std::shared_ptr <SkywellLineCache> pointer;
    typedef pointer const& ref;

    explicit SkywellLineCache (Ledger::ref l);

    Ledger::ref getLedger () //  TODO const?
    {
        return mLedger;
    }

    std::vector<SkywellState::pointer> const&
    getSkywellLines (Account const& accountID);

private:
    typedef SkywellMutex LockType;
    typedef std::lock_guard <LockType> ScopedLockType;
    LockType mLock;

    skywell::hardened_hash<> hasher_;
    Ledger::pointer mLedger;

    struct AccountKey
    {
        Account account_;
        std::size_t hash_value_;

        AccountKey (Account const& account, std::size_t hash)
            : account_ (account)
            , hash_value_ (hash)
        { }

        AccountKey (AccountKey const& other) = default;

        AccountKey&
        operator=(AccountKey const& other) = default;

        bool operator== (AccountKey const& lhs) const
        {
            return hash_value_ == lhs.hash_value_ && account_ == lhs.account_;
        }

        std::size_t
        get_hash () const
        {
            return hash_value_;
        }

        struct Hash
        {
            std::size_t
            operator () (AccountKey const& key) const noexcept
            {
                return key.get_hash ();
            }
        };
    };

    hash_map <AccountKey, SkywellStateVector, AccountKey::Hash> mRLMap;
};

} // skywell

#endif
