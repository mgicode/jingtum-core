//------------------------------------------------------------------------------
/*
    This file is part of skywelld: https://github.com/skywell/skywelld
    Copyright (c) 2015 Skywell Labs Inc.

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

#ifndef SKYWELLD_SKYWELL_RPC_HANDLERS_SERVERINFO_H
#define SKYWELLD_SKYWELL_RPC_HANDLERS_SERVERINFO_H

#include <BeastConfig.h>
#include <typeinfo>
#include <services/server/Role.h>
#include <services/server/Role.h>
#include <services/rpc/handlers/RPCInfo.h>
#include <common/base/StringUtilities.h>
#include <transaction/book/Amounts.h>

namespace skywell {
Json::Value doThreadInfo (RPC::Context& context);
Json::Value parseTxBlob (Json::Value const& txBlob);
Json::Value doShowBlob  (RPC::Context& context);
Json::Value doRPCInfo  (RPC::Context& context);
Json::Value doServerInfo (RPC::Context& context);
Json::Value doBlacklistInfo (RPC::Context& context);
Account findIssuer(STAmount const& balance,STAmount const& lowLimit,STAmount const& highLimit);
Json::Value doShowData (RPC::Context& context);

} // skywell

#endif
