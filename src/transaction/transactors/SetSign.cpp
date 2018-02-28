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
#include <transaction/book/Quality.h>
#include <transaction/transactors/Transactor.h>
#include <common/base/Log.h>
#include <protocol/Indexes.h>
#include <protocol/TxFlags.h>

namespace skywell {

class SetSign
	: public Transactor
{
public:
	SetSign(
		STTx const& txn,
		TransactionEngineParams params,
		TransactionEngine* engine)
		: Transactor(
		txn,
		params,
		engine,
		deprecatedLogs().journal("SetSign"))
	{
	}

	TER preCheck() override
	{
		std::uint32_t const uTxFlags = mTxn.getFlags();

		if (uTxFlags & tfTrustSetMask)
		{
			m_journal.trace <<
				"Malformed transaction: Invalid flags set.";
			return temINVALID_FLAG;
		}

		if (mTxn.isFieldPresent(sfMasterWeight)){
            std::uint16_t const uMasterWeight(mTxn.getFieldU16(sfMasterWeight));

            if (uMasterWeight < 0)
			{
				if (m_journal.trace) m_journal.trace <<
					"Malformed transaction: Negative weight.";
				return temBAD_WEIGHT;
			}
		}

		if (mTxn.isFieldPresent(sfQuorum)){
            std::uint32_t const uQuorum(mTxn.getFieldU32(sfQuorum));

            if (uQuorum < 0)
			{
                if (m_journal.trace) m_journal.trace <<
					"Malformed transaction: Negative quorum.";
				return temBAD_QUORUM;
			}
		}

        if (mTxn.isFieldPresent(sfQuorumHigh)){
            std::uint32_t const uQuorumHigh(mTxn.getFieldU32(sfQuorumHigh));

            if (uQuorumHigh < 0)
            {
                if (m_journal.trace) m_journal.trace <<
                    "Malformed transaction: Negative quorum.";
                return temBAD_QUORUM;
            }
        }

		auto tSigners = mTxn.getFieldArray(sfSignerEntries);

		for (auto & ts : tSigners){
			Account const uDstAccountID(ts.getFieldAccount160(sfAccount));

			if (!uDstAccountID || uDstAccountID == noAccount())
			{
				if (m_journal.trace) m_journal.trace <<
					"Malformed transaction: no destination account.";
				return temDST_NEEDED;
			}

			if (mTxnAccountID == uDstAccountID)
			{
				m_journal.trace << "Malformed transaction: " <<
					"Redundant sign from " << to_string(mTxnAccountID) <<
					" to self";
				return temREDUNDANTSIGN;
			}

			if (ts.isFieldPresent(sfWeight)){

                std::uint16_t const uWeight(ts.getFieldU16(sfWeight));

                if (uWeight < 0)
				{
					if (m_journal.trace) m_journal.trace <<
						"Malformed transaction: Negative weight.";
					return temBAD_WEIGHT;
				}
			}
		}

		return Transactor::preCheck();
	}

	TER doApply() override
	{
		TER terResult = tesSUCCESS;

		mTxnAccountID = mTxn.getSourceAccount().getAccountID();
		if (mTxnAccountID.isZero())
		{
			m_journal.warning << "apply: bad transaction source id";
			return temBAD_SRC_ACCOUNT;
		}

		// Find source account
		// When the AccountMerge delete source account, entryCache returns to nullptr
		mTxnAccount = mEngine->view().entryCache(ltACCOUNT_ROOT,
			getAccountRootIndex(mTxnAccountID));

		std::uint16_t const saMasterWeight = mTxn.getFieldU16(sfMasterWeight);

		std::uint32_t const saQuorum = mTxn.getFieldU32(sfQuorum);
		std::uint32_t const saQuorumHigh = mTxn.getFieldU32(sfQuorumHigh);

		auto const tSignEntries = mTxn.getFieldArray(sfSignerEntries);

		SLE::pointer sleSkywellState(mEngine->view().entryCache(ltSIGNER_LIST,
			getSignStateIndex(mTxnAccountID)));

		if (!sleSkywellState && mTxn.getTxnType() == ttSIGN_SET)
		{
			uint256 index(getSignStateIndex(mTxnAccountID));

			m_journal.trace <<
				"doSignSet: Creating rep line: " <<
				to_string(index);

           // sleSkywellState = mEngine->view().entryCreate(ltSIGNER_LIST,
            //    getSignStateIndex(mTxnAccountID));

			terResult = mEngine->view().signCreate(
				index,
				mTxnAccount,
				mTxnAccountID,
				saMasterWeight,
				saQuorum,
				saQuorumHigh,
				tSignEntries
                
			);
			
		}
		else if (sleSkywellState && mTxn.getTxnType() == ttSIGN_SET)
		{
			if (saMasterWeight)
				sleSkywellState->setFieldU16(
				sfMasterWeight, saMasterWeight);

			if (saQuorum)
				sleSkywellState->setFieldU32(
				sfQuorum, saQuorum);

			if (saQuorumHigh)
				sleSkywellState->setFieldU32(
				sfQuorumHigh, saQuorumHigh);

			if (!tSignEntries.empty())
				sleSkywellState->setFieldArray(
				sfSignerEntries, tSignEntries);

			mEngine->view().entryModify(sleSkywellState);

			m_journal.trace << "Modify sign line";
		}
		return terResult;
	}
};

TER
transact_SetSign(
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
	return SetSign(txn, params, engine).apply();
}
}
