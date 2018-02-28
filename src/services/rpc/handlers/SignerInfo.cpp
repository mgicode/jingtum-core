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
#include <services/rpc/impl/LookupLedger.h>
#include <services/rpc/impl/AccountFromString.h>
#include <common/misc/NetworkOPs.h>
#include <common/base/StringUtilities.h>
#include <common/json/json_reader.h>
#include <common/core/LoadFeeTrack.h>
#include <protocol/TxFlags.h>
#include <protocol/Indexes.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <ledger/LedgerMaster.h>
#include <transaction/paths/FindPaths.h>
#include <services/rpc/impl/KeypairForSignature.h>
#include <services/rpc/impl/TransactionSign.h>
#include <services/rpc/impl/Tuning.h>
#include <services/rpc/impl/LegacyPathFind.h>
#include <services/net/RPCErr.h>
#include <main/Application.h>

namespace skywell {

Json::Value doSignerInfo(RPC::Context& context)
{
    auto const& params(context.params);
    if (!params.isMember(jss::account))
        return RPC::missing_field_error(jss::account);

    Ledger::pointer ledger;
    Json::Value result(RPC::lookupLedger(params, ledger, context.netOps));
    if (!ledger)
        return result;

    std::string strIdent(params[jss::account].asString());
    bool bIndex(params.isMember(jss::account_index));
    int iIndex(bIndex ? params[jss::account_index].asUInt() : 0);
    SkywellAddress skywellAddress;

    Json::Value const jv(RPC::accountFromString(ledger, skywellAddress, bIndex,
        strIdent, iIndex, false, context.netOps));
    if (!jv.empty())
    {
        for (Json::Value::const_iterator it(jv.begin()); it != jv.end(); ++it)
            result[it.memberName()] = it.key();

        return result;
    }

    if (!ledger->hasAccount(skywellAddress))
        return rpcError(rpcACT_NOT_FOUND);

    result[jss::account] = skywellAddress.humanAccountID();
    Json::Value& info(result[jss::info] = Json::objectValue);
    STLedgerEntry::pointer ledgerEntry = ledger->getSLEi(getSignStateIndex(skywellAddress.getAccountID()));

    if (ledgerEntry)
    {
        info[jss::quorum] = ledgerEntry->getFieldU32(sfQuorum);
        info[jss::master_weight] = ledgerEntry->getFieldU16(sfMasterWeight);

        STArray const& tSignEntries = ledgerEntry->getFieldArray(sfSignerEntries);
        Json::Value& signers(info[jss::signers] = Json::arrayValue);

        for (STObject const& tSignEntry : tSignEntries)
        {
            Json::Value& obj(signers.append(Json::objectValue));
            obj[jss::account] = tSignEntry.getFieldAccount(sfAccount).ToString();
            obj[jss::weight] = tSignEntry.getFieldU16(sfWeight);
        }
    }

    return result;
}
}
