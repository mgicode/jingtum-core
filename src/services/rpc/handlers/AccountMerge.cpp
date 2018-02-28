#include <BeastConfig.h>
#include <protocol/Indexes.h>
#include <services/rpc/impl/TransactionSign.h>
#include <list>
#include <protocol/JsonFields.h>
#include <services/rpc/Context.h>
#include <common/core/Job.h>
#include <main/Application.h>
#include <protocol/ErrorCodes.h>
#include <services/rpc/impl/LookupLedger.h>
#include <common/base/Log.h>
#include <common/misc/NetworkOPs.h>
#include <network/resource/Fees.h>
#include <protocol/ErrorCodes.h>
#include <services/rpc/RPCHandler.h>
#include <services/net/RPCErr.h>
#include <services/rpc/impl/Handler.h>
#include <common/json/to_string.h>

namespace skywell {

Json::Value doAccountMerge(RPC::Context& context)
{
  Json::Value jvResult;
  bool bFailHard = context.params.isMember (jss::fail_hard)
                && context.params[jss::fail_hard].asBool ();
  WriteLog (lsWARNING, RPCHandler) <<"---doAccountMerge-----begin--xxxxx--";
  Json::Value params = context.params;
  std::set<SLE::pointer> trustLines; //trust line
  if (! params.isMember (jss::tx_json))
     return RPC::missing_field_error (jss::tx_json);
  if (! params[jss::tx_json].isMember (jss::Account))
     return RPC::make_error (rpcSRC_ACT_MISSING,
            RPC::missing_field_message ("tx_json.Account"));
  SkywellAddress raSrcAddressID;
  if (! raSrcAddressID.setAccountID (params[jss::tx_json][jss::Account].asString ()))
     return RPC::make_error (rpcSRC_ACT_MALFORMED,
            RPC::invalid_field_message ("tx_json.Account"));
  if (!params[jss::tx_json].isMember(jss::Destination))
     return RPC::make_error (rpcSRC_ACT_MALFORMED,
            RPC::invalid_field_message ("tx_json.Destination"));

  Ledger::pointer  ledger =context.netOps.getCurrentLedger();
  if (ledger)
  {
    ledger->visitAccountItems(raSrcAddressID.getAccountID(),[&trustLines](SLE::ref sle)
                               {
                                  if (sle && sle->getType () == ltSKYWELL_STATE)
                                  {
                                        trustLines.emplace(sle);
                                  }
                               });
  }
  std::list<Json::Value> jvParams_list; //save params
  Json::Value jvParams = params;
  for (auto const& sle :trustLines)
  {//
     Json::Value jvTx_json;
     STAmount saBalance = sle ->getFieldAmount (sfBalance);
     if (saBalance <zero)
          saBalance.negate();
     Account issuer = ((raSrcAddressID == sle->getFirstOwner() )
                      ? sle->getSecondOwner() : sle->getFirstOwner()).getAccountID();
     saBalance.setIssuer(issuer);
     //payment parm
     jvTx_json[jss::Account] = params[jss::tx_json][jss::Account];
     jvTx_json[jss::Destination] = params[jss::tx_json][jss::Destination];
     jvTx_json[jss::TransactionType] = "Payment";
     jvTx_json[jss::Amount]= saBalance.getJson(0);
     jvParams[jss::tx_json] = jvTx_json;
     jvParams_list.push_back(jvParams); //save payment parm
     //  setTrust parm
     jvTx_json = Json::objectValue;
     jvTx_json[jss::Account] = params[jss::tx_json][jss::Account];
     jvTx_json[jss::TransactionType] = "TrustSet";
     jvTx_json[jss::LimitAmount] = saBalance.zeroed().getJson(0);
     jvParams[jss::tx_json] = jvTx_json;
     jvParams_list.push_back(jvParams); //save payment parm
  }
  {
    Json::Value jvTx_json;
    jvTx_json[jss::Account] = params[jss::tx_json][jss::Account];
    jvTx_json[jss::Destination] = params[jss::tx_json][jss::Destination];
    jvTx_json[jss::TransactionType] = "AccountMerge";
    jvParams[jss::tx_json] = jvTx_json;
    jvParams_list.push_back(jvParams); //save AccountMerge parm
  }
  int i =0;
  bool brollback = false;
  Json::Value jvRes;
  for (auto const& p:jvParams_list)
  {
    jvRes = RPC::transactionSign (p, true, bFailHard, context.netOps, context.role); 
    if (jvRes.isMember(jss::engine_result) && jvRes[jss::engine_result] != "tesSUCCESS")
    {
       //WriteLog (lsWARNING, RPCHandler) <<"--merge transactionSign error no:"<<i<<"\nres:"<<jvRes;
       brollback = true;
       break;
    }
 //    WriteLog (lsWARNING, RPCHandler) <<"--merge transactionSign no:"<<i++<<"\nres:"<<jvRes<<"\nbrollback:"<<brollback;
  }
  jvResult = jvRes;
  
  WriteLog (lsWARNING, RPCHandler) <<"---doAccountMerge-----end-------\nres:" << jvResult;
  
  return jvResult;
}

}
