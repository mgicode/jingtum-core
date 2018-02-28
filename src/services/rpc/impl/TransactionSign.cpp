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
#include <main/Application.h>
#include <common/misc/NetworkOPs.h>
#include <transaction/paths/FindPaths.h>
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
#include <protocol/STParsedJSON.h>
#include <common/json/to_string.h>

namespace skywell {

namespace RPC {
namespace RPCDetail {

// LedgerFacade methods

void LedgerFacade::snapshotAccountState (SkywellAddress const& accountID)
{
    if (!netOPs_) // Unit testing.
        return;

    ledger_ = netOPs_->getCurrentLedger ();
    accountID_ = accountID;
    accountState_ = netOPs_->getAccountState (ledger_, accountID_);
}

bool LedgerFacade::isValidAccount () const
{
    if (!ledger_) // Unit testing.
        return true;

    return static_cast <bool> (accountState_);
}

std::uint32_t LedgerFacade::getSeq () const
{
    if (!ledger_) // Unit testing.
        return 0;

    return accountState_->getSeq ();
}

Transaction::pointer LedgerFacade::submitTransactionSync (
    Transaction::ref tpTrans,
    bool bAdmin,
    bool bLocal,
    bool bFailHard,
    bool bSubmit)
{
    if (!netOPs_) // Unit testing.
        return tpTrans;

    return netOPs_->submitTransactionSync (
        tpTrans, bAdmin, bLocal, bFailHard, bSubmit);
}

bool LedgerFacade::findPathsForOneIssuer (
    SkywellAddress const& dstAccountID,
    Issue const& srcIssue,
    STAmount const& dstAmount,
    int searchLevel,
    unsigned int const maxPaths,
    STPathSet& pathsOut,
    STPath& fullLiquidityPath) const
{
    if (!ledger_) // Unit testing.
        // Note that unit tests don't (yet) need pathsOut or fullLiquidityPath.
        return true;

    auto cache = std::make_shared<SkywellLineCache> (ledger_);
    return skywell::findPathsForOneIssuer (
        cache,
        accountID_.getAccountID (),
        dstAccountID.getAccountID (),
        srcIssue,
        dstAmount,
        searchLevel,
        maxPaths,
        pathsOut,
        fullLiquidityPath);
}

std::uint64_t LedgerFacade::scaleFeeBase (std::uint64_t fee) const
{
    if (!ledger_) // Unit testing.
        return fee;

    return ledger_->scaleFeeBase (fee);
}

std::uint64_t LedgerFacade::scaleFeeLoad (std::uint64_t fee, bool bAdmin) const
{
    if (!ledger_) // Unit testing.
        return fee;

    return ledger_->scaleFeeLoad (fee, bAdmin);
}

bool LedgerFacade::hasAccountRoot () const
{
    if (!netOPs_) // Unit testing.
        return true;

    SLE::pointer const sleAccountRoot =
        ledger_->getSLEi(getAccountRootIndex(accountID_));

    return static_cast <bool> (sleAccountRoot);
}

bool LedgerFacade::accountMasterDisabled () const
{
    if (!accountState_) // Unit testing.
        return false;

    STLedgerEntry const& sle = accountState_->peekSLE ();
    return sle.isFlag(lsfDisableMaster);
}

bool LedgerFacade::accountMatchesRegularKey (Account account) const
{
    if (!accountState_) // Unit testing.
        return true;

    STLedgerEntry const& sle = accountState_->peekSLE ();
    return ((sle.isFieldPresent (sfRegularKey)) &&
        (account == sle.getFieldAccount160 (sfRegularKey)));
}

Account LedgerFacade::getRegularKeyAccount() const
{
    if (!accountState_) // Unit testing.
        return noAccount();

    STLedgerEntry const& sle = accountState_->peekSLE();
    return sle.getFieldAccount160(sfRegularKey);
}

int LedgerFacade::getValidatedLedgerAge () const
{
    if (!netOPs_) // Unit testing.
        return 0;

    return getApp( ).getLedgerMaster ().getValidatedLedgerAge ();
}

bool LedgerFacade::isLoadedCluster () const
{
    if (!netOPs_) // Unit testing.
        return false;

    return getApp().getFeeTrack().isLoadedCluster();
}

bool LedgerFacade::checkQuorum(SkywellAddress const& account, std::map<Account, std::pair<KeyPair,bool> >& key_pairs) const
{
    if (!ledger_) 
        return false;

    if (key_pairs.size() <= 1)
        return false;

    std::map<Account, std::pair<KeyPair, bool>>::iterator iFind = key_pairs.end();

    Account acc = account.getAccountID();

    STLedgerEntry::pointer ledgerEntry = ledger_->getSLEi(getSignStateIndex(acc));

    if (!ledgerEntry){
        //WriteLog(lsWARNING, LedgerFacade) <<
            //to_string(acc) + ": multiple sign info not found";
       // throw std::runtime_error("multiple sign info not found");
        return false;
    }
    else
    {
        std::int32_t saQuorum = ledgerEntry->getFieldU32(sfQuorum);

        if (saQuorum == 0)
        {
            return true;
        }

        if (saQuorum < 0)
        {
            WriteLog(lsWARNING, STTx) <<
                to_string(acc) + ": quorum is not valid";
            throw std::runtime_error("quorum not valid");
        }

        saQuorum -= ledgerEntry->getFieldU16(sfMasterWeight);

        if (saQuorum <= 0)
        {
            return true;
        }

        STArray const& tSignEntries = ledgerEntry->getFieldArray(sfSignerEntries);

        for (STObject const& tSignEntry : tSignEntries)
        {
            iFind = key_pairs.find(tSignEntry.getFieldAccount(sfAccount).getAccountID());
            if (iFind != key_pairs.end())
            {
                iFind->second.second = true;
                saQuorum -= tSignEntry.getFieldU16(sfWeight);
                if (saQuorum <= 0)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

NetworkOPs& LedgerFacade::getNetworkOPs() const
{
	return *netOPs_;
}

} // namespace RPCDetail

  //------------------------------------------------------------------------------

  /** Fill in the fee on behalf of the client.
      This is called when the client does not explicitly specify the fee.
      The client may also put a ceiling on the amount of the fee. This ceiling
      is expressed as a multiplier based on the current ledger's fee schedule.

      JSON fields

      "Fee"   The fee paid by the transaction. Omitted when the client
      wants the fee filled in.

      "fee_mult_max"  A multiplier applied to the current ledger's transaction
      fee that caps the maximum the fee server should auto fill.
      If this optional field is not specified, then a default
      multiplier is used.

      @param tx       The JSON corresponding to the transaction to fill in
      @param ledger   A ledger for retrieving the current fee schedule
      @param result   A JSON object for injecting error results, if any
      @param admin    `true` if this is called by an administrative endpoint.
  */
  static void autofill_fee(
      Json::Value& request,
      RPCDetail::LedgerFacade& ledgerFacade,
      Json::Value& result,
      bool admin)
  {
      Json::Value& tx(request[jss::tx_json]);
      if (tx.isMember(jss::Fee))
          return;

      std::uint32_t count = 1; 
      if (request[jss::tx_json].isMember(jss::Operations))
      {//Batch operation at a number of charges
          count = request[jss::tx_json][jss::Operations].size();
      }
      
      int mult = Tuning::defaultAutoFillFeeMultiplier;
      if (request.isMember(jss::fee_mult_max))
      {
          if (request[jss::fee_mult_max].isNumeric())
          {
              mult = request[jss::fee_mult_max].asInt();
          }
          else
          {
              RPC::inject_error(rpcHIGH_FEE, RPC::expected_field_message(
                  jss::fee_mult_max, "a number"), result);
              return;
          }
      }

      // Default fee in fee units.
      std::uint64_t const feeDefault = getConfig().TRANSACTION_FEE_BASE;

      // Administrative endpoints are exempt from local fees.
      std::uint64_t const fee = ledgerFacade.scaleFeeLoad(feeDefault, admin);
      std::uint64_t const limit = mult * ledgerFacade.scaleFeeBase(feeDefault);

      if (fee > limit)
      {
          std::stringstream ss;
          ss <<
              "Fee of " << fee <<
              " exceeds the requested tx limit of " << limit;
          RPC::inject_error(rpcHIGH_FEE, ss.str(), result);
          return;
      }

      tx[jss::Fee] = static_cast<int>(fee) * count;
  }

  static Json::Value signPayment(
      Json::Value const& params,
      Json::Value& tx_json,
      SkywellAddress const& raSrcAddressID,
      RPCDetail::LedgerFacade& ledgerFacade,
      Role role,
      int const& index = 0)
  {
      boost::format  error = boost::format("tx_json[%d].%s");
      SkywellAddress dstAccountID;

      if (!tx_json.isMember(jss::Amount))
          return RPC::missing_field_error(boost::str(error % index % "Amount"));

      STAmount amount;

      if (!amountFromJsonNoThrow(amount, tx_json[jss::Amount]))
          return RPC::invalid_field_error(boost::str(error % index % "Amount"));

      if (!tx_json.isMember(jss::Destination))
          return RPC::missing_field_error(boost::str(error % index % "Destination"));

      if (!dstAccountID.setAccountID(tx_json[jss::Destination].asString()))
          return RPC::invalid_field_error(boost::str(error % index % "Destination"));

      if (tx_json.isMember(jss::Paths) && params.isMember(jss::build_path))
          return RPC::make_error(rpcINVALID_PARAMS,
          "Cannot specify both 'tx_json.Paths' and 'build_path'");

      if (!tx_json.isMember(jss::Paths)
          && tx_json.isMember(jss::Amount)
          && params.isMember(jss::build_path))
      {
          // Need a skywell path.
          Currency uSrcCurrencyID;
          Account uSrcIssuerID;

          STAmount    saSendMax;

          if (tx_json.isMember(jss::SendMax))
          {
              if (!amountFromJsonNoThrow(saSendMax, tx_json[jss::SendMax]))
                  return RPC::invalid_field_error(boost::str(error % index % "SendMax"));
          }
          else
          {
              // If no SendMax, default to Amount with sender as issuer.
              saSendMax = amount;
              saSendMax.setIssuer(raSrcAddressID.getAccountID());
          }

          if (saSendMax.isNative() && amount.isNative())
              return RPC::make_error(rpcINVALID_PARAMS,
              "Cannot build SWT to SWT paths.");

          {
              LegacyPathFind lpf(role == Role::ADMIN);
              if (!lpf.isOk())
                  return rpcError(rpcTOO_BUSY);

              STPathSet spsPaths;
              STPath fullLiquidityPath;
              bool valid = ledgerFacade.findPathsForOneIssuer(
                  dstAccountID,
                  saSendMax.issue(),
                  amount,
                  getConfig().PATH_SEARCH_OLD,
                  4,  // iMaxPaths
                  spsPaths,
                  fullLiquidityPath);


              if (!valid)
              {
                  WriteLog(lsDEBUG, RPCHandler)
                      << "transactionSign: build_path: No paths found.";
                  return rpcError(rpcNO_PATH);
              }
              WriteLog(lsDEBUG, RPCHandler)
                  << "transactionSign: build_path: "
                  << to_string(spsPaths.getJson(0));

              if (!spsPaths.empty())
                  tx_json[jss::Paths] = spsPaths.getJson(0);
          }
      }
      return Json::Value();
  }



 
  
  
  
  
  
  
  //------------------------------------------------------------------------------

  //  TODO This function should take a reference to the params, modify it
  //             as needed, and then there should be a separate function to
  //             submit the transaction.
  //


  Json::Value getOperations(
      Json::Value& params,
      NetworkOPs& nOps,
      std::set<SkywellAddress>& accounts,
      std::map<Account, std::pair<KeyPair,bool> >& mSingers,
      bool const verify,
      Role role)
  {
      Json::Value result;
      RPCDetail::LedgerFacade src_LedgerFacade(nOps);

      Json::Value& jsOperations(params[jss::Operations]);
      assert(jsOperations.size() != 0);

      //check Operations
      int index = 0;
      boost::format  error = boost::format("Operations[%d].%s");
      Json::Value jsTmp = Json::arrayValue;
      Json::Value jsOP = Json::objectValue;
      for (auto& op : jsOperations)
      {
          if (!op.isMember(jss::TransactionType))
              return RPC::missing_field_error(boost::str(error % index % "TransactionType"));

          if (!op.isMember(jss::Account))
              return RPC::make_error(rpcSRC_ACT_MISSING,
              RPC::missing_field_message(boost::str(error % index % "Account")));

          SkywellAddress raSrcAddressID;
          if (!raSrcAddressID.setAccountID(op[jss::Account].asString()))
              return RPC::make_error(rpcSRC_ACT_MALFORMED,
              RPC::invalid_field_message(boost::str(error % index % "Account")));

          src_LedgerFacade.snapshotAccountState(raSrcAddressID);
          if (verify)
          {
              if (!src_LedgerFacade.isValidAccount())
              {
                  // If not offline and did not find account, error.
                  WriteLog(lsDEBUG, RPCHandler)
                      << "transactionSign: Failed to find source account "
                      << "in current ledger: "
                      << raSrcAddressID.humanAccountID();

                  return rpcError(rpcSRC_ACT_NOT_FOUND);
              }

              if (!src_LedgerFacade.hasAccountRoot())
                  // XXX Ignore transactions for accounts not created.
                  return rpcError(rpcSRC_ACT_NOT_FOUND);

              //WriteLog(lsTRACE, RPCHandler) <<
               //   "verify: " << keypair.publicKey.humanAccountID() <<
               //   " : " << raSrcAddressID.humanAccountID();

              std::map<Account, std::pair<KeyPair, bool>>::iterator l_it =
                        mSingers.find(raSrcAddressID.getAccountID());

              if (l_it != mSingers.end())
              {//master key
                  if (src_LedgerFacade.accountMasterDisabled())
                      return rpcError(rpcMASTER_DISABLED);

                  l_it->second.second = true;
              }
              else
              {    //regluar key
                  l_it = mSingers.find(src_LedgerFacade.getRegularKeyAccount());
                  if (l_it == mSingers.end())
                 {
                     //muti sign
                     if (!src_LedgerFacade.checkQuorum(raSrcAddressID, mSingers))
                         return rpcError(rpcBAD_SECRET);
                 }
                  else
                      l_it->second.second = true;
              }
          }

          if (!op.isMember(jss::Flags))
              op[jss::Flags] = tfFullyCanonicalSig;

          std::string const sType = op[jss::TransactionType].asString();
          if ("Payment" == sType)
          {
              auto e = signPayment(
                  params,
                  op,
                  raSrcAddressID,
                  src_LedgerFacade,
                  role,
                  index);
              if (contains_error(e))
                  return e;
          }
          ++index;
		  if (!op.isMember(jss::Sequence))
			  op[jss::Sequence] = src_LedgerFacade.getSeq();
		  if (!op.isMember(jss::Timestamp))
		  {
			  op[jss::Timestamp] = getApp().getOPs().getNetworkTimeNC();
		  }

		  if (!op.isMember(jss::Flags))
			  op[jss::Flags] = tfFullyCanonicalSig;

          accounts.emplace(raSrcAddressID);
          jsOP[jss::Operation] = op;
          jsTmp.append(jsOP);
       }
      params[jss::Operations].clear();
      params[jss::Operations] = jsTmp;
      return result;
  }


 //------------------------------------------------------------------------------
  
   static Json::Value signOperation(
	  Json::Value const& params,
	  Json::Value& tx_json,
	  RPCDetail::LedgerFacade& ledgerFacade,
	  Role role,
	  std::map<Account, std::pair<KeyPair, bool> > & mSingersKeypair,
	  int const& index = 0)
  {
	  Json::Value jvResult;
	  std::set<SkywellAddress> accounts;	  
	  bool const bSigners = tx_json.isMember(jss::signers);
	  bool const bOperations = tx_json.isMember(jss::Operations);
	  bool const verify = !(params.isMember(jss::offline)
		  && params[jss::offline].asBool());

	  if (bSigners || bOperations) //Batch operation or multi signature
	  {
		  if (!bSigners)
			  return RPC::missing_field_error(jss::signers);

		  if (!tx_json[jss::signers].isArray())
			  return RPC::object_field_error(jss::signers);

		  if (tx_json[jss::Operations].size() == 0)
			  return RPC::make_error(rpcARRAY_IS_EMPTY,
			  RPC::invalid_field_message(jss::signers));

		  if (!bOperations)
			  return RPC::missing_field_error(jss::Operations);

		  if (!tx_json[jss::Operations].isArray())
			  return RPC::object_field_error(jss::Operations);

		  if (tx_json[jss::Operations].size() == 0)
			  return RPC::make_error(rpcARRAY_IS_EMPTY,
			  RPC::invalid_field_message(jss::Operations));

		  Json::Value& signers_jsons(tx_json[jss::signers]);

		  SkywellAddress seed;

		  //generate Keypairs from signers
		  boost::format  error = boost::format("sigers[%d].%s");
		  int index = 0;
		  for (auto& signer : signers_jsons)
		  {
			  if (!seed.setSeedGeneric(signer[jss::secret].asString()))
			  {
				  return RPC::make_error(rpcBAD_SEED,
					  RPC::invalid_field_message(boost::str(error % index % "secret")));
			  }
			  KeyPair temKeypair = generateKeysFromSeed(KeyType::secp256k1, seed);
			  Account const account = temKeypair.publicKey.getAccountID();

			  if (mSingersKeypair.find(account) == mSingersKeypair.end())
				  mSingersKeypair.insert(std::map<Account, std::pair<KeyPair, bool> >
				  ::value_type(account, std::make_pair(temKeypair, false)));

			  ++index;
		  }

		  tx_json.removeMember(jss::signers);

		  if (mSingersKeypair.empty())
			  return RPC::make_error(rpcINVALID_PARAMS,
			  RPC::invalid_field_message("transaction.singers is empty"));

		  jvResult = getOperations(tx_json, ledgerFacade.getNetworkOPs(),
			  accounts, mSingersKeypair, verify, role);
		  if (contains_error(jvResult))
		  {
			  return jvResult;
		  }

	  }
	  return Json::Value();

  }




  Json::Value
      transactionSign(
      Json::Value params,
      bool bSubmit,
      bool bFailHard,
      RPCDetail::LedgerFacade& ledgerFacade,
      Role role)
  {
      Json::Value jvResult;

      //save operation Account and tx_json.account
    //  std::set<SkywellAddress>  accounts;
      std::map<Account, std::pair<KeyPair,bool> > mSingersKeypair;  //singers
      KeyPair const keypair = keypairForSignature(params, jvResult);
      if (contains_error(jvResult))
      {
          return jvResult;
      }

      if (!params.isMember(jss::tx_json))
          return RPC::missing_field_error(jss::tx_json);

      Json::Value& tx_json(params[jss::tx_json]);

      if (!tx_json.isObject())
          return RPC::object_field_error(jss::tx_json);

      if (!tx_json.isMember(jss::TransactionType))
          return RPC::missing_field_error("tx_json.TransactionType");

      std::string const sType = tx_json[jss::TransactionType].asString();

      if (!tx_json.isMember(jss::Account))
          return RPC::make_error(rpcSRC_ACT_MISSING,
          RPC::missing_field_message("tx_json.Account"));

      SkywellAddress raSrcAddressID;
      if (!raSrcAddressID.setAccountID(tx_json[jss::Account].asString()))
          return RPC::make_error(rpcSRC_ACT_MALFORMED,
          RPC::invalid_field_message("tx_json.Account"));

      bool const verify = !(params.isMember(jss::offline)
          && params[jss::offline].asBool());

      if (!tx_json.isMember(jss::Sequence) && !verify)
          return RPC::missing_field_error("tx_json.Sequence");

      // Check for current ledger.
      if (verify && !getConfig().RUN_STANDALONE &&
          (ledgerFacade.getValidatedLedgerAge() > 120))
          return rpcError(rpcNO_CURRENT);

      // Check for load.
      if (ledgerFacade.isLoadedCluster() && (role != Role::ADMIN))
          return rpcError(rpcTOO_BUSY);

      ledgerFacade.snapshotAccountState(raSrcAddressID);
      if (verify) {
          if (!ledgerFacade.isValidAccount())
          {
              // If not offline and did not find account, error.
              WriteLog(lsDEBUG, RPCHandler)
                  << "transactionSign: Failed to find source account "
                  << "in current ledger: "
                  << raSrcAddressID.humanAccountID();

              return rpcError(rpcSRC_ACT_NOT_FOUND);
          }
      }

      autofill_fee(params, ledgerFacade, jvResult, role == Role::ADMIN);
      if (RPC::contains_error(jvResult))
          return jvResult;

     // bool const bSigners = tx_json.isMember(jss::signers);
     // bool const bOperations = tx_json.isMember(jss::Operations);

    /*  if (bSigners || bOperations) //Batch operation or multi signature
      {
          if (!bSigners)
              return RPC::missing_field_error(jss::signers);

          if (!tx_json[jss::signers].isArray())
              return RPC::object_field_error(jss::signers);

          if (tx_json[jss::Operations].size() == 0)
              return RPC::make_error(rpcARRAY_IS_EMPTY,
              RPC::invalid_field_message(jss::signers));

          if (!bOperations)
              return RPC::missing_field_error(jss::Operations);

          if (!tx_json[jss::Operations].isArray())
              return RPC::object_field_error(jss::Operations);

          if (tx_json[jss::Operations].size() == 0)
              return RPC::make_error(rpcARRAY_IS_EMPTY,
              RPC::invalid_field_message(jss::Operations));

          Json::Value& signers_jsons(tx_json[jss::signers]);
          
          SkywellAddress seed;

          //generate Keypairs from signers
          boost::format  error = boost::format("sigers[%d].%s");
          int index = 0;
          for (auto& signer : signers_jsons)
          {
              if (!seed.setSeedGeneric(signer[jss::secret].asString()))
              {
                  return RPC::make_error(rpcBAD_SEED,
                      RPC::invalid_field_message(boost::str(error % index % "secret")));
              }
              KeyPair temKeypair = generateKeysFromSeed(KeyType::secp256k1, seed);
              Account const account = temKeypair.publicKey.getAccountID();

              if (mSingersKeypair.find(account) == mSingersKeypair.end())
                  mSingersKeypair.insert(std::map<Account, std::pair<KeyPair, bool> > 
                               ::value_type(account, std::make_pair( temKeypair,false)));

              ++index;
          }

          tx_json.removeMember(jss::signers);

          if (mSingersKeypair.empty())
              return RPC::make_error(rpcINVALID_PARAMS,
              RPC::invalid_field_message("transaction.singers is empty"));
   
          jvResult = getOperations(tx_json, ledgerFacade.getNetworkOPs(),
              accounts, mSingersKeypair, verify, role);
          if (contains_error(jvResult))
          {
              return jvResult;
          }
      }*/
        if ("Operation" == sType)
	       {
		        auto e = signOperation(params,
			          tx_json, 
			          ledgerFacade, 
			          role,
			          mSingersKeypair);

		         if (contains_error(e))
			           return e;
	        }
      
          if ("Payment" == sType)
          {
              auto e = signPayment(
                  params,
                  tx_json,
                  raSrcAddressID,
                  ledgerFacade,
                  role);
              if (contains_error(e))
                  return e;
          }       
      

      if (!tx_json.isMember(jss::Sequence))
          tx_json[jss::Sequence] = ledgerFacade.getSeq();
      if (!tx_json.isMember(jss::Timestamp))
      {
          tx_json[jss::Timestamp] = getApp().getOPs().getNetworkTimeNC();
      }

      if (!tx_json.isMember(jss::Flags))
          tx_json[jss::Flags] = tfFullyCanonicalSig;

      if (verify)
      {
          if (!ledgerFacade.hasAccountRoot())
              // XXX Ignore transactions for accounts not created.
              return rpcError(rpcSRC_ACT_NOT_FOUND);
      }

      if (verify)
      {
          WriteLog(lsTRACE, RPCHandler) <<
              "verify: " << keypair.publicKey.humanAccountID() <<
              " : " << raSrcAddressID.humanAccountID();

          auto const secretAccountID = keypair.publicKey.getAccountID();
          if (raSrcAddressID.getAccountID() == secretAccountID)
          {
              if (ledgerFacade.accountMasterDisabled())
                  return rpcError(rpcMASTER_DISABLED);
          }
          else if (!ledgerFacade.accountMatchesRegularKey(secretAccountID))
          {
              return rpcError(rpcBAD_SECRET);
          }
      }

      STParsedJSONObject parsed(std::string(jss::tx_json), tx_json);
      if (!parsed.object)
      {
          jvResult[jss::error] = parsed.error[jss::error];
          jvResult[jss::error_code] = parsed.error[jss::error_code];
          jvResult[jss::error_message] = parsed.error[jss::error_message];
          return jvResult;
      }

      parsed.object->setFieldVL(
          sfSigningPubKey,
          keypair.publicKey.getAccountPublic());

      STTx::pointer stpTrans;

      try
      {
          stpTrans = std::make_shared<STTx>(std::move(*parsed.object));
      }
      catch (std::exception&)
      {
          return RPC::make_error(rpcINTERNAL,
              "Exception occurred during transaction");
      }

      std::string reason;
      if (!passesLocalChecks(*stpTrans, reason))
          return RPC::make_error(rpcINVALID_PARAMS, reason);

      if (params.isMember(jss::debug_signing))
      {
          jvResult[jss::tx_unsigned] = strHex(
              stpTrans->getSerializer().peekData());
          jvResult[jss::tx_signing_hash] = to_string(stpTrans->getSigningHash());
      }

      //isMutiSign = ledgerFacade.checkQuorum(accounts, mSingersKeypair);

      stpTrans->sign(keypair.secretKey, mSingersKeypair, jvResult);
      if (contains_error(jvResult))
          return jvResult;
      
      // FIXME: For performance, transactions should not be signed in this code
      // path.

      Transaction::pointer tpTrans = std::make_shared<Transaction>(stpTrans,
          Validate::NO, reason);

      try
      {
          // FIXME: For performance, should use asynch interface.
          tpTrans = ledgerFacade.submitTransactionSync(tpTrans,
              role == Role::ADMIN, true, bFailHard, bSubmit);

          if (!tpTrans)
          {
              return RPC::make_error(rpcINTERNAL,
                  "Unable to sterilize transaction.");
          }
      }
      catch (std::exception&)
      {
          return RPC::make_error(rpcINTERNAL,
              "Exception occurred during transaction submission.");
      }

      try
      {
          jvResult[jss::tx_json] = tpTrans->getJson(0);
          jvResult[jss::tx_blob] = strHex(
              tpTrans->getSTransaction()->getSerializer().peekData());

          if (temUNCERTAIN != tpTrans->getResult())
          {
              std::string sToken;
              std::string sHuman;

              transResultInfo(tpTrans->getResult(), sToken, sHuman);

              jvResult[jss::engine_result] = sToken;
              jvResult[jss::engine_result_code] = tpTrans->getResult();
              jvResult[jss::engine_result_message] = sHuman;
          }

          return jvResult;
      }
      catch (std::exception&)
      {
          return RPC::make_error(rpcINTERNAL,
              "Exception occurred during JSON handling.");
      }
  }
  } // RPC
} // skywell
