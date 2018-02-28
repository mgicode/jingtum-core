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
#include <ledger/LedgerToJson.h>
#include <common/core/LoadFeeTrack.h>
#include <common/json/Object.h>
#include <protocol/ErrorCodes.h>
#include <services/rpc/handlers/Ledger.h>
#include <services/server/Role.h>
#include <ledger/LedgerMaster.h>
#include <services/rpc/impl/LookupLedger.h>
#include <network/resource/Fees.h>

namespace skywell {
namespace RPC {

LedgerHandler::LedgerHandler (Context& context) : context_ (context)
{
}

Status LedgerHandler::check ()
{
    auto const& params = context_.params;
    bool needsLedger = params.isMember (jss::ledger) ||
            params.isMember (jss::ledger_hash) ||
            params.isMember (jss::ledger_index);
    if (!needsLedger)
        return Status::OK;

    if (auto s = RPC::lookupLedger (params, ledger_, context_.netOps, result_))
        return s;

    bool bFull = params[jss::full].asBool();
    bool bTransactions = params[jss::transactions].asBool();
    bool bAccounts = params[jss::accounts].asBool();
    bool bExpand = params[jss::expand].asBool();
    bool bBinary = params[jss::binary].asBool();

    options_ = (bFull ? LedgerFill::full : 0)
            | (bExpand ? LedgerFill::expand : 0)
            | (bTransactions ? LedgerFill::dumpTxrp : 0)
            | (bAccounts ? LedgerFill::dumpState : 0)
            | (bBinary ? LedgerFill::binary : 0);

    if (bFull || bAccounts)
    {
        // Until some sane way to get full ledgers has been implemented,
        // disallow retrieving all state nodes.
        if (context_.role != Role::ADMIN)
            return rpcNO_PERMISSION;

        if (getApp().getFeeTrack().isLoadedLocal() &&
            context_.role != Role::ADMIN)
        {
            return rpcTOO_BUSY;
        }
        context_.loadType = bBinary ? Resource::feeMediumBurdenRPC :
            Resource::feeHighBurdenRPC;
    }

    return Status::OK;
}

} // RPC
} // skywell
