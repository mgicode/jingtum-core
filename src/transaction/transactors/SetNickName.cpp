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
#include <common/core/Config.h>
#include <protocol/Indexes.h>
#include <protocol/TxFlags.h>
namespace skywell{
	class SetNickName 
		:public Transactor
	{
        static std::size_t const DOMAIN_BYTES_MAX = 256;
		static std::size_t const NICKNAME_BYTES_MAX = 256;

	public:
		SetNickName(
			STTx const& txn,
			TransactionEngineParams params,
			TransactionEngine* engine)
			: Transactor(
			txn,
			params,
			engine,
			deprecatedLogs().journal("SetNickName"))
		{
            
		}

		TER preCheck() override
		{
			return Transactor::preCheck();
		}

		TER doApply() override
		{

             mTxnAccountID=mTxn.getSourceAccount().getAccountID();
            

			 //find the nicknamenode
			 auto const index = getNickNameIndex(mTxnAccountID);
			 SLE::pointer sleNick(mEngine->view().entryCache(ltNICKNAME, index));

			 if (!sleNick)
			 {   //this accountcickname does not exist
				 //create the nickname for this account
				 auto const newIndex = getNickNameIndex(mTxnAccountID);
				 sleNick = mEngine->view().entryCreate(ltNICKNAME, newIndex);
				 sleNick->setFieldAccount(sfAccount, mTxnAccountID);
				// sleDst->setFieldU32(sfSequence, 1);
			 }

			 //modify the nickname;
			 if (sleNick)
				 mEngine->view().entryModify(sleNick);

			if (mTxn.isFieldPresent(sfDomain))
			{
				Blob const domain = mTxn.getFieldVL(sfDomain);

				if (domain.size() > DOMAIN_BYTES_MAX)
				{
					m_journal.trace << "domain too long";
					return telBAD_DOMAIN;
				}

				if (domain.empty())
				{
					m_journal.trace << "unset domain";
					sleNick->makeFieldAbsent(sfDomain);
				}
				else
				{
					m_journal.trace << "set domain";
					sleNick->setFieldVL(sfDomain, domain);
					
				}
			}

			if (mTxn.isFieldPresent(sfNickname))
			{
				Blob const NickName = mTxn.getFieldVL(sfNickname);

				if (NickName.size() > NICKNAME_BYTES_MAX)
				{
					m_journal.trace << "NickName too long";
					return telBAD_NICKNAME;
				}

				if (NickName.empty())
				{
					m_journal.trace << "unset NickName";
					sleNick->makeFieldAbsent(sfNickname);
				}
				else
				{
					m_journal.trace << "set NickName";
					sleNick->setFieldVL(sfNickname, NickName);
				}
			}

			return tesSUCCESS;
		}
		

	};

	TER
		transact_SetNickName(
		STTx const& txn,
		TransactionEngineParams params,
		TransactionEngine* engine)
	{
		return SetNickName(txn, params, engine).apply();
	}

}