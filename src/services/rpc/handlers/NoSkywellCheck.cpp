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
#include <services/rpc/impl/Tuning.h>
#include <transaction/paths/SkywellState.h>
#include <protocol/TxFlags.h>
#include <services/rpc/Context.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <services/rpc/impl/LookupLedger.h>
#include <services/net/RPCErr.h>
#include <services/rpc/impl/AccountFromString.h>

namespace skywell {

static void fillTransaction (
    Json::Value& txArray,
    SkywellAddress const& account,
    std::uint32_t& sequence,
    Ledger::ref ledger)
{
    txArray["Sequence"] = Json::UInt (sequence++);
    txArray["Account"] = account.humanAccountID ();
    txArray["Fee"] = Json::UInt (ledger->scaleFeeLoad (10, false));
}

// {
//   account: <account>|<account_public_key>
//   account_index: <number>        // optional, defaults to 0.
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
//   limit: integer                 // optional, number of problems
//   role: gateway|user             // account role to assume
//   transactions: true             // optional, reccommend transactions
// }
Json::Value doNoSkywellCheck (RPC::Context& context)
{
    auto const& params (context.params);
    if (! params.isMember (jss::account))
        return RPC::missing_field_error ("account");

    if (! params.isMember ("role"))
        return RPC::missing_field_error ("role");
    bool roleGateway = false;
    {
        std::string const role = params["role"].asString();
        if (role == "gateway")
            roleGateway = true;
        else if (role != "user")
        return RPC::invalid_field_message ("role");
    }

    unsigned int limit = 300;
    if (params.isMember (jss::limit))
    {
        auto const& jvLimit (params[jss::limit]);
        if (! jvLimit.isIntegral ())
            return RPC::expected_field_error ("limit", "unsigned integer");
        limit = jvLimit.isUInt () ? jvLimit.asUInt () :
            std::max (0, jvLimit.asInt ());
        if (context.role != Role::ADMIN)
        {
            limit = std::max (RPC::Tuning::minLinesPerRequest,
                std::min (limit, RPC::Tuning::maxLinesPerRequest));
        }
    }

    bool transactions = false;
    if (params.isMember (jss::transactions))
        transactions = params["transactions"].asBool();

    Ledger::pointer ledger;
    Json::Value result (RPC::lookupLedger (params, ledger, context.netOps));
    if (! ledger)
        return result;

    Json::Value dummy;
    Json::Value& jvTransactions =
        transactions ? (result[jss::transactions] = Json::arrayValue) : dummy;

    std::string strIdent (params[jss::account].asString ());
    bool bIndex (params.isMember (jss::account_index));
    int iIndex (bIndex ? params[jss::account_index].asUInt () : 0);
    SkywellAddress skywellAddress;

    Json::Value const jv (RPC::accountFromString (ledger, skywellAddress, bIndex,
        strIdent, iIndex, false, context.netOps));
    if (! jv.empty ())
    {
        for (Json::Value::const_iterator it (jv.begin ()); it != jv.end (); ++it)
            result[it.memberName ()] = it.key ();

        return result;
    }

    AccountState::pointer accountState = ledger->getAccountState (skywellAddress);
    if (! accountState)
        return rpcError (rpcACT_NOT_FOUND);

    std::uint32_t seq = accountState->peekSLE().getFieldU32 (sfSequence);

    Json::Value& problems = (result["problems"] = Json::arrayValue);

    bool bDefaultSkywell = (accountState->peekSLE().getFieldU32 (sfFlags) & lsfDefaultSkywell)  == 0;

    if (bDefaultSkywell & ! roleGateway)
    {
        problems.append ("You appear to have set your default skywell flag even though you "
            "are not a gateway. This is not recommended unless you are experimenting");
    }
    else if (roleGateway & ! bDefaultSkywell)
    {
        problems.append ("You should immediately set your default skywell flag");
        if (transactions)
        {
            Json::Value& tx = jvTransactions.append (Json::objectValue);
            tx["TransactionType"] = "AccountSet";
            tx["SetFlag"] = 8;
            fillTransaction (tx, skywellAddress, seq, ledger);
        }
    }

    auto const accountID = skywellAddress.getAccountID ();

    ledger->visitAccountItems (accountID, uint256(), 0, limit,
        [&](SLE::ref ownedItem)
        {
            if (ownedItem->getType() == ltSKYWELL_STATE)
            {
                bool const bLow = accountID == ownedItem->getFieldAmount(sfLowLimit).getIssuer();

                bool const bNoSkywell = ownedItem->getFieldU32(sfFlags) &
                   (bLow ? lsfLowNoSkywell : lsfHighNoSkywell);

                std::string problem;
                bool needFix = false;
                if (bNoSkywell & roleGateway)
                {
                    problem = "You should clear the no skywell flag on your ";
                    needFix = true;
                }
                else if (! roleGateway & ! bNoSkywell)
                {
                    problem = "You should probably set the no skywell flag on your ";
                    needFix = true;
                }
                if (needFix)
                {
                    Account peer =
                        ownedItem->getFieldAmount (bLow ? sfHighLimit : sfLowLimit).getIssuer();
                    STAmount peerLimit = ownedItem->getFieldAmount (bLow ? sfHighLimit : sfLowLimit);
                    problem += to_string (peerLimit.getCurrency());
                    problem += " line to ";
                    problem += to_string (peerLimit.getIssuer());
                    problems.append (problem);

                    STAmount limitAmount (ownedItem->getFieldAmount (bLow ? sfLowLimit : sfHighLimit));
                    limitAmount.setIssuer (peer);

                    Json::Value& tx = jvTransactions.append (Json::objectValue);
                    tx["TransactionType"] = "TrustSet";
                    tx["LimitAmount"] = limitAmount.getJson (0);
                    tx["Flags"] = bNoSkywell ? tfClearNoSkywell : tfSetNoSkywell;
                    fillTransaction(tx, skywellAddress, seq, ledger);

                    return true;
                }
            }
	    return false;
        });

    return result;
}

} // skywell
