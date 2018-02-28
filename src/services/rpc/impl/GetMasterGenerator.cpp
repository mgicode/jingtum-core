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
#include <common/misc/NetworkOPs.h>
#include <transaction/paths/FindPaths.h>
#include <common/base/StringUtilities.h>
#include <common/json/json_reader.h>
#include <common/core/LoadFeeTrack.h>
#include <protocol/TxFlags.h>
#include <protocol/Indexes.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <protocol/SkywellAddress.h>
#include <ledger/LedgerMaster.h>
#include <services/rpc/impl/KeypairForSignature.h>
#include <services/rpc/impl/TransactionSign.h>
#include <services/rpc/impl/Tuning.h>
#include <services/rpc/impl/GetMasterGenerator.h>
#include <services/rpc/impl/LegacyPathFind.h>
#include <services/net/RPCErr.h>
#include <main/Application.h>
#include <common/misc/NetworkOPs.h>
#include <protocol/ErrorCodes.h>

namespace skywell {
namespace RPC {

// Look up the master public generator for a regular seed so we may index source accounts ids.
// --> naRegularSeed
// <-- naMasterGenerator
Json::Value getMasterGenerator (
    Ledger::ref lrLedger, SkywellAddress const& naRegularSeed,
    SkywellAddress& naMasterGenerator, NetworkOPs& netOps)
{
    // SkywellAddress       na0Public;      // To find the generator's index.
    // SkywellAddress       na0Private;     // To decrypt the master generator's cipher.
    // SkywellAddress       naGenerator = SkywellAddress::createGeneratorPublic (naRegularSeed);

    // na0Public.setAccountPublic (naGenerator, 0);
    // na0Private.setAccountPrivate (naGenerator, naRegularSeed, 0);

    // SLE::pointer        sleGen = 0;//       = netOps.getGenerator (lrLedger, na0Public.getAccountID ());

    // if (!sleGen)
    // {
    //     // No account has been claimed or has had it password set for seed.
    //     return rpcError (rpcNO_ACCOUNT);
    // }

    // Blob    vucCipher          = 0;// = sleGen->getFieldVL (sfGenerator);
    // Blob    vucMasterGenerator  = na0Private.accountPrivateDecrypt (na0Public, vucCipher);

    // if (vucMasterGenerator.empty ())
    // {
    //     // return rpcError (rpcFAIL_GEN_DECRYPT);
    //     return rpcError(0);
    // }

    // naMasterGenerator.setGenerator (vucMasterGenerator);

    return Json::Value (Json::objectValue);
}

} // RPC
} // skywell
