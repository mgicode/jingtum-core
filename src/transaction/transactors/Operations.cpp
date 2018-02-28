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
#include <transaction/transactors/Operations.h>
#include <common/core/Config.h>
#include <transaction/transactors/Transactor.h>
// #include <skywell/legacy/0.27/Emulate027.h>
#include <protocol/Indexes.h>
#include <main/Application.h>
#include <ledger/LedgerMaster.h>

namespace skywell {

    Operations::Operations(
        STTx const& txn,
        TransactionEngineParams params,
		TransactionEngine* engine) :Transactor(txn,params,engine)
    {

    }

	TER Operations::precheckSig(STTx txn, TransactionEngineParams& params)
	{
		
		std::set<Account>::const_iterator it = mSigners.end();
		Account mTxnAccountID = txn.getSourceAccount().getAccountID();
		it = mSigners.find(mTxnAccountID);
		auto mTxnAccount = mEngine->view().entryCache(ltACCOUNT_ROOT,
			getAccountRootIndex(mTxnAccountID));
		
		if (it != mSigners.end())
		{
			if (mTxnAccount->isFlag(lsfDisableMaster))
				return tefMASTER_DISABLED;

			params = static_cast<TransactionEngineParams> (params | tapMASTER_SIGN);
			return tesSUCCESS;

		} 
		if (mTxnAccount->isFieldPresent(sfRegularKey))
		{
			if (mTxnAccountID == mTxnAccount->getFieldAccount160(sfRegularKey))
				return tesSUCCESS;
		}
		if (mSigners.size() >= 1)
		{	//muti sign
			SLE::pointer sle(mEngine->view().entryCache(ltSIGNER_LIST,
				getSignStateIndex(mTxnAccountID)));

			if (sle)
			{
				std::int32_t saQuorum = sle->getFieldU32(sfQuorum);

				if (saQuorum == 0)
				{
					return tesSUCCESS;
				}

				if (saQuorum < 0)
				{
					return tefBAD_AUTH;
				}

				saQuorum -= sle->getFieldU16(sfMasterWeight);

				if (saQuorum <= 0)
				{
					return tesSUCCESS;
				}

				STArray const& tSignEntries = sle->getFieldArray(sfSignerEntries);

				for (STObject const& tSignEntry : tSignEntries)
				{
					it = mSigners.find(tSignEntry.getFieldAccount(sfAccount).getAccountID());
					if (it != mSigners.end())
					{
						saQuorum -= tSignEntry.getFieldU16(sfWeight);
						if (saQuorum <= 0)
						{
							return tesSUCCESS;
						}
					}
				}
			}
		}
		m_journal.trace << "Delay: Not authorized to use account.";
		return tefBAD_AUTH;
	}

    TER Operations::doApply()
    {
       // bool const mBatchOperation = mTxn.getTxnType() == ttOPERATION ? true : false;
        TER terResult(tesSUCCESS);

        try
        {
            mTxn.getSignAccount(mSigners);
            mTxn.getSTTxs(mSTTxs);
            if (mSTTxs.size() == 0)
            {
                m_journal.warning << "transaction is empty";
                return temINVALID;
            }
        }
        catch (...)
        {
            m_journal.warning << "transaction is empty";
            return temINVALID;
        } 
		

        for (auto const& sttx : mSTTxs)
        {
			terResult = precheckSig(sttx, mParams);
			if (terResult != tesSUCCESS) return terResult;

			mParams = static_cast<TransactionEngineParams> (mParams | tapPRE_CHECKED);
            terResult = Transactor::transact(sttx, mParams, mEngine);

            if (terResult == temUNKNOWN)
            {
                m_journal.warning <<
					 "applyTransaction: Invalid transaction: unknown transaction type";
                break;
            }
            if (!isTesSuccess(terResult))
                break;
        }
        return terResult;
    }

	TER transact_Operations(STTx const& txn, TransactionEngineParams params, TransactionEngine* engine)
	{
	 	return	Operations (txn, params, engine).apply();
	}
}//skywell
