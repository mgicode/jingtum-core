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
#include <common/misc/NetworkOPs.h>
#include <common/base/Log.h>
#include <network/resource/Fees.h>
#include <common/core/Config.h>
#include <common/core/JobQueue.h>
#include <common/json/Object.h>
#include <common/json/to_string.h>
#include <services/rpc/RPCHandler.h>
#include <services/rpc/Yield.h>
#include <services/rpc/impl/Tuning.h>
#include <services/rpc/impl/Handler.h>
#include <services/net/InfoSub.h>
#include <services/net/RPCErr.h>
#include <services/server/Role.h>
#include <ledger/LedgerMaster.h>
#include <protocol/JsonFields.h>
#include <main/Application.h>

namespace skywell {
namespace RPC {

namespace {

/**
   This code is called from both the HTTP RPC handler and Websockets.

   The form of the Json returned is somewhat different between the two services.

   HTML:
     Success:
        {
           "result" : {
              "ledger" : {
                 "accepted" : false,
                 "transaction_hash" : "..."
              },
              "ledger_index" : 10300865,
              "validated" : false,
              "status" : "success"  # Status is inside the result.
           }
        }

     Failure:
        {
           "result" : {
              "error" : "noNetwork",
              "error_code" : 16,
              "error_message" : "Not synced to Skywell network.",
              "request" : {
                 "command" : "ledger",
                 "ledger_index" : 10300865
              },
              "status" : "error"
           }
        }

   Websocket:
     Success:
        {
           "result" : {
              "ledger" : {
                 "accepted" : false,
                 "transaction_hash" : "..."
              },
              "ledger_index" : 10300865,
              "validated" : false
           }
           "type": "response",
           "status": "success",   # Status is OUTside the result!
           "id": "client's ID",   # Optional
           "warning": 3.14        # Optional
        }

     Failure:
        {
          "error" : "noNetwork",
          "error_code" : 16,
          "error_message" : "Not synced to Skywell network.",
          "request" : {
             "command" : "ledger",
             "ledger_index" : 10300865
          },
          "type": "response",
          "status" : "error",
          "id": "client's ID"   # Optional
        }

 */

error_code_i fillHandler (Context& context,
                          boost::optional<Handler const&>& result)
{
    if (context.role != Role::ADMIN)
    {
        //  NOTE Should we also add up the jtRPC jobs?
        //
        int jc = getApp().getJobQueue ().getJobCountGE (jtCLIENT);
        if (jc > Tuning::maxJobQueueClients)
        {
            WriteLog (lsDEBUG, RPCHandler) << "Too busy for command: " << jc;
            return rpcTOO_BUSY;
        }
    }

    if (!context.params.isMember ("command"))
        return rpcCOMMAND_MISSING;

    std::string strCommand  = context.params[jss::command].asString ();

    WriteLog (lsTRACE, RPCHandler) << "COMMAND:" << strCommand;
    WriteLog (lsTRACE, RPCHandler) << "REQUEST:" << context.params;

    auto handler = getHandler(strCommand);

    if (!handler)
        return rpcUNKNOWN_COMMAND;

    if (handler->role_ == Role::ADMIN && context.role != Role::ADMIN)
        return rpcNO_PERMISSION;

    if ((handler->condition_ & NEEDS_NETWORK_CONNECTION) &&
        (context.netOps.getOperatingMode () < NetworkOPs::omSYNCING))
    {
        WriteLog (lsINFO, RPCHandler)
            << "Insufficient network mode for RPC: "
            << context.netOps.strOperatingMode ();

        return rpcNO_NETWORK;
    }

    if (! getConfig ().RUN_STANDALONE &&
        handler->condition_ & NEEDS_CURRENT_LEDGER)
    {
        if (getApp ().getLedgerMaster ().getValidatedLedgerAge () >
            Tuning::maxValidatedLedgerAge)
        {
            return rpcNO_CURRENT;
        }

        auto const cID = context.netOps.getCurrentLedgerID ();
        auto const vID = context.netOps.getValidatedSeq ();

        if (cID + 10 < vID)
        {
            WriteLog (lsDEBUG, RPCHandler) << "Current ledger ID(" << cID <<
                ") is less than validated ledger ID(" << vID << ")";
            return rpcNO_CURRENT;
        }
    }

    if ((handler->condition_ & NEEDS_CLOSED_LEDGER) &&
        !context.netOps.getClosedLedger ())
    {
        return rpcNO_CLOSED;
    }

    result = *handler;
    return rpcSUCCESS;
}

template <class Object, class Method>
Status callMethod (
    Context& context, Method method, std::string const& name, Object& result)
{
    try
    {
        auto v = getApp().getJobQueue().getLoadEventAP(
            jtGENERIC, "cmd:" + name);
        return method (context, result);
    }
    catch (std::exception& e)
    {
        WriteLog (lsINFO, RPCHandler) << "Caught throw: " << e.what ();

        if (context.loadType == Resource::feeReferenceRPC)
            context.loadType = Resource::feeExceptionRPC;

        inject_error (rpcINTERNAL, result);
        return rpcINTERNAL;
    }
}

template <class Method, class Object>
void getResult (
    Context& context, Method method, Object& object, std::string const& name)
{
    auto&& result = Json::addObject (object, jss::result);
    if (auto status = callMethod (context, method, name, result))
    {
        WriteLog (lsDEBUG, RPCErr) << "rpcError: " << status.toString();
        result[jss::status] = jss::error;
        result[jss::request] = context.params;
    }
    else
    {
        result[jss::status] = jss::success;
    }
}

} // namespace

Status doCommand (
    RPC::Context& context, Json::Value& result, YieldStrategy const&)
{
    boost::optional <Handler const&> handler;
    if (auto error = fillHandler (context, handler))
    {
        inject_error (error, result);
        return error;
    }

    if (auto method = handler->valueMethod_)
        return callMethod (context, method, handler->name_, result);

    return rpcUNKNOWN_COMMAND;
}

/** Execute an RPC command and store the results in a string. */
void executeRPC (
    RPC::Context& context, std::string& output, YieldStrategy const& strategy)
{
    boost::optional <Handler const&> handler;
    if (auto error = fillHandler (context, handler))
    {
        auto wo = Json::stringWriterObject (output);
        auto&& sub = Json::addObject (*wo, jss::result);
        inject_error (error, sub);
    }
    else if (auto method = handler->objectMethod_)
    {
        auto wo = Json::stringWriterObject (output);
        getResult (context, method, *wo, handler->name_);
    }
    else if (auto method = handler->valueMethod_)
    {
        auto object = Json::Value (Json::objectValue);
        getResult (context, method, object, handler->name_);
        if (strategy.streaming == YieldStrategy::Streaming::yes)
            output = jsonAsString (object);
        else
            output = to_string (object);
    }
    else
    {
        // Can't ever get here.
        assert (false);
        throw std::logic_error ("RPC handler with no method");
    }
}

Role roleRequired (std::string const& method)
{
    auto handler = RPC::getHandler(method);

    if (!handler)
        return Role::FORBID;

    return handler->role_;
}

} // RPC
} // skywell
