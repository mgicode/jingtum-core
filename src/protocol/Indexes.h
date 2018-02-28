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

#ifndef SKYWELL_PROTOCOL_INDEXES_H_INCLUDED
#define SKYWELL_PROTOCOL_INDEXES_H_INCLUDED

#include <common/base/base_uint.h>
#include <protocol/LedgerFormats.h>
#include <protocol/SkywellAddress.h>
#include <protocol/Serializer.h>
#include <protocol/UintTypes.h>
#include <protocol/Book.h>

namespace skywell {

// get the index of the node that holds the last 256 ledgers
uint256
getLedgerHashIndex ();

// Get the index of the node that holds the set of 256 ledgers that includes
// this ledger's hash (or the first ledger after it if it's not a multiple
// of 256).
uint256
getLedgerHashIndex (std::uint32_t desiredLedgerIndex);

// get the index of the node that holds the enabled amendments
uint256
getLedgerAmendmentIndex ();

// get the index of the node that holds the fee schedule
uint256
getLedgerFeeIndex ();

uint256
getAccountRootIndex (Account const& account);

uint256
getAccountRootIndex (const SkywellAddress & account);

uint256
getGeneratorIndex (Account const& uGeneratorID);

uint256
getBookBase (Book const& book);

uint256
getOfferIndex (Account const& account, std::uint32_t uSequence);

uint256
getOwnerDirIndex (Account const& account);

uint256
getDirNodeIndex (uint256 const& uDirRoot, const std::uint64_t uNodeIndex);

uint256
getQualityIndex (uint256 const& uBase, const std::uint64_t uNodeDir = 0);

uint256
getQualityNext (uint256 const& uBase);

//  This name could be better
std::uint64_t
getQuality (uint256 const& uBase);

uint256
getTicketIndex (Account const& account, std::uint32_t uSequence);

uint256
getSkywellStateIndex (Account const& a, Account const& b, Currency const& currency);

uint256
getSkywellStateIndex (Account const& a, Issue const& issue);

uint256 
getTrustStateIndex (Account const& a, Account const& b, std::uint32_t uType,Account const& issuer,Currency const& currency);

uint256
getSignStateIndex(Account const& a);
uint256
getNickNameIndex(Account  const &a);
uint256
getNickNameIndex(const SkywellAddress & account);
}

#endif
