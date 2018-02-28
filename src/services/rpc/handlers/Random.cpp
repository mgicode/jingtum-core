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
#include <crypto/RandomNumbers.h>
#include <common/json/json_value.h>
#include <services/rpc/Context.h>
#include <protocol/JsonFields.h>
#include <services/net/RPCErr.h>
#include <protocol/ErrorCodes.h>

namespace skywell {

// Result:
// {
//   random: <uint256>
// }
Json::Value doRandom (RPC::Context& context)
{
    // TODO(tom): the try/catch is almost certainly redundant, we catch at the
    // top level too.
    try
    {
        uint256 rand;
        random_fill (rand.begin (), rand.size ());

        Json::Value jvResult;
        jvResult[jss::random]  = to_string (rand);
        return jvResult;
    }
    catch (...)
    {
        return rpcError (rpcINTERNAL);
    }
}

} // skywell
