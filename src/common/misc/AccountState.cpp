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

#include <BeastConfig.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <common/misc/AccountState.h>
#include <common/base/Log.h>
#include <common/base/StringUtilities.h>
#include <common/json/to_string.h>
#include <protocol/Indexes.h>
#include <protocol/JsonFields.h>

namespace skywell {

AccountState::AccountState (SkywellAddress const& naAccountID)
    : mAccountID (naAccountID)
    , mValid (false)
{
    if (naAccountID.isValid ())
    {
        mValid = true;

        mLedgerEntry = std::make_shared <STLedgerEntry> (
                           ltACCOUNT_ROOT, getAccountRootIndex (naAccountID));

        mLedgerEntry->setFieldAccount (sfAccount, naAccountID.getAccountID ());
    }
}

AccountState::AccountState (SLE::ref ledgerEntry, SkywellAddress const& naAccountID) :
    mAccountID (naAccountID), mLedgerEntry (ledgerEntry), mValid (false)
{
    if (!mLedgerEntry)
        return;

	if (mLedgerEntry->getType() != ltACCOUNT_ROOT && mLedgerEntry->getType()!=ltNICKNAME)
        return;

    mValid = true;
}

//  TODO Make this a generic utility function of some container class
//
std::string AccountState::createGravatarUrl (uint128 uEmailHash)
{
    Blob    vucMD5 (uEmailHash.begin (), uEmailHash.end ());
    std::string                 strMD5Lower = strHex (vucMD5);
    boost::to_lower (strMD5Lower);

    //  TODO Give a name and move this constant to a more visible location.
    //             Also shouldn't this be https?
    return str (boost::format ("http://www.gravatar.com/avatar/%s") % strMD5Lower);
}

void AccountState::addJson (Json::Value& val)
{
    val = mLedgerEntry->getJson (0);

    if (mValid)
    {
        if (mLedgerEntry->isFieldPresent (sfEmailHash))
            val[jss::urlgravatar]  = createGravatarUrl (mLedgerEntry->getFieldH128 (sfEmailHash));
		
    }
    else
    {
        val[jss::Invalid] = true;
    }
}

void AccountState::dump ()
{
    Json::Value j (Json::objectValue);
    addJson (j);
    WriteLog (lsINFO, Ledger) << j;
}

} // skywell
