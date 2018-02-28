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
#include <transaction/transactors/Transactor.h>
#include <common/base/Log.h>
#include <main/Application.h>
#include <ledger/LedgerMaster.h>
#include <protocol/Indexes.h>
namespace skywell {

// See https://skywell.com/wiki/Transaction_Format#Message

class Message 
    : public Transactor
{

public:
    Message(STTx const& txn, TransactionEngineParams params, TransactionEngine* engine)
        : Transactor(txn, params, engine, deprecatedLogs().journal("Message"))
    {
    }

    TER preCheck () override
    {
        mTxnAccountID = mTxn.getSourceAccount().getAccountID();

        Account const uDstAccountID (mTxn.getFieldAccount160 (sfDestination));

        if (!uDstAccountID)
        {
            m_journal.trace << "Malformed transaction: " <<
                "Message destination account not specified.";
            return temDST_NEEDED;
        }
        if (mTxnAccountID == uDstAccountID)
        {
            m_journal.trace << "Malformed transaction: " <<
                "Redundant message from " << to_string (mTxnAccountID) << " to self";
            return temREDUNDANT;
        }
	    if(getApp().getLedgerMaster ().getClosedLedger()->checkBlackList(uDstAccountID))
        {
            m_journal.warning << "BlackList transaction: " <<
                "Message from " << to_string (mTxnAccountID) <<
                " to blacklist account  " << to_string (uDstAccountID);
	          return telBLKLIST;
        }
	  
        return Transactor::preCheck ();
    }

    TER doApply () override
    {
        Account const uDstAccountID (mTxn.getFieldAccount160 (sfDestination));

        auto const index = getAccountRootIndex (uDstAccountID);
        SLE::pointer sleDst (mEngine->view().entryCache (ltACCOUNT_ROOT, index));

        if (!sleDst)
        {
            m_journal.trace << "Delay transaction: Destination account does not exist.";
            return tecNO_DST;

        }
        return tesSUCCESS;
    }
};

TER
transact_Message (
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    return Message(txn, params, engine).apply();
}

}  // skywell

