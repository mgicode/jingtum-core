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
#include <transaction/paths/SkywellCalc.h>
#include <transaction/transactors/Transactor.h>
#include <common/base/Log.h>
#include <protocol/TxFlags.h>
#include <main/Application.h>
#include <ledger/LedgerMaster.h>

namespace skywell {

class AccountMerge
    : public Transactor
{
public:
    AccountMerge (
		STTx const& txn,
        TransactionEngineParams params,
        TransactionEngine* engine)
        : Transactor (
            txn,
            params,
            engine,
            deprecatedLogs().journal("AccountMerge"))
    {
    }

    TER preCheck () override
    {
       std::uint32_t const uTxFlags = mTxn.getFlags ();
       if (uTxFlags & tfAccountMergeMask)
       {
           m_journal.trace <<
                "Malformed transaction: Invalid flags set.";
           return temINVALID_FLAG;
       }
   
       Account const uDstAccountID (mTxn.getFieldAccount160 (sfDestination));
       if (!uDstAccountID)
       {
          m_journal.trace << "Malformed transaction: " <<
                "AccountMerge destination account not specified.";
          return temDST_NEEDED;
       }

       if(getApp().getLedgerMaster ().getClosedLedger()->checkBlackList(uDstAccountID))
       {
          m_journal.warning << "BlackList transaction: " <<
                "AccountMerge from " << to_string (mTxnAccountID) <<
                " to blacklist account  " << to_string (uDstAccountID);
	      return telBLKLIST;
       }

       if ( mTxnAccountID == mFeeAccountID 
           || mTxnAccountID == getConfig().SISSUER_ACCOUNTID.getAccountID()
           || mTxnAccountID == getConfig().SMNG_ACCOUNTID.getAccountID())
       {
           m_journal.warning << "applyTransaction: bad source id: " << to_string(mTxnAccountID)
                      <<"  Source account can not be a fee account or manage account";

           return temBAD_SRC_ACCOUNT;
       }

       if (mTxnAccountID == uDstAccountID )  
       {
          m_journal.trace << "Malformed transaction: " <<
	     "Unable to merge from " << to_string (mTxnAccountID) <<
	      " to " << to_string (uDstAccountID);
          return temDST_IS_SRC;
       }

       std::uint32_t const current_count(mTxnAccount->getFieldU32(sfOwnerCount));
       if (current_count != 0)
       {
           m_journal.warning <<"Malformed transaction: "
               << to_string(mTxnAccountID) << " ownerCount is not zero";
           return terOWNERS;
       }

       return Transactor::preCheck ();
    }


    TER doApply () override
    {
       TER terResult = tesSUCCESS;
       Account const uDstAccountID (mTxn.getFieldAccount160 (sfDestination));
       SLE::pointer sleDst (mEngine->view().entryCache (ltACCOUNT_ROOT, getAccountRootIndex (uDstAccountID)));
       if(!sleDst)
       {
          m_journal.trace <<
                      "Delay transaction: Destination account does not exist.";
          return tecNO_DST;
       }

       STAmount saAmount = mTxnAccount->getFieldAmount (sfBalance);
       if (saAmount != saAmount.zeroed())
       { //send swt
         terResult = mEngine->view ().accountSend(mTxnAccountID,uDstAccountID,saAmount);
         if (terResult != tesSUCCESS)
         {
            m_journal.warning<<"send swt error";   
            return terResult;
         }
       }

       mEngine->view ().entryDelete(mTxnAccount); 
       return terResult;
    }

    TER payFee () override
    { 
      return tesSUCCESS;
    }
};

TER
transact_AccountMerge (
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    return AccountMerge (txn, params, engine).apply ();
}
}
