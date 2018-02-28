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
#include <transaction/paths/SkywellTrust.h>
#include <services/rpc/Context.h>
#include <network/peers/UniqueNodeList.h>
#include <services/rpc/impl/Handler.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <services/net/RPCErr.h>
//#include <main/Application.h>
//#include <ledger/LedgerMaster.h>
#include <services/rpc/impl/AccountFromString.h>
#include <network/resource/Fees.h>
#include <services/rpc/impl/LookupLedger.h>

namespace skywell {

struct VisitData
{
    std::vector <SkywellState::pointer> items;
    Account const& accountID;
    SkywellAddress const& skywellAddressPeer;
    Account const& raPeerAccount;
};

void addLine (Json::Value& jsonLines, SkywellState const& line)
{
    STAmount const& saBalance (line.getBalance ());
    STAmount const& saLimit (line.getLimit ());
    STAmount const& saLimitPeer (line.getLimitPeer ());
    Json::Value& jPeer (jsonLines.append (Json::objectValue));

    jPeer[jss::account] = to_string (line.getAccountIDPeer ());
    // Amount reported is positive if current account holds other
    // account's IOUs.
    //
    // Amount reported is negative if other account holds current
    // account's IOUs.
    jPeer[jss::balance] = saBalance.getText ();
    jPeer[jss::currency] = saBalance.getHumanCurrency ();
    jPeer[jss::limit] = saLimit.getText ();
    jPeer[jss::limit_peer] = saLimitPeer.getText ();
    jPeer[jss::quality_in]
        = static_cast<Json::UInt> (line.getQualityIn ());
    jPeer[jss::quality_out]
        = static_cast<Json::UInt> (line.getQualityOut ());
    if (line.getAuth ())
        jPeer[jss::authorized] = true;
    if (line.getAuthPeer ())
        jPeer[jss::peer_authorized] = true;
    if (line.getNoSkywell ())
        jPeer[jss::no_skywell] = true;
    if (line.getNoSkywellPeer ())
        jPeer[jss::no_skywell_peer] = true;
    if (line.getFreeze ())
        jPeer[jss::freeze] = true;
    if (line.getFreezePeer ())
        jPeer[jss::freeze_peer] = true;
}

// {
//   show_trust <account> [<currency> [<ledger>]]
//   account_index: <number>        // optional, defaults to 0.
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
// }
//Show who trust me
Json::Value doShowTrust (RPC::Context& context)
{
  auto const& params (context.params);
  if (! params.isMember (jss::account))
      return RPC::missing_field_error (jss::account);
  std::string strIdent (params[jss::account].asString ());
  Ledger::pointer ledger;
  Json::Value result (RPC::lookupLedger (params, ledger, context.netOps));
  if (! ledger)
     return result;

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
  if (! ledger->hasAccount (skywellAddress))
      return rpcError (rpcACT_NOT_FOUND);
  
  Currency currency;
  bool isCurrency = params.isMember(jss::currency);    //
  if (isCurrency) {
    if (!to_currency(currency,params[jss::currency].asString())) {
       return rpcError (rpcINVALID_PARAMS);
    }
    result[jss::currency] = params[jss::currency].asString();
 }

  Json::Value& jsonLines (result[jss::lines] = Json::arrayValue);
  Account const& raAccount(skywellAddress.getAccountID ());
  
  std::vector <SkywellState::pointer> items;
  ledger->visitAccountItems(raAccount,
            [&](SLE::ref sleCur)  
            {
              auto const line (SkywellState::makeItem (raAccount, sleCur));
              if (line != nullptr )
              {
                 STAmount const& saLimitPeer (line->getLimitPeer ());
                 STAmount const& saBalance (line->getBalance ());
                 if (saLimitPeer.getText () != "0") {
                   if (isCurrency) {
                     if (currency == saBalance.getCurrency()) {
                       items.emplace_back (line);
                     } 
                   }
                   else {
                      items.emplace_back (line);
                   }
                 }
                 return ;
              }
              return ;
            });
  result[jss::account] = skywellAddress.humanAccountID ();
  for (auto const& item :items)
     addLine (jsonLines, *item.get ());

  context.loadType = Resource::feeMediumBurdenRPC;
  return result;   
}

// {
//   account: <account>|<account_public_key>
//   account_index: <number>        // optional, defaults to 0.
//   ledger_hash : <ledger>
//   ledger_index : <ledger_index>
//   limit: integer                 // optional
//   marker: opaque                 // optional, resume previous query
// }
Json::Value doAccountLines (RPC::Context& context)
{
    auto const& params (context.params);
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

    Ledger::pointer ledger;
    Json::Value result (RPC::lookupLedger (params, ledger, context.netOps));
    if (! ledger)
        return result;

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

    if (! ledger->hasAccount (skywellAddress))
        return rpcError (rpcACT_NOT_FOUND);

    std::string strPeer (params.isMember (jss::peer)
        ? params[jss::peer].asString () : "");
    bool bPeerIndex (params.isMember (jss::peer_index));
    int iPeerIndex (bPeerIndex ? params[jss::peer_index].asUInt () : 0);

    SkywellAddress skywellAddressPeer;

    if (! strPeer.empty ())
    {
        result[jss::peer] = skywellAddress.humanAccountID ();

        if (bPeerIndex)
            result[jss::peer_index] = iPeerIndex;

        result = RPC::accountFromString (ledger, skywellAddressPeer, bPeerIndex,
            strPeer, iPeerIndex, false, context.netOps);

        if (! result.empty ())
            return result;
    }

    Account raPeerAccount;
    if (skywellAddressPeer.isValid ())
        raPeerAccount = skywellAddressPeer.getAccountID ();

    unsigned int limit;
    if (params.isMember (jss::limit))
    {
        auto const& jvLimit (params[jss::limit]);
        if (! jvLimit.isIntegral ())
            return RPC::expected_field_error (jss::limit, "unsigned integer");

        limit = jvLimit.isUInt () ? jvLimit.asUInt () :
            std::max (0, jvLimit.asInt ());

        if (context.role != Role::ADMIN)
        {
            limit = std::max (RPC::Tuning::minLinesPerRequest,
                std::min (limit, RPC::Tuning::maxLinesPerRequest));
        }
    }
    else
    {
        limit = RPC::Tuning::defaultLinesPerRequest;
    }

    Json::Value& jsonLines (result[jss::lines] = Json::arrayValue);
    Account const& raAccount(skywellAddress.getAccountID ());
    VisitData visitData = { {}, raAccount, skywellAddressPeer, raPeerAccount };
    unsigned int reserve (limit);
    uint256 startAfter;
    std::uint64_t startHint;

    if (params.isMember (jss::marker))
    {
        // We have a start point. Use limit - 1 from the result and use the
        // very last one for the resume.
        Json::Value const& marker (params[jss::marker]);

        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        startAfter.SetHex (marker.asString ());
        SLE::pointer sleLine (ledger->getSLEi (startAfter));

        if (sleLine == nullptr || sleLine->getType () != ltSKYWELL_STATE)
            return rpcError (rpcINVALID_PARAMS);

        if (sleLine->getFieldAmount (sfLowLimit).getIssuer () == raAccount)
            startHint = sleLine->getFieldU64 (sfLowNode);
        else if (sleLine->getFieldAmount (sfHighLimit).getIssuer () == raAccount)
            startHint = sleLine->getFieldU64 (sfHighNode);
        else
            return rpcError (rpcINVALID_PARAMS);

        // Caller provided the first line (startAfter), add it as first result
        auto const line (SkywellState::makeItem (raAccount, sleLine));
        if (line == nullptr)
            return rpcError (rpcINVALID_PARAMS);

        addLine (jsonLines, *line);
        visitData.items.reserve (reserve);
    }
    else
    {
        startHint = 0;
        // We have no start point, limit should be one higher than requested.
        visitData.items.reserve (++reserve);
    }

    if (! ledger->visitAccountItems (raAccount, startAfter, startHint, reserve,
        [&visitData](SLE::ref sleCur)
        {
            auto const line (SkywellState::makeItem (visitData.accountID, sleCur));
            if (line != nullptr &&
                (! visitData.skywellAddressPeer.isValid () ||
                visitData.raPeerAccount == line->getAccountIDPeer ()))
            {
                visitData.items.emplace_back (line);
                return true;
            }

            return false;
        }))
    {
        return rpcError (rpcINVALID_PARAMS);
    }

    if (visitData.items.size () == reserve)
    {
        result[jss::limit] = limit;

        SkywellState::pointer line (visitData.items.back ());
        result[jss::marker] = to_string (line->peekSLE ().getIndex ());
        visitData.items.pop_back ();
    }

    result[jss::account] = skywellAddress.humanAccountID ();

    for (auto const& item : visitData.items)
        addLine (jsonLines, *item.get ());

    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

struct VisitData1
{
    std::vector <SkywellTrust::pointer> items;
    Account const& accountID;
    SkywellAddress const& skywellAddressPeer;
    Account const& raPeerAccount;
};

void addTrust (Json::Value& jsonLines, SkywellTrust const& line)
{
    STAmount const& saBalance (line.getBalance ());
    STAmount const& saLimit (line.getLimit ());
    STAmount const& saType (line.getRelationType ()) ;
//    STAmount const& saLimitPeer (line.getLimitPeer ());
    Json::Value& jPeer (jsonLines.append (Json::objectValue));

    jPeer[jss::limit_peer] = to_string (line.getAccountIDPeer ());
    jPeer[jss::relation_type] = saType.getText ();
    // Amount reported is positive if current account holds other
    // account's IOUs.
    //
    // Amount reported is negative if other account holds current
    // account's IOUs.
    jPeer[jss::issuer] = to_string(saBalance.getIssuer());
    jPeer[jss::currency] = saBalance.getHumanCurrency ();
    jPeer[jss::limit] = saLimit.getText ();
}
Json::Value doAccountTrust (RPC::Context& context)
{
    auto const& params (context.params);
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

	if (!params.isMember(jss::relation_type))
		return RPC::missing_field_error(jss::relation_type);
	auto relation = params[jss::relation_type].asInt();

    Ledger::pointer ledger;
    Json::Value result (RPC::lookupLedger (params, ledger, context.netOps));
    if (! ledger)
        return result;

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

    if (! ledger->hasAccount (skywellAddress))
        return rpcError (rpcACT_NOT_FOUND);

    std::string strPeer (params.isMember (jss::peer)
        ? params[jss::peer].asString () : "");
    bool bPeerIndex (params.isMember (jss::peer_index));
    int iPeerIndex (bIndex ? params[jss::peer_index].asUInt () : 0);

    SkywellAddress skywellAddressPeer;

    if (! strPeer.empty ())
    {
        result[jss::peer] = skywellAddress.humanAccountID ();

        if (bPeerIndex)
            result[jss::peer_index] = iPeerIndex;

        result = RPC::accountFromString (ledger, skywellAddressPeer, bPeerIndex,
            strPeer, iPeerIndex, false, context.netOps);

        if (! result.empty ())
            return result;
    }

    Account raPeerAccount;
    if (skywellAddressPeer.isValid ())
        raPeerAccount = skywellAddressPeer.getAccountID ();

    unsigned int limit;
    if (params.isMember (jss::limit))
    {
        auto const& jvLimit (params[jss::limit]);
        if (! jvLimit.isIntegral ())
            return RPC::expected_field_error (jss::limit, "unsigned integer");

        limit = jvLimit.isUInt () ? jvLimit.asUInt () :
            std::max (0, jvLimit.asInt ());

        if (context.role != Role::ADMIN)
        {
            limit = std::max (RPC::Tuning::minLinesPerRequest,
                std::min (limit, RPC::Tuning::maxLinesPerRequest));
        }
    }
    else
    {
        limit = RPC::Tuning::defaultLinesPerRequest;
    }

    Json::Value& jsonLines (result[jss::lines] = Json::arrayValue);
    Account const& raAccount(skywellAddress.getAccountID ());
    VisitData1 visitData = { {}, raAccount, skywellAddressPeer, raPeerAccount };
    unsigned int reserve (limit);
    uint256 startAfter;
    std::uint64_t startHint;

    if (params.isMember (jss::marker))
    {
        // We have a start point. Use limit - 1 from the result and use the
        // very last one for the resume.
        Json::Value const& marker (params[jss::marker]);

        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        startAfter.SetHex (marker.asString ());
        SLE::pointer sleLine (ledger->getSLEi (startAfter));

        if (sleLine == nullptr || sleLine->getType () != ltTrust_STATE)
            return rpcError (rpcINVALID_PARAMS);

        if (sleLine->getFieldAmount (sfLowLimit).getIssuer () == raAccount)
            startHint = sleLine->getFieldU64 (sfLowNode);
        else if (sleLine->getFieldAmount (sfHighLimit).getIssuer () == raAccount)
            startHint = sleLine->getFieldU64 (sfHighNode);
        else
            return rpcError (rpcINVALID_PARAMS);

        // Caller provided the first line (startAfter), add it as first result
        auto const line (SkywellTrust::makeItem (raAccount, sleLine));
        if (line == nullptr)
            return rpcError (rpcINVALID_PARAMS);

        addTrust (jsonLines, *line);
        visitData.items.reserve (reserve);
    }
    else
    {
        startHint = 0;
        // We have no start point, limit should be one higher than requested.
        visitData.items.reserve (++reserve);
    }

    if (! ledger->visitAccountItems (raAccount, startAfter, startHint, reserve,
        [&visitData,&relation](SLE::ref sleCur)
        {
            auto const line (SkywellTrust::makeItem (visitData.accountID, sleCur));
            if (line != nullptr &&
                (! visitData.skywellAddressPeer.isValid () ||
                visitData.raPeerAccount == line->getAccountIDPeer ()) && (line->getLimit() >zero))
            {
              //  visitData.items.emplace_back (line);
				if (line->getRelationType() == relation)
				{
					visitData.items.emplace_back(line);
				}
                return true;
            }

            return false;
        }))
    {
        return rpcError (rpcINVALID_PARAMS);
    }

    if (visitData.items.size () == reserve)
    {
        result[jss::limit] = limit;

        SkywellTrust::pointer line (visitData.items.back ());
        result[jss::marker] = to_string (line->peekSLE ().getIndex ());
        visitData.items.pop_back ();
    }

    result[jss::account] = skywellAddress.humanAccountID ();

    for (auto const& item : visitData.items)
        addTrust (jsonLines, *item.get ());

    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

void addTrustcp (Json::Value& jsonLines, SkywellTrust const& line)
{
    STAmount const& saBalance (line.getBalance ());
    STAmount const& saLimit (line.getLimitPeer ());
    STAmount const& saType (line.getRelationType ()) ;
//    STAmount const& saLimitPeer (line.getLimitPeer ());
    Json::Value& jPeer (jsonLines.append (Json::objectValue));

    jPeer[jss::limit_peer] = to_string (line.getAccountIDPeer ());
    jPeer[jss::relation_type] = saType.getText ();
    // Amount reported is positive if current account holds other
    // account's IOUs.
    //
    // Amount reported is negative if other account holds current
    // account's IOUs.
    jPeer[jss::issuer] = to_string(saBalance.getIssuer());
    jPeer[jss::currency] = saBalance.getHumanCurrency ();
    jPeer[jss::limit] = saLimit.getText ();
}
Json::Value doAccountTrustcp (RPC::Context& context)
{
    auto const& params (context.params);
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

	if (!params.isMember(jss::relation_type))
		return RPC::missing_field_error(jss::relation_type);
	auto relation = params[jss::relation_type].asInt();

    Ledger::pointer ledger;
    Json::Value result (RPC::lookupLedger (params, ledger, context.netOps));
    if (! ledger)
        return result;

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

    if (! ledger->hasAccount (skywellAddress))
        return rpcError (rpcACT_NOT_FOUND);

    std::string strPeer (params.isMember (jss::peer)
        ? params[jss::peer].asString () : "");
    bool bPeerIndex (params.isMember (jss::peer_index));
    int iPeerIndex (bIndex ? params[jss::peer_index].asUInt () : 0);

    SkywellAddress skywellAddressPeer;

    if (! strPeer.empty ())
    {
        result[jss::peer] = skywellAddress.humanAccountID ();

        if (bPeerIndex)
            result[jss::peer_index] = iPeerIndex;

        result = RPC::accountFromString (ledger, skywellAddressPeer, bPeerIndex,
            strPeer, iPeerIndex, false, context.netOps);

        if (! result.empty ())
            return result;
    }

    Account raPeerAccount;
    if (skywellAddressPeer.isValid ())
        raPeerAccount = skywellAddressPeer.getAccountID ();

    unsigned int limit;
    if (params.isMember (jss::limit))
    {
        auto const& jvLimit (params[jss::limit]);
        if (! jvLimit.isIntegral ())
            return RPC::expected_field_error (jss::limit, "unsigned integer");

        limit = jvLimit.isUInt () ? jvLimit.asUInt () :
            std::max (0, jvLimit.asInt ());

        if (context.role != Role::ADMIN)
        {
            limit = std::max (RPC::Tuning::minLinesPerRequest,
                std::min (limit, RPC::Tuning::maxLinesPerRequest));
        }
    }
    else
    {
        limit = RPC::Tuning::defaultLinesPerRequest;
    }

    Json::Value& jsonLines (result[jss::lines] = Json::arrayValue);
    Account const& raAccount(skywellAddress.getAccountID ());
    VisitData1 visitData = { {}, raAccount, skywellAddressPeer, raPeerAccount };
    unsigned int reserve (limit);
    uint256 startAfter;
    std::uint64_t startHint;

    if (params.isMember (jss::marker))
    {
        // We have a start point. Use limit - 1 from the result and use the
        // very last one for the resume.
        Json::Value const& marker (params[jss::marker]);

        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        startAfter.SetHex (marker.asString ());
        SLE::pointer sleLine (ledger->getSLEi (startAfter));

        if (sleLine == nullptr || sleLine->getType () != ltTrust_STATE)
            return rpcError (rpcINVALID_PARAMS);

        if (sleLine->getFieldAmount (sfLowLimit).getIssuer () == raAccount)
            startHint = sleLine->getFieldU64 (sfLowNode);
        else if (sleLine->getFieldAmount (sfHighLimit).getIssuer () == raAccount)
            startHint = sleLine->getFieldU64 (sfHighNode);
        else
            return rpcError (rpcINVALID_PARAMS);

        // Caller provided the first line (startAfter), add it as first result
        auto const line (SkywellTrust::makeItem (raAccount, sleLine));
        if (line == nullptr)
            return rpcError (rpcINVALID_PARAMS);

        addTrustcp (jsonLines, *line);
        visitData.items.reserve (reserve);
    }
    else
    {
        startHint = 0;
        // We have no start point, limit should be one higher than requested.
        visitData.items.reserve (++reserve);
    }

    if (! ledger->visitAccountItems (raAccount, startAfter, startHint, reserve,
        [&visitData,&relation](SLE::ref sleCur)
        {
            auto const line (SkywellTrust::makeItem (visitData.accountID, sleCur));
            if (line != nullptr &&
                (! visitData.skywellAddressPeer.isValid () ||
                visitData.raPeerAccount == line->getAccountIDPeer ())&& (line->getLimitPeer() >zero))
            {
               // visitData.items.emplace_back (line);
				if (line->getRelationType() == relation)
				{
					visitData.items.emplace_back(line);
				}
                return true;
            }

            return false;
        }))
    {
        return rpcError (rpcINVALID_PARAMS);
    }

    if (visitData.items.size () == reserve)
    {
        result[jss::limit] = limit;

        SkywellTrust::pointer line (visitData.items.back ());
        result[jss::marker] = to_string (line->peekSLE ().getIndex ());
        visitData.items.pop_back ();
    }

    result[jss::account] = skywellAddress.humanAccountID ();

    for (auto const& item : visitData.items)
        addTrustcp (jsonLines, *item.get ());

    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}
} // skywell
