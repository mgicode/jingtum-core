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

#ifndef SKYWELL_APP_TX_TRANSACTION_H_INCLUDED
#define SKYWELL_APP_TX_TRANSACTION_H_INCLUDED

#include <protocol/Protocol.h>
#include <protocol/STTx.h>
#include <protocol/TER.h>
#include <boost/optional.hpp>
#include <services/rpc/Context.h>
#include <protocol/JsonFields.h>
#include <transaction/tx/TransactionMeta.h>

namespace skywell {


//
// Transactions should be constructed in JSON with. Use STObject::parseJson to
// obtain a binary version.
//

class Database;

    enum TransStatus
    {
        NEW         = 0, // just received / generated
        INVALID     = 1, // no valid signature, insufficient funds
        INCLUDED    = 2, // added to the current ledger
        CONFLICTED  = 3, // losing to a conflicting transaction
        COMMITTED   = 4, // known to be in a ledger
        HELD        = 5, // not valid now, maybe later
        REMOVED     = 6, // taken out of a ledger
        OBSOLETE    = 7, // a compatible transaction has taken precedence
        INCOMPLETE  = 8  // needs more signatures
    };

    enum class Validate {NO, YES};

    // This class is for constructing and examining transactions.
    // Transactions are static so manipulation functions are unnecessary.
    class Transaction
        : public std::enable_shared_from_this<Transaction>
        , public CountedObject <Transaction>
    {
    public:
        static char const* getCountedObjectName () { return "Transaction"; }

        typedef std::shared_ptr<Transaction> pointer;
        typedef const pointer& ref;

    public:
        Transaction (STTx::ref, Validate, std::string&) noexcept;

        static
        Transaction::pointer
        sharedTransaction (Blob const&, Validate);

        static
        Transaction::pointer
        transactionFromSQL (
            boost::optional<std::uint64_t> const& ledgerSeq,
            boost::optional<std::string> const& status,
            std::string const& rawTxn,
            Validate validate);

        static
        TransStatus
        sqlTransactionStatus(boost::optional<std::string> const& status);
        
        bool checkSign (std::string&) const;

        STTx::ref getSTransaction ()
        {
            return mTransaction;
        }


        uint256 const& getID() const
        {
            return mTransactionID;
        }

        LedgerIndex getLedger() const
        {
            return mInLedger;
        }

        TransStatus getStatus() const
        {
            return mStatus;
        }

        TER getResult()
        {
            return mResult;
        }

        TxType getTransactionType()
        {
            return mTransaction->getTxnType();
        }

        void setResult(TER terResult)
        {
            mResult = terResult;
        }

        void setStatus(TransStatus status, std::uint32_t ledgerSeq);

        void setStatus(TransStatus status)
        {
            mStatus = status;
        }

        void setLedger(LedgerIndex ledger)
        {
            mInLedger = ledger;
        }

        Json::Value getJson(int options, bool binary = false) const;

        static Transaction::pointer load(uint256 const& id);

        STArray const& getTxs()
        {
            return mTransaction->getFieldArray(sfOperations);
        }

    private:
        uint256         mTransactionID;
        SkywellAddress   mAccountFrom;
        SkywellAddress   mFromPubKey;    // Sign transaction with this. mSignPubKey
        SkywellAddress   mSourcePrivate; // Sign transaction with this.

        LedgerIndex     mInLedger;
        TransStatus     mStatus;
        TER             mResult;

        STTx::pointer mTransaction;
    };

extern void
addPaymentDeliveredAmount (
    Json::Value& meta,
    RPC::Context& context,
    Transaction::pointer transaction,
TransactionMetaSet::pointer transactionMeta);

} // skywell



#endif
