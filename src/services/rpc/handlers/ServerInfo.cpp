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
#include <services/rpc/handlers/ServerInfo.h>
#include <common/core/LoadMonitor.h>
#include <common/misc/NetworkOPs.h>
#include <common/base/StringUtilities.h>
#include <common/json/json_reader.h>
#include <common/core/LoadFeeTrack.h>
#include <protocol/TxFlags.h>
#include <protocol/Indexes.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <ledger/LedgerMaster.h>
#include <services/rpc/impl/KeypairForSignature.h>
#include <services/rpc/impl/TransactionSign.h>
#include <services/rpc/impl/Tuning.h>
#include <services/rpc/impl/LegacyPathFind.h>
#include <services/net/RPCErr.h>
#include <transaction/paths/FindPaths.h>
#include <main/Application.h>
#include <protocol/UintTypes.h>

namespace skywell {

Json::Value doThreadInfo  (RPC::Context& context)
{
  auto& params = context.params;
  Json::Value jvResult (Json::objectValue);
  bool isReset = params.isMember(jss::reset);
  bool isJobName = params.isMember(jss::job_name);
  std::string jobName = isJobName ? params[jss::job_name].asString():"";
  if (isReset) {
    skywell::LoadMonitor::reset(jobName);
  }
  else {
    jvResult[jss::info] = skywell::LoadMonitor::getThreadInfo(jobName);
  }
  return jvResult;
}

Json::Value parseTxBlob (Json::Value const& txBlob)
{
  Json::Value jvResult (Json::objectValue);
  try
  {
    std::pair<Blob, bool> ret(strUnHex (txBlob.asString ()));
    if (!ret.second || !ret.first.size ())
       return rpcError (rpcINVALID_PARAMS);
    Serializer sTrans (ret.first);
    SerialIter sitTrans (sTrans) ;
    STObject object(sitTrans,sfTransaction);
    jvResult[jss::info] = object.getJson(0);
  }
  catch(...)
  {
       return rpcError (rpcTX_BLOB_INVALID);
  }
  return jvResult;
}

Json::Value doShowBlob  (RPC::Context& context)
{
  auto& params = context.params;
  Json::Value jvResult (Json::objectValue);
  if(params.isMember(jss::tx_blob))
  {
    jvResult = parseTxBlob(params[jss::tx_blob]);
  }
  else
  {
    return rpcError (rpcINVALID_PARAMS);
  }
  return jvResult;
}

Json::Value doRPCInfo  (RPC::Context& context)
{
    auto& params = context.params;
    Json::Value ret (Json::objectValue);
    bool isReset = params.isMember(jss::reset);
    bool isPrint = params.isMember(jss::print);
    bool isCmd = params.isMember(jss::rpc_command);
    if (isPrint) {
       if (isCmd && !isReset)  {
          RPC::RPCInfo::isprint = true ;
          RPC::RPCInfo::cmdstr = params[jss::rpc_command].asString();
       }
       else if (!isCmd && isReset) {
          RPC::RPCInfo::isprint = false;
       }
       else if (isCmd && isReset )  {
         RPC::RPCInfo::cmdstr.clear();
       }
       else if ( !isCmd && !isReset )  {
         RPC::RPCInfo::cmdstr.clear();
         RPC::RPCInfo::isprint = true ;
       }
    }
    else {
      if (isReset) {
        RPC::RPCInfo::isprint = false;
        RPC::RPCInfo::cmdstr.clear();
        RPC::RPCInfo::reset();
        return ret;
      }
    }
    ret[jss::info] =  RPC::RPCInfo::show(); 
    return ret;
}


Json::Value doServerInfo (RPC::Context& context)
{
    Json::Value ret (Json::objectValue);

    ret[jss::info] = context.netOps.getServerInfo (
        true, context.role == Role::ADMIN);

    return ret;
}

Json::Value doBlacklistInfo (RPC::Context& context)
{
    int const JSON_PAGE_LENGTH = 256;

    Ledger::pointer lpLedger;
    Json::Value jvResult (Json::objectValue);

//    Json::Value jvResult = RPC::lookupLedger (params, lpLedger, context.netOps);
	lpLedger =  context.netOps.getCurrentLedger();
    if (!lpLedger)
        return jvResult;

    uint256 resumePoint;


    int limit = -1;
    int maxLimit = JSON_PAGE_LENGTH;

    limit = maxLimit;

    jvResult[jss::ledger_hash] = to_string (lpLedger->getHash());
    jvResult[jss::ledger_index] = std::to_string( lpLedger->getLedgerSeq ());

    Json::Value& nodes = (jvResult[jss::state] = Json::arrayValue);
    SHAMap& map = *(lpLedger->peekAccountStateMap ());

    for (;;)
    {
       std::shared_ptr<SHAMapItem> item = map.peekNextItem (resumePoint);
       if (!item)
           break;
       resumePoint = item->getTag();

       {
           SLE sle (item->peekSerializer(), item->getTag ());
	     if(sle.getType() == ltBLACKLIST)
	     {
			 if (limit-- <= 0)
			 {
				 --resumePoint;
				 jvResult[jss::marker] = to_string (resumePoint);
				 break;
			 }
	         Json::Value& entry = nodes.append (sle.getJson (0));
	         entry[jss::index] = to_string (item->getTag ());
	    }
       }
    }

    return jvResult;
}


Account findIssuer(STAmount const& balance,STAmount const& lowLimit,STAmount const& highLimit)
{
  if (lowLimit.getIssuer() == highLimit.getIssuer() )
  {
     return lowLimit.getIssuer();
  }
  STAmount staZero =  balance.zeroed();
  if (lowLimit == staZero && highLimit != staZero )
  {
    return lowLimit.getIssuer();
  }
  else if (highLimit == staZero && lowLimit != staZero)
  {
    return highLimit.getIssuer();
  }
  else if (highLimit != staZero && lowLimit != staZero)
  {
    if (balance.negative() )
    {
       return lowLimit.getIssuer();
    }
    return highLimit.getIssuer();
  }
  return noAccount();
}



Json::Value doShowData (RPC::Context& context)
{
    std::map<std::string,std::pair< std::vector<int>,STAmount> > mRes1;

    Ledger::pointer lpLedger;
    Json::Value jvResult (Json::objectValue);

    lpLedger =  context.netOps.getCurrentLedger();
    if (!lpLedger)
        return jvResult;

    uint256 resumePoint;

    jvResult[jss::ledger_hash] = to_string (lpLedger->getHash());
    jvResult[jss::ledger_index] = std::to_string( lpLedger->getLedgerSeq ());

    SHAMap& map = *(lpLedger->peekAccountStateMap ());
    int i =1;
    for (;;)
    {
       std::shared_ptr<SHAMapItem> item = map.peekNextItem (resumePoint);
       if (!item)
           break;
       resumePoint = item->getTag();
       SLE sle (item->peekSerializer(), item->getTag ());
       auto accountType = sle.getType();
       if(accountType == ltACCOUNT_ROOT || accountType == ltSKYWELL_STATE)       //SWT 
       {
          STAmount saBalance   = sle.getFieldAmount (sfBalance);
          auto cur = saBalance.getCurrency();
          auto issuer = saBalance.getIssuer();
          if (accountType == ltSKYWELL_STATE)
          {
             STAmount saBalancel   = sle.getFieldAmount (sfLowLimit);
             STAmount saBalanceh   = sle.getFieldAmount (sfHighLimit);
             issuer = findIssuer(saBalance,saBalancel,saBalanceh);
          }
          else if (accountType == ltACCOUNT_ROOT)
          {
              if ( Account(sle.getFieldAccount160 (sfAccount)) == xrpAccount() )
              {
                continue;
              }
          }
          std::string key = to_string(issuer)+"|"+to_string(cur);
          
          i = 1 ;
          if (saBalance.negative())
          {
             saBalance = - saBalance;
          }

          if ( saBalance.zeroed() == saBalance )
          {
             i =0 ;
          }
          if ( mRes1.find(key) != mRes1.end() )
          {
                mRes1[key].first[i] += 1;
                mRes1[key].second += saBalance;
          }
          else
          {
             std::vector<int> vec(2,0);
             vec[i] =1 ;
             mRes1.insert(std::map<std::string, std::pair< std::vector<int>, STAmount> >::value_type(key, std::pair< std::vector<int>, STAmount>(vec,saBalance)));
          }
        }
      }
     std::map<std::string,std::pair< std::vector<int> ,STAmount> >::iterator ib = mRes1.begin();
     std::map<std::string,std::pair< std::vector<int> ,STAmount> >::iterator ie = mRes1.end();

     Json::Value jsonLines = Json::arrayValue;
     i =0 ;
     for (;ib!=ie;++ib)
     {
        Json::Value jsonTmp = Json::objectValue;
        //jsonTmp[jss::issuer_Currency] = to_string(ib->first);
        jsonTmp[jss::total_amount] = ib->second.second.getText();
        //jsonTmp[jss::Amount] = to_string(ib->second.first[1]);
        //jsonTmp[jss::no_Amount] = to_string(ib->second.first[0]);
        jsonLines.append(jsonTmp);
     }
    jvResult[jss::total_users] = mRes1["jjjjjjjjjjjjjjjjjjjjjhoLvTp|SWT"].first[1];
    jvResult[jss::info] = Json::Value(jsonLines);
    return jvResult;
}




} // skywell
