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

#ifndef SKYWELL_RPC_INTERNALHANDLER_H_INCLUDED
#define SKYWELL_RPC_INTERNALHANDLER_H_INCLUDED

#include <services/rpc/Context.h>
#include <protocol/JsonFields.h>
#include <protocol/ErrorCodes.h>
#include <services/net/RPCErr.h>
#include <services/rpc/RPCHandler.h>
#include <common/base/Log.h>

namespace skywell {
namespace RPC {

/** To dynamically add custom or experimental RPC handlers, construct a new
 * instance of InternalHandler with your own handler function. */
struct InternalHandler
{
    typedef Json::Value (*handler_t) (const Json::Value&);

    InternalHandler (const std::string& name, handler_t handler)
            : name_ (name),
              handler_ (handler)
    {
        nextHandler_ = InternalHandler::headHandler;
        InternalHandler::headHandler = this;
    }

    InternalHandler* nextHandler_;
    std::string name_;
    handler_t handler_;

    static InternalHandler* headHandler;
};

} // RPC
} // skywell

#endif
