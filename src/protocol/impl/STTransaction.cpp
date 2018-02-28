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
#include <common/base/Log.h>
#include <common/base/StringUtilities.h>
#include <common/json/to_string.h>
#include <protocol/HashPrefix.h>
#include <protocol/JsonFields.h>
#include <protocol/Protocol.h>
#include <protocol/ErrorCodes.h>
#include <protocol/STAccount.h>
#include <protocol/STArray.h>
#include <protocol/STTx.h>
#include <protocol/TER.h>
#include <protocol/STTransaction.h>
#include <protocol/TxFlags.h>

#include <boost/format.hpp>
#include <array>

namespace skywell {

	STTransaction::STTransaction(TxType type)
		: STObject(sfTransaction)
		, tx_type_(type)
		, sig_state_(boost::indeterminate)
	{
        if (tx_type_ != ttOPERATION)
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction type: " << tx_type_ ;
			throw std::runtime_error("invalid transaction type");
		}
		auto format = TxFormats::getInstance().findByType(type);

		if (format == nullptr)
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction type: " << type;
			throw std::runtime_error("invalid transaction type");
		}

		set(format->elements);
		setFieldU16(sfTransactionType, format->getType());
	}

	STTransaction::STTransaction(STObject&& object)
		: STObject(std::move(object))
		, sig_state_(boost::indeterminate)
	{
		tx_type_ = static_cast <TxType> (getFieldU16(sfTransactionType));
        if (tx_type_ != ttOPERATION)
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction type: " << tx_type_;
			throw std::runtime_error("invalid transaction type");
		}

		auto format = TxFormats::getInstance().findByType(tx_type_);

		if (!format)
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction type: " << tx_type_;
			throw std::runtime_error("invalid transaction type");
		}

		if (!setType(format->elements))
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction not legal for format";
			throw std::runtime_error("transaction not valid");
		}
	}

	STTransaction::STTransaction(SerialIter& sit)
		:STObject(sfTransaction)
		, sig_state_(boost::indeterminate)
	{
		int length = sit.getBytesLeft();

		if ((length < Protocol::txMinSizeBytes) || (length > Protocol::txMaxSizeBytes))
		{
			WriteLog(lsERROR, STTransaction) <<
				"Transaction has invalid length: " << length;
			throw std::runtime_error("Transaction length invalid");
		}

		set(sit);
		tx_type_ = static_cast<TxType> (getFieldU16(sfTransactionType));
        if (tx_type_ != ttOPERATION)
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction type: " << tx_type_ ;
			throw std::runtime_error("invalid transaction type");
		}
		auto format = TxFormats::getInstance().findByType(tx_type_);

		if (!format)
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Invalid transaction type: " << tx_type_;
			throw std::runtime_error("invalid transaction type");
		}

		if (!setType(format->elements))
		{
			WriteLog(lsWARNING, STTransaction) <<
				"Transaction not legal for format";
			throw std::runtime_error("transaction not valid");
		}
	}

	std::string
		STTransaction::getFullText() const
	{
		std::string ret = "\"";
		ret += to_string(getTransactionID());
		ret += "\" = {";
		ret += STObject::getFullText();
		ret += "}";
		return ret;
	}

	std::vector<SkywellAddress>
		STTransaction::getMentionedAccounts() const
	{
		std::vector<SkywellAddress> accounts;

		for (auto const& it : *this)
		{
			if (auto sa = dynamic_cast<STAccount const*> (&it))
			{
				auto const na = sa->getValueNCA();

				if (std::find(accounts.cbegin(), accounts.cend(), na) == accounts.cend())
					accounts.push_back(na);
			}
			else if (auto sa = dynamic_cast<STAmount const*> (&it))
			{
				auto const& issuer = sa->getIssuer();

				if (isSWT(issuer))
					continue;

				SkywellAddress na;
				na.setAccountID(issuer);

				if (std::find(accounts.cbegin(), accounts.cend(), na) == accounts.cend())
					accounts.push_back(na);
			}
		}

		return accounts;
	}

	std::map<int, std::vector<Account> > STTransaction::getMentionedAccounts(SkywellAddress const& account)
	{
		std::map<int, std::vector<Account> > mData;
		int index = 0;
		std::vector<Account> vec;
        STArray const& txs = getFieldArray(sfOperations);
		for (auto const tx : txs)
		{
			bool bflag = false;
			vec.clear();
			for (auto const& it : tx)
			{
				if (auto sa = dynamic_cast<STAccount const*> (&it))
				{
					auto const na = sa->getValueNCA();
					if (na == account)
						bflag = true;
					else
						vec.push_back(na.getAccountID());
				}
				else if (auto sa = dynamic_cast<STAmount const*> (&it))
				{
					auto const& issuer = sa->getIssuer();

					if (isSWT(issuer))
						continue;

					SkywellAddress na;
					na.setAccountID(issuer);
					if (na == account)
						bflag = true;
					else
						vec.push_back(na.getAccountID());
				}
			}			
			if (bflag)
			{
				vec.push_back(account.getAccountID());
				mData.insert(std::map<int, std::vector<Account> >::value_type(index, vec));
			}
			++index;
		}
		return mData;
	}

	static Blob getSigningData(STTransaction const& that)
	{
		Serializer s;
		s.add32(HashPrefix::txSign);
		that.add(s, false);
		return s.getData();
	}

	uint256
		STTransaction::getSigningHash() const
	{
		return STObject::getSigningHash(HashPrefix::txSign);
	}

	uint256
		STTransaction::getTransactionID() const
	{
		return getHash(HashPrefix::transactionID);
	}

	Blob STTransaction::getSignature() const
	{
		try
		{
			return getFieldVL(sfTxnSignature);
		}
		catch (...)
		{
			return Blob();
		}
	}

	bool STTransaction::isMutiSign() const
	{
		if (isFieldPresent(sfSigns))
		{
			if (getFieldArray(sfSigns).empty())
				return false;

			return true;
		}
		return false;
	}

	void STTransaction::sign(SkywellAddress const& private_key)
	{
		Blob const signature = private_key.accountPrivateSign(getSigningData(*this));
		setFieldVL(sfTxnSignature, signature);
	}

	void STTransaction::mutiSign(std::map<Account,KeyPair>const& key_pairs)
	{
		auto tSigns = getFieldArray(sfSigns);
		auto const & SigningData = getSigningData(*this);
		for (auto const& keypair : key_pairs)
		{
			STObject mSign(sfSign);
			mSign.setFieldVL(sfSigningPubKey, keypair.second.publicKey.getAccountPublic());
			Blob const signature = keypair.second.secretKey.accountPrivateSign(SigningData);
			mSign.setFieldVL(sfTxnSignature, signature);
			tSigns.push_back(std::move(mSign));
		}
		setFieldArray(sfSigns, tSigns);
	}

	void STTransaction::batchOptSign(std::map<Account, KeyPair>const& key_pairs,Json::Value& error)
	{
        auto tTxDatas = getFieldArray(sfOperations);
		std::map<Account, KeyPair>::const_iterator iFind = key_pairs.end();
		for (auto & td : tTxDatas)
		{
			Account acc = td.getFieldAccount(sfAccount).getAccountID();
			iFind = key_pairs.find(acc);
			if (iFind == key_pairs.end())
			{
				error = RPC::make_error(rpcSRC_ACT_MALFORMED,
					to_string(acc) + " did not find a matching key to sign");
				return;
			}
			td.setFieldVL(sfSigningPubKey, iFind->second.publicKey.getAccountPublic());
			Serializer s;
			s.add32(HashPrefix::txSign);
			td.add(s, false);
			Blob const signature = iFind->second.secretKey.accountPrivateSign(s.getData());
			td.setFieldVL(sfTxnSignature, signature);
		}
        setFieldArray(sfOperations, tTxDatas);
	}

	bool STTransaction::checkSign() const
	{
		if (boost::indeterminate(sig_state_))
		{
			try
			{
				ECDSA const fullyCanonical = (getFlags() & tfFullyCanonicalSig)
					? ECDSA::strict
					: ECDSA::not_strict;

				SkywellAddress n;
				n.setAccountPublic(getFieldVL(sfSigningPubKey));

				sig_state_ = n.accountPublicVerify(getSigningData(*this),
					getFieldVL(sfTxnSignature), fullyCanonical);

				if (!sig_state_)
					throw std::runtime_error(transHuman(temBAD_SIGNATURE));
				
				if (isMutiSign())
				{
					auto const & SigningData = getSigningData(*this);
					STArray tSigns = getFieldArray(sfSigns);
					for (STObject const& tSign : tSigns)
					{
						n.setAccountPublic(tSign.getFieldVL(sfSigningPubKey));
						sig_state_ = n.accountPublicVerify(SigningData,
							tSign.getFieldVL(sfTxnSignature), fullyCanonical);
						if (!sig_state_) 
							throw std::runtime_error(transHuman(temBAD_SIGNATURE));
					}
				}
				else
				{
                    STArray txs = getFieldArray(sfOperations);
					for (STObject const& tx : txs)
					{
						n.setAccountPublic(tx.getFieldVL(sfSigningPubKey));
						Serializer s;
						s.add32(HashPrefix::txSign);
						tx.add(s, false);
						sig_state_ = n.accountPublicVerify(s.getData(),
							tx.getFieldVL(sfTxnSignature), fullyCanonical);
						if (!sig_state_)
							throw std::runtime_error(transHuman(temBAD_SIGNATURE));
					}
				}
			}
			catch (...)
			{
				sig_state_ = false;
			}
		}

		assert(!boost::indeterminate(sig_state_));

		return static_cast<bool> (sig_state_);
	}

	void STTransaction::setSigningPubKey(SkywellAddress const& naSignPubKey)
	{
		setFieldVL(sfSigningPubKey, naSignPubKey.getAccountPublic());
	}

	void STTransaction::setSourceAccount(SkywellAddress const& naSource)
	{
		setFieldAccount(sfAccount, naSource);
	}

	Json::Value STTransaction::getJson(int) const
	{
		Json::Value ret = STObject::getJson(0);
		ret[jss::hash] = to_string(getTransactionID());
		return ret;
	}

	Json::Value STTransaction::getJson(int options, bool binary) const
	{
		if (binary)
		{
			Json::Value ret;
			Serializer s = STObject::getSerializer();
			ret[jss::tx] = strHex(s.peekData());
			ret[jss::hash] = to_string(getTransactionID());
			return ret;
		}
		return getJson(options);
	}

	std::string const&
		STTransaction::getMetaSQLInsertReplaceHeader()
	{
		static std::string const sql = "INSERT OR REPLACE INTO Transactions "
			"(TransID, TransType, FromAcct, FromSeq, LedgerSeq, Status, RawTxn, TxnMeta)"
			" VALUES ";

		return sql;
	}

	std::string STTransaction::getMetaSQL(std::uint32_t inLedger,
		std::string const& escapedMetaData) const
	{
		Serializer s;
		add(s);
		return getMetaSQL(s, inLedger, TXN_SQL_VALIDATED, escapedMetaData);
	}

	std::string
		STTransaction::getMetaSQL(Serializer rawTxn,
		std::uint32_t inLedger, char status, std::string const& escapedMetaData) const
	{
		static boost::format bfTrans("('%s', '%s', '%s', '%d', '%d', '%c', %s, %s)");
		std::string rTxn = sqlEscape(rawTxn.peekData());

		auto format = TxFormats::getInstance().findByType(tx_type_);
		assert(format != nullptr);

		return str(boost::format(bfTrans)
			% to_string(getTransactionID()) % format->getName()
			% getOperationAccount().humanAccountID()
			% getSequence() % inLedger % status % rTxn % escapedMetaData);
	}

	static bool
		isMemoOkay(STObject const& st, std::string& reason)
	{
		if (!st.isFieldPresent(sfMemos))
			return true;

		auto const& memos = st.getFieldArray(sfMemos);

		// The number 2048 is a preallocation hint, not a hard limit
		// to avoid allocate/copy/free's
		Serializer s(2048);
		memos.add(s);

		// FIXME move the memo limit into a config tunable
		if (s.getDataLength() > 1024)
		{
			reason = "The memo exceeds the maximum allowed size.";
			return false;
		}

		for (auto const& memo : memos)
		{
			auto memoObj = dynamic_cast <STObject const*> (&memo);

			if (!memoObj || (memoObj->getFName() != sfMemo))
			{
				reason = "A memo array may contain only Memo objects.";
				return false;
			}

			for (auto const& memoElement : *memoObj)
			{
				auto const& name = memoElement.getFName();

				if (name != sfMemoType &&
					name != sfMemoData &&
					name != sfMemoFormat)
				{
					reason = "A memo may contain only MemoType, MemoData or "
						"MemoFormat fields.";
					return false;
				}

				// The raw data is stored as hex-octets, which we want to decode.
				auto data = strUnHex(memoElement.getText());

				if (!data.second)
				{
					reason = "The MemoType, MemoData and MemoFormat fields may "
						"only contain hex-encoded data.";
					return false;
				}

				if (name == sfMemoData)
					continue;

				// The only allowed characters for MemoType and MemoFormat are the
				// characters allowed in URLs per RFC 3986: alphanumerics and the
				// following symbols: -._~:/?#[]@!$&'()*+,;=%
				static std::array<char, 256> const allowedSymbols = []
				{
					std::array<char, 256> a;
					a.fill(0);

					std::string symbols(
						"0123456789"
						"-._~:/?#[]@!$&'()*+,;=%"
						"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
						"abcdefghijklmnopqrstuvwxyz");

					for (char c : symbols)
						a[c] = 1;
					return a;
				}();

				for (auto c : data.first)
				{
					if (!allowedSymbols[c])
					{
						reason = "The MemoType and MemoFormat fields may only "
							"contain characters that are allowed in URLs "
							"under RFC 3986.";
						return false;
					}
				}
			}
		}

		return true;
	}

	// Ensure all account fields are 160-bits
	static
		bool
		isAccountFieldOkay(STObject const& st)
	{
		for (int i = 0; i < st.getCount(); ++i)
		{
			auto t = dynamic_cast<STAccount const*>(st.peekAtPIndex(i));
			if (t && !t->isValueH160())
				return false;
		}

		return true;
	}

	bool passesLocalChecks(STObject const& st, std::string& reason)
	{
		if (!isMemoOkay(st, reason))
			return false;

		if (!isAccountFieldOkay(st))
		{
			reason = "An account field is invalid.";
			return false;
		}

		return true;
	}

} // skywell
