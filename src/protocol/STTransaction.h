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

#ifndef SKYWELL_PROTOCOL_STTRANSACTION_H_INCLUDED
#define SKYWELL_PROTOCOL_STTRANSACTION_H_INCLUDED

#include <protocol/STObject.h>
#include <protocol/TxFormats.h>
#include <protocol/STArray.h>
#include <protocol/STTx.h>
#include <common/base/Log.h>
#include <boost/logic/tribool.hpp>
#include <set>

namespace skywell {

	//  TODO replace these macros with language constants
#define TXN_SQL_NEW         'N'
#define TXN_SQL_CONFLICT    'C'
#define TXN_SQL_HELD        'H'
#define TXN_SQL_VALIDATED   'V'
#define TXN_SQL_INCLUDED    'I'
#define TXN_SQL_UNKNOWN     'U'

	class STTransaction final
		: public STObject
		, public CountedObject <STTransaction>
	{
	public:
		static char const* getCountedObjectName() { return "STTransaction"; }

		typedef std::shared_ptr<STTransaction>        pointer;
		typedef const std::shared_ptr<STTransaction>& ref;

	public:
		STTransaction() = delete;
		STTransaction& operator= (STTransaction const& other) = delete;

		STTransaction(STTransaction const& other) = default;

		explicit STTransaction(SerialIter& sit);
		explicit STTransaction(TxType type);

		explicit STTransaction(STObject&& object);

		STBase*
			copy(std::size_t n, void* buf) const override
		{
			return emplace(n, buf, *this);
		}

		STBase*
			move(std::size_t n, void* buf) override
		{
			return emplace(n, buf, std::move(*this));
		}

		// STObject functions
		SerializedTypeID getSType() const override
		{
			return STI_TRANSACTION;
		}
		std::string getFullText() const override;

		// outer transaction functions / signature functions
		Blob getSignature() const;

		uint256 getSigningHash() const;

		TxType getTxnType() const
		{
			return tx_type_;
		}
		STAmount getTransactionFee() const
		{
			return getFieldAmount(sfFee);
		}
		void setTransactionFee(const STAmount & fee)
		{
			setFieldAmount(sfFee, fee);
		}

		SkywellAddress getOperationAccount() const
		{
			return getFieldAccount(sfAccount);
		}
		Blob getSigningPubKey() const
		{
			return getFieldVL(sfSigningPubKey);
		}
		void setSigningPubKey(const SkywellAddress & naSignPubKey);
		void setSourceAccount(const SkywellAddress & naSource);

		std::uint32_t getTimestamp() const
		{
			return getFieldU32(sfTimestamp);
		}

		std::uint32_t getSequence() const
		{
			return getFieldU32(sfSequence);
		}
		void setSequence(std::uint32_t seq)
		{
			return setFieldU32(sfSequence, seq);
		}

		std::vector<STTx::pointer> getSTTxs() const
		{
			std::vector<STTx::pointer> sttxs;

            if (!isFieldPresent(sfOperations))
				return sttxs;

			try
			{
                STArray const& stArr = getFieldArray(sfOperations);
				for (auto i : stArr)
				{
					STTx::pointer stx = std::make_shared<STTx>(std::move(i));
					sttxs.push_back(stx);					
				}	
			}
			catch (...)
			{
				sttxs.clear();
				WriteLog(lsWARNING, STTransaction) <<
					        "Transaction not legal for format";
			}		
			return sttxs;
		}

		std::vector<SkywellAddress> getMentionedAccounts() const;

		std::map<int, std::vector<Account> > getMentionedAccounts(SkywellAddress const& account);

		uint256 getTransactionID() const;

		virtual Json::Value getJson(int options) const override;
		virtual Json::Value getJson(int options, bool binary) const;

		bool isMutiSign() const;

		//operation account sign
		void sign(SkywellAddress const& private_key);

		void mutiSign(std::map<Account,KeyPair>const& key_pairs);

		void batchOptSign(std::map<Account, KeyPair>const& key_pairs, Json::Value& error);

		bool checkSign() const;

		bool isKnownGood() const
		{
			return (sig_state_ == true);
		}
		bool isKnownBad() const
		{
			return (sig_state_ == false);
		}
		void setGood() const
		{
			sig_state_ = true;
		}
		void setBad() const
		{
			sig_state_ = false;
		}

		// SQL Functions with metadata
		static
			std::string const&
			getMetaSQLInsertReplaceHeader();

		std::string getMetaSQL(
			std::uint32_t inLedger, std::string const& escapedMetaData) const;

		std::string getMetaSQL(
			Serializer rawTxn,
			std::uint32_t inLedger,
			char status,
			std::string const& escapedMetaData) const;

	private:
		TxType tx_type_;

		mutable boost::tribool sig_state_;
	};

	bool passesLocalChecks(STObject const& st, std::string&);

} // skywell

#endif
