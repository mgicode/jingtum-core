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

class SetTrust
    : public Transactor
{
public:
    SetTrust (
        STTx const& txn,
        TransactionEngineParams params,
        TransactionEngine* engine)
        : Transactor (
            txn,
            params,
            engine,
            deprecatedLogs().journal("SetTrust"))
    {
    }

    TER preCheck () override
    {
        std::uint32_t const uTxFlags = mTxn.getFlags ();

        if (uTxFlags & tfTrustSetMask)
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

        STAmount const saLimitAmount (mTxn.getFieldAmount (sfLimitAmount));
        bool const bQualityIn (mTxn.isFieldPresent (sfQualityIn));
        bool const bQualityOut (mTxn.isFieldPresent (sfQualityOut));

        Currency const currency (saLimitAmount.getCurrency ());
        Account uDstAccountID (saLimitAmount.getIssuer ());

        // true, iff current is high account.
        bool const bHigh = mTxnAccountID > uDstAccountID;

        std::uint32_t const uOwnerCount (mTxnAccount->getFieldU32 (sfOwnerCount));

        // The reserve required to create the line. Note that we allow up to
        // two trust lines without requiring a reserve because being able to
        // exchange currencies is a powerful Skywell feature.
        //
        // This is also a security feature: if you're a gateway and you want to
        // be able to let someone use your services, you would otherwise have to
        // give them enough SWT to cover the incremental reserve for their trust
        // line. If they had no intention of using your services, they could use
        // the SWT for their own purposes. So we make it possible for gateways
        // to fund accounts in a way where there's no incentive to trick them
        // into creating an account you have no intention of using.

        std::uint64_t const uReserveCreate = (uOwnerCount < 2)
            ? 0
            : mEngine->getLedger ()->getReserve (uOwnerCount + 1);

        std::uint32_t uQualityIn (bQualityIn ? mTxn.getFieldU32 (sfQualityIn) : 0);
        std::uint32_t uQualityOut (bQualityOut ? mTxn.getFieldU32 (sfQualityOut) : 0);

        if (bQualityOut && QUALITY_ONE == uQualityOut)
            uQualityOut = 0;

        std::uint32_t const uTxFlags = mTxn.getFlags ();

        bool const bSetAuth = (uTxFlags & tfSetfAuth);
        bool const bSetNoSkywell = (uTxFlags & tfSetNoSkywell);
        bool const bClearNoSkywell  = (uTxFlags & tfClearNoSkywell);
        bool const bSetFreeze = (uTxFlags & tfSetFreeze);
        bool const bClearFreeze = (uTxFlags & tfClearFreeze);

        if (bSetAuth && !(mTxnAccount->getFieldU32 (sfFlags) & lsfRequireAuth))
        {
            m_journal.trace <<
                "Retry: Auth not required.";
            return tefNO_AUTH_REQUIRED;
        }

        if (mTxnAccountID == uDstAccountID)
        {
            // The only purpose here is to allow a mistakenly created
            // trust line to oneself to be deleted. If no such trust
            // lines exist now, why not remove this code and simply
            // return an error?
            SLE::pointer selDelete (
                mEngine->view().entryCache (ltSKYWELL_STATE,
                    getSkywellStateIndex (
                        mTxnAccountID, uDstAccountID, currency)));

            if (selDelete)
            {
                m_journal.warning <<
                    "Clearing redundant line.";

                return mEngine->view ().trustDelete (
                    selDelete, mTxnAccountID, uDstAccountID);
            }
            else
            {
                m_journal.trace <<
                    "Malformed transaction: Can not extend credit to self.";
                return temDST_IS_SRC;
            }
        }

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

        SLE::pointer sleSkywellState (mEngine->view().entryCache (ltSKYWELL_STATE,
            getSkywellStateIndex (mTxnAccountID, uDstAccountID, currency)));
		
		  
        if (sleSkywellState)
        {
            STAmount        saLowBalance;
            STAmount        saLowLimit;
            STAmount        saHighBalance;
            STAmount        saHighLimit;
            std::uint32_t   uLowQualityIn;
            std::uint32_t   uLowQualityOut;
            std::uint32_t   uHighQualityIn;
            std::uint32_t   uHighQualityOut;
            auto const& uLowAccountID   = !bHigh ? mTxnAccountID : uDstAccountID;
            auto const& uHighAccountID  =  bHigh ? mTxnAccountID : uDstAccountID;
            SLE::ref        sleLowAccount   = !bHigh ? mTxnAccount : sleDst;
            SLE::ref        sleHighAccount  =  bHigh ? mTxnAccount : sleDst;

            //
            // Balances
            //

            saLowBalance    = sleSkywellState->getFieldAmount (sfBalance);
            saHighBalance   = -saLowBalance;

            //
            // Limits
            //

            sleSkywellState->setFieldAmount (!bHigh ? sfLowLimit : sfHighLimit, saLimitAllow);

            saLowLimit  = !bHigh ? saLimitAllow : sleSkywellState->getFieldAmount (sfLowLimit);
            saHighLimit =  bHigh ? saLimitAllow : sleSkywellState->getFieldAmount (sfHighLimit);

            //
            // Quality in
            //

            if (!bQualityIn)
            {
                // Not setting. Just get it.

                uLowQualityIn   = sleSkywellState->getFieldU32 (sfLowQualityIn);
                uHighQualityIn  = sleSkywellState->getFieldU32 (sfHighQualityIn);
            }
            else if (uQualityIn)
            {
                // Setting.

                sleSkywellState->setFieldU32 (!bHigh ? sfLowQualityIn : sfHighQualityIn, uQualityIn);

                uLowQualityIn   = !bHigh ? uQualityIn : sleSkywellState->getFieldU32 (sfLowQualityIn);
                uHighQualityIn  =  bHigh ? uQualityIn : sleSkywellState->getFieldU32 (sfHighQualityIn);
            }
            else
            {
                // Clearing.

                sleSkywellState->makeFieldAbsent (!bHigh ? sfLowQualityIn : sfHighQualityIn);

                uLowQualityIn   = !bHigh ? 0 : sleSkywellState->getFieldU32 (sfLowQualityIn);
                uHighQualityIn  =  bHigh ? 0 : sleSkywellState->getFieldU32 (sfHighQualityIn);
            }

            if (QUALITY_ONE == uLowQualityIn)   uLowQualityIn   = 0;

            if (QUALITY_ONE == uHighQualityIn)  uHighQualityIn  = 0;

            //
            // Quality out
            //

            if (!bQualityOut)
            {
                // Not setting. Just get it.

                uLowQualityOut  = sleSkywellState->getFieldU32 (sfLowQualityOut);
                uHighQualityOut = sleSkywellState->getFieldU32 (sfHighQualityOut);
            }
            else if (uQualityOut)
            {
                // Setting.

                sleSkywellState->setFieldU32 (!bHigh ? sfLowQualityOut : sfHighQualityOut, uQualityOut);

                uLowQualityOut  = !bHigh ? uQualityOut : sleSkywellState->getFieldU32 (sfLowQualityOut);
                uHighQualityOut =  bHigh ? uQualityOut : sleSkywellState->getFieldU32 (sfHighQualityOut);
            }
            else
            {
                // Clearing.

                sleSkywellState->makeFieldAbsent (!bHigh ? sfLowQualityOut : sfHighQualityOut);

                uLowQualityOut  = !bHigh ? 0 : sleSkywellState->getFieldU32 (sfLowQualityOut);
                uHighQualityOut =  bHigh ? 0 : sleSkywellState->getFieldU32 (sfHighQualityOut);
            }

            std::uint32_t const uFlagsIn (sleSkywellState->getFieldU32 (sfFlags));
            std::uint32_t uFlagsOut (uFlagsIn);

            if (bSetNoSkywell && !bClearNoSkywell && (bHigh ? saHighBalance : saLowBalance) >= zero)
            {
                uFlagsOut |= (bHigh ? lsfHighNoSkywell : lsfLowNoSkywell);
            }
            else if (bClearNoSkywell && !bSetNoSkywell)
            {
                uFlagsOut &= ~(bHigh ? lsfHighNoSkywell : lsfLowNoSkywell);
            }

            if (bSetFreeze && !bClearFreeze && !mTxnAccount->isFlag  (lsfNoFreeze))
            {
                uFlagsOut           |= (bHigh ? lsfHighFreeze : lsfLowFreeze);
            }
            else if (bClearFreeze && !bSetFreeze)
            {
                uFlagsOut           &= ~(bHigh ? lsfHighFreeze : lsfLowFreeze);
            }

            if (QUALITY_ONE == uLowQualityOut)  uLowQualityOut  = 0;

            if (QUALITY_ONE == uHighQualityOut) uHighQualityOut = 0;

            bool const bLowDefSkywell        = (sleLowAccount->getFlags() & lsfDefaultSkywell)  == 0;
            bool const bHighDefSkywell       = (sleHighAccount->getFlags() & lsfDefaultSkywell)  == 0;

            bool const  bLowReserveSet      = uLowQualityIn || uLowQualityOut ||
                                              ((uFlagsOut & lsfLowNoSkywell) == 0) != bLowDefSkywell ||
                                              (uFlagsOut & lsfLowFreeze) ||
                                              saLowLimit || saLowBalance > zero;
            bool const  bLowReserveClear    = !bLowReserveSet;

            bool const  bHighReserveSet     = uHighQualityIn || uHighQualityOut ||
                                              ((uFlagsOut & lsfHighNoSkywell) == 0) != bHighDefSkywell ||
                                              (uFlagsOut & lsfHighFreeze) ||
                                              saHighLimit || saHighBalance > zero;
            bool const  bHighReserveClear   = !bHighReserveSet;

            bool const  bDefault            = bLowReserveClear && bHighReserveClear;

            bool const  bLowReserved = (uFlagsIn & lsfLowReserve);
            bool const  bHighReserved = (uFlagsIn & lsfHighReserve);

            bool        bReserveIncrease    = false;

            if (bSetAuth)
            {
                uFlagsOut |= (bHigh ? lsfHighAuth : lsfLowAuth);
            }

            if (bLowReserveSet && !bLowReserved)
            {
                // Set reserve for low account.
                mEngine->view ().incrementOwnerCount (sleLowAccount);
                uFlagsOut |= lsfLowReserve;

                if (!bHigh)
                    bReserveIncrease = true;
            }

            if (bLowReserveClear && bLowReserved)
            {
                // Clear reserve for low account.
                mEngine->view ().decrementOwnerCount (sleLowAccount);
                uFlagsOut &= ~lsfLowReserve;
            }

            if (bHighReserveSet && !bHighReserved)
            {
                // Set reserve for high account.
                mEngine->view ().incrementOwnerCount (sleHighAccount);
                uFlagsOut |= lsfHighReserve;

                if (bHigh)
                    bReserveIncrease    = true;
            }

            if (bHighReserveClear && bHighReserved)
            {
                // Clear reserve for high account.
                mEngine->view ().decrementOwnerCount (sleHighAccount);
                uFlagsOut &= ~lsfHighReserve;
            }

            if (uFlagsIn != uFlagsOut)
                sleSkywellState->setFieldU32 (sfFlags, uFlagsOut);

            if (bDefault || badCurrency() == currency)
            {
                // Delete.

                terResult = mEngine->view ().trustDelete (sleSkywellState, uLowAccountID, uHighAccountID);
            }
            // Reserve is not scaled by load.
            else if (bReserveIncrease && mPriorBalance < uReserveCreate)
            {
                m_journal.trace <<
                    "Delay transaction: Insufficent reserve to add trust line.";

                // Another transaction could provide SWT to the account and then
                // this transaction would succeed.
                terResult = tecINSUF_RESERVE_LINE;
            }
            else
            {
                mEngine->view().entryModify (sleSkywellState);

                m_journal.trace << "Modify skywell line";
            }
        }
        // Line does not exist.
        else if (!saLimitAmount                       // Setting default limit.
                 && (!bQualityIn || !uQualityIn)      // Not setting quality in or setting default quality in.
                 && (!bQualityOut || !uQualityOut))   // Not setting quality out or setting default quality out.
        {
            m_journal.trace <<
                "Redundant: Setting non-existent skywell line to defaults.";
            return tecNO_LINE_REDUNDANT;
        }
        else if (mPriorBalance < uReserveCreate) // Reserve is not scaled by load.
        {
            m_journal.trace <<
                "Delay transaction: Line does not exist. Insufficent reserve to create line.";

            // Another transaction could create the account and then this transaction would succeed.
            terResult = tecNO_LINE_INSUF_RESERVE;
        }
        else
        {
            // Zero balance in currency.
            STAmount saBalance ({currency, noAccount()});

            uint256 index (getSkywellStateIndex (
                mTxnAccountID, uDstAccountID, currency));

            m_journal.trace <<
                "doTrustSet: Creating skywell line: " <<
                to_string (index);

            // Create a new skywell line.
            terResult = mEngine->view ().trustCreate (
                              bHigh,
                              mTxnAccountID,
                              uDstAccountID,
                              index,
                              mTxnAccount,
                              bSetAuth,
                              bSetNoSkywell && !bClearNoSkywell,
                              bSetFreeze && !bClearFreeze,
                              saBalance,
                              saLimitAllow,       // Limit for who is being charged.
                              uQualityIn,
                              uQualityOut);
			
        }

        return terResult;
    }

};

TER
transact_SetTrust (
    STTx const& txn,
    TransactionEngineParams params,
    TransactionEngine* engine)
{
    return SetTrust (txn, params, engine).apply ();
}
}
