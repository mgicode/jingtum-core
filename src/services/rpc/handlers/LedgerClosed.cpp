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
#include <services/rpc/Context.h>
#include <network/peers/UniqueNodeList.h>
#include <services/rpc/impl/Handler.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <services/net/RPCErr.h>
#include <common/misc/NetworkOPs.h>
#include <network/resource/Fees.h>
#include <services/rpc/impl/LookupLedger.h>
#include <services/rpc/impl/AccountFromString.h>


namespace skywell {

Json::Value doLedgerClosed (RPC::Context& context)
{
    uint256 uLedger = context.netOps.getClosedLedgerHash ();

    Json::Value jvResult;
    jvResult[jss::ledger_index] = context.netOps.getLedgerID (uLedger);
    jvResult[jss::ledger_hash] = to_string (uLedger);

    return jvResult;
}

} // skywell
