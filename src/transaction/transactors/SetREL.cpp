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

class SetREL
    : public Transactor
{
public:
    SetREL (
        STTx const& txn,
        TransactionEngineParams params,
        TransactionEngine* engine)
        : Transactor (
            txn,
            params,
            engine,
            deprecatedLogs().journal("SetREL"))
    {
    }

    TER preCheck () override
    {
        std::uint32_t const uTxFlags = mTxn.getFlags ();

        if (uTxFlags & tfRelationSetMask)
        {
            m_journal.trace <<
                "Malformed transaction: Invalid flags set.";
            return temINVALID_FLAG;
        }

        STAmount const saLimitAmount (mTxn.getFieldAmount (sfLimitAmount));

        if (!isLegalNet (saLimitAmount))
            return temBAD_AMOUNT;

        if (saLimitAmount.isNative ())
        {
            if (m_journal.trace) m_journal.trace <<
                "Malformed transaction: specifies native limit " <<
                saLimitAmount.getFullText ();
            return temBAD_LIMIT;
        }

        if (badCurrency() == saLimitAmount.getCurrency ())
        {
            if (m_journal.trace) m_journal.trace <<
                "Malformed transaction: specifies SWT as IOU";
            return temBAD_CURRENCY;
        }

        if (saLimitAmount < zero)
        {
            if (m_journal.trace) m_journal.trace <<
                "Malformed transaction: Negative credit limit.";
            return temBAD_LIMIT;
        }

        // Check if destination makes sense.
        auto const& issuer = saLimitAmount.getIssuer ();

        if (!issuer || issuer == noAccount())
        {
            if (m_journal.trace) m_journal.trace <<
                "Malformed transaction: no destination account.";
            return temDST_NEEDED;
        }

        return Transactor::preCheck ();
    }

    TER doApply () override
    {
        TER terResult = tesSUCCESS;

        std::uint32_t const uRelationType = mTxn.getFieldU32 (sfRelationType);
		
        STAmount const saLimitAmount (mTxn.getFieldAmount (sfLimitAmount));
        bool const bQualityIn (mTxn.isFieldPresent (sfQualityIn));
        bool const bQualityOut (mTxn.isFieldPresent (sfQualityOut));

        Currency const currency (saLimitAmount.getCurrency ());
        Account uIssuerID (saLimitAmount.getIssuer ());
        Account uDstAccountID (mTxn.getFieldAccount160 (sfTarget));

        // true, iff current is high account.
        bool const bHigh = mTxnAccountID > uDstAccountID;

        std::uint32_t uQualityIn (bQualityIn ? mTxn.getFieldU32 (sfQualityIn) : 0);
        std::uint32_t uQualityOut (bQualityOut ? mTxn.getFieldU32 (sfQualityOut) : 0);

        if (bQualityOut && QUALITY_ONE == uQualityOut)
            uQualityOut = 0;

        SLE::pointer sleDst (mEngine->view().entryCache (
            ltACCOUNT_ROOT, getAccountRootIndex (uDstAccountID)));

        if (!sleDst)
        {
            m_journal.trace <<
                "Delay transaction: Destination account does not exist.";
            return tecNO_DST;
        }

        STAmount saLimitAllow = saLimitAmount;
        saLimitAllow.setIssuer (mTxnAccountID);

        SLE::pointer sleSkywellState (mEngine->view().entryCache (ltTrust_STATE,
            getTrustStateIndex (mTxnAccountID, uDstAccountID,uRelationType,uIssuerID, currency)));
		
    	  if (mTxn.getTxnType () == ttREL_DEL && sleSkywellState)
    	  {
                return mEngine->view ().trustDelete (
                    sleSkywellState, mTxnAccountID, uDstAccountID);
    	  }
		  
        if (!sleSkywellState && mTxn.getTxnType () == ttREL_SET)
        {
            // Zero balance in currency.
//            STAmount saBalance ({currency, noAccount()});
            STAmount saBalance ({currency, uIssuerID});

            uint256 index (getTrustStateIndex (
                mTxnAccountID, uDstAccountID,uRelationType,uIssuerID, currency));

            m_journal.trace <<
                "doRelationSet: Creating rep line: " <<
                to_string (index);

            // Create a new skywell line.
            terResult = mEngine->view ().relationCreate (
                              bHigh,
                              mTxnAccountID,
                              uDstAccountID,
                              index,
                              mTxnAccount,
                              saBalance,
                              saLimitAllow,       // Limit for who is being charged.
                              uQualityIn,
                              uQualityOut);
		if(terResult == tesSUCCESS)
		{
			SLE::pointer sleSkywellState (mEngine->view().entryCache (ltTrust_STATE,
		            getTrustStateIndex (mTxnAccountID, uDstAccountID,uRelationType,uIssuerID, currency)));
	             sleSkywellState->setFieldU32 (sfRelationType, uRelationType);
	             mEngine->view().entryModify (sleSkywellState);			
		}
        }
	  else if(sleSkywellState && mTxn.getTxnType () == ttREL_SET)
	{
            //
            // Limits
            //

            sleSkywellState->setFieldAmount (!bHigh ? sfLowLimit : sfHighLimit, saLimitAllow);


            mEngine->view().entryModify (sleSkywellState);

            m_journal.trace << "Modify trust line";
        }
        return terResult;
    }

};

TER
transact_SetREL (
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    return SetREL (txn, params, engine).apply ();
}
}
