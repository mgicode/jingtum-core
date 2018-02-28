//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2012-2014 Skywell Labs Inc.

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
#include <crypto/KeyType.h>
#include <protocol/SkywellAddress.h>
#include <services/rpc/handlers/WalletPropose.h>
#include <crypto/ed25519-donna/ed25519.h>
#include <BeastConfig.h>
#include <protocol/JsonFields.h>
#include <services/rpc/Context.h>
#include <common/core/Job.h>
#include <main/Application.h>
#include <protocol/ErrorCodes.h>
#include <services/rpc/impl/LookupLedger.h>
#include <common/base/Log.h>
#include <common/misc/NetworkOPs.h>
#include <network/resource/Fees.h>
#include <protocol/ErrorCodes.h>
#include <services/rpc/RPCHandler.h>
#include <services/net/RPCErr.h>

namespace skywell {

// {
//  passphrase: <string>
// }
Json::Value doWalletPropose (RPC::Context& context)
{
    return walletPropose (context.params);
}

Json::Value walletPropose (Json::Value const& params)
{
    SkywellAddress   naSeed;
    SkywellAddress   naAccount;

    KeyType type = KeyType::secp256k1;

    bool const has_key_type   = params.isMember (jss::key_type);
    bool const has_passphrase = params.isMember (jss::passphrase);

    if (has_key_type)
    {
        // `key_type` must be valid if present.

        type = keyTypeFromString (params[jss::key_type].asString());

        if (type == KeyType::invalid)
        {
            return rpcError (rpcBAD_SEED);
        }

        naSeed = getSeedFromRPC (params);
    }
    else if (has_passphrase)
    {
        naSeed.setSeedGeneric (params[jss::passphrase].asString());
    }
    else
    {
        naSeed.setSeedRandom();
    }

    if (!naSeed.isSet())
    {
        return rpcError(rpcBAD_SEED);
    }

    if (type == KeyType::secp256k1)
    {
        SkywellAddress naGenerator = SkywellAddress::createGeneratorPublic (naSeed);
        naAccount.setAccountPublic (naGenerator, 0);
    }
    else if (type == KeyType::ed25519)
    {
        uint256 secretkey = keyFromSeed (naSeed.getSeed());

        Blob publickey (33);
        publickey[0] = 0xED;
        ed25519_publickey (secretkey.data(), &publickey[1]);
        secretkey.zero();  // security erase

        naAccount.setAccountPublic (publickey);
    }
    else
    {
        assert (false);  // not reached
    }

    Json::Value obj (Json::objectValue);

    obj[jss::master_seed] = naSeed.humanSeed ();
    obj[jss::master_seed_hex] = to_string (naSeed.getSeed ());
    obj[jss::master_key] = naSeed.humanSeed1751();
    obj[jss::account_id] = naAccount.humanAccountID ();
    obj[jss::public_key] = naAccount.humanAccountPublic();
    obj[jss::key_type] = to_string (type);

    auto acct = naAccount.getAccountPublic();
    obj[jss::public_key_hex] = strHex(acct.begin(), acct.size());

    return obj;
}

} // skywell
