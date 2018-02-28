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
#include <common/core/Config.h>


namespace skywell {

class ManageTransactor
    : public Transactor
{

public:
    ManageTransactor (
        STTx const& txn,
        TransactionEngineParams params,
        TransactionEngine* engine)
        : Transactor (
            txn,
            params,
            engine,
            deprecatedLogs().journal("ManageTransactor"))
    {

    }
	TER doApply () override
	{

	    if (mTxn.getTxnType () == ttMNGFEE)
	        return applyManageFee ();
	    if (mTxn.getTxnType () == ttMNGISSUER)
	        return applyManageIssuer ();


	    if (mTxn.getTxnType () == ttBLKLISTSET)
	        return applyManageBlackList (true);
		
	    if (mTxn.getTxnType () == ttBLKLISTRMV)
	        return applyManageBlackList (false);

	    return temUNKNOWN;
	}

	TER checkSig () override
	{
	    return tesSUCCESS;
	}

	TER checkSeq () override
	{

		uint256 txID = mTxn.getTransactionID ();

		if (mEngine->getLedger ()->hasTransaction (txID))
		    return tefALREADY;

	    return tesSUCCESS;
	}
	 
	TER payFee () override
	{
	    return tesSUCCESS;
	}

	TER preCheck () override
	{
	    Account                         mMngAccountID;
	    mTxnAccountID   = mTxn.getSourceAccount ().getAccountID ();
	    mMngAccountID   = getConfig ().SMNG_ACCOUNTID.getAccountID ();
	
	    if (mTxn.getTxnType () == ttMNGISSUER)
	    {
		    if (mTxnAccountID != getConfig ().SISSUER_ACCOUNTID.getAccountID ())
		    {
		        m_journal.warning << "applyTransaction: issuer bad source Manage id: "<< to_string (mTxnAccountID);

		        return temBAD_SRC_ACCOUNT;
		    }
	    }
	    else if (mTxnAccountID != mMngAccountID)
	    {
	        m_journal.warning << "applyTransaction: bad source Manage id: "<< to_string (mTxnAccountID);

	        return temBAD_SRC_ACCOUNT;
	    }

	    return tesSUCCESS;
	}

	TER applyManageFee ()
	{

	    SLE::pointer feeObject = mEngine->view ().entryCache (ltMANAGE_FEE, Ledger::getLedgerManageFeeIndex ());

	    if (!feeObject)
	        feeObject = mEngine->view ().entryCreate (ltMANAGE_FEE, Ledger::getLedgerManageFeeIndex ());
		

	    feeObject->setFieldAccount (sfFeeAccountID,  mTxn.getFieldAccount160 (sfFeeAccountID));

	    mEngine->view ().entryModify (feeObject);

//	    WriteLog (lsINFO, ManageTransactor) << "New Mngfee object: " << feeObject->getJson (0);
	    m_journal.warning << "Fees info have been changed to "  <<to_string( mTxn.getFieldAccount160 (sfFeeAccountID));
	    return tesSUCCESS;
	}

	TER applyManageIssuer ()
	{

	    SLE::pointer issuerObject = mEngine->view ().entryCache (ltMANAGE_ISSUER, Ledger::getLedgerManageIssuerIndex ());

	    if (!issuerObject)
	        issuerObject = mEngine->view ().entryCreate (ltMANAGE_ISSUER, Ledger::getLedgerManageIssuerIndex ());
		

	    issuerObject->setFieldAccount (sfIssuerAccountID,  mTxn.getFieldAccount160 (sfIssuerAccountID));

	    mEngine->view ().entryModify (issuerObject);

	    m_journal.warning << "Issuer info have been changed to "  <<to_string( mTxn.getFieldAccount160 (sfIssuerAccountID));
	    return tesSUCCESS;
	}

	TER applyManageBlackList (bool isSet)
	{

	    SLE::pointer blistObject = mEngine->view ().entryCache (ltBLACKLIST, Ledger::getBlackListIndex (mTxn.getFieldAccount160 (sfBlackListAccountID)));

		if (!blistObject && isSet) //set blacklist
		{
			blistObject = mEngine->view ().entryCreate (ltBLACKLIST, Ledger::getBlackListIndex (mTxn.getFieldAccount160 (sfBlackListAccountID)));
			blistObject->setFieldAccount (sfBlackListAccountID,  mTxn.getFieldAccount160 (sfBlackListAccountID));
			m_journal.warning << "add blacklist info : "  <<to_string( mTxn.getFieldAccount160 (sfBlackListAccountID));
		}	
		else if(blistObject &&!isSet) //remove blacklist
		{
			m_journal.warning << "delete blacklist info : "  <<to_string( mTxn.getFieldAccount160 (sfBlackListAccountID));
			mEngine->view ().entryDelete(blistObject);
		       return tesSUCCESS;
		}
		else
		{
			return temINVALID;
		}


	    mEngine->view ().entryModify (blistObject);
		
	    return tesSUCCESS;
	}
	
};

TER
transact_Manage (
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    return ManageTransactor(txn, params, engine).apply ();
}

}
