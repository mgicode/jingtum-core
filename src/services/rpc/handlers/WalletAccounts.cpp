//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2012-2014 Skywell Labs Inc.

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
#include <services/rpc/impl/Accounts.h>
#include <services/rpc/impl/GetMasterGenerator.h>
#include <services/rpc/impl/LookupLedger.h>
#include <services/rpc/impl/Handler.h>
#include <protocol/JsonFields.h>
#include <services/rpc/Context.h>

namespace skywell {

// {
//   seed: <string>
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
// }
Json::Value doWalletAccounts (RPC::Context& context)
{
    Ledger::pointer ledger;
    Json::Value jvResult
            = RPC::lookupLedger (context.params, ledger, context.netOps);

    if (!ledger)
        return jvResult;

    SkywellAddress   naSeed;

    if (!context.params.isMember (jss::seed)
        || !naSeed.setSeedGeneric (context.params[jss::seed].asString ()))
    {
        return rpcError (rpcBAD_SEED);
    }

    // Try the seed as a master seed.
    SkywellAddress naMasterGenerator
            = SkywellAddress::createGeneratorPublic (naSeed);

    Json::Value jsonAccounts
            = RPC::accounts (ledger, naMasterGenerator, context.netOps);

    if (jsonAccounts.empty ())
    {
        // No account via seed as master, try seed a regular.
        Json::Value ret = RPC::getMasterGenerator (
            ledger, naSeed, naMasterGenerator, context.netOps);

        if (!ret.empty ())
            return ret;

        ret[jss::accounts]
                = RPC::accounts (ledger, naMasterGenerator, context.netOps);
        return ret;
    }
    else
    {
        // Had accounts via seed as master, return them.
        return RPC::makeObjectValue (jsonAccounts, jss::accounts);
    }
}

} // skywell
