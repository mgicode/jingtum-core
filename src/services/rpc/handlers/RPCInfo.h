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
#ifndef SKYWELL_RPC_HANDLERS_RPCINFO_H
#define SKYWELL_RPC_HANDLERS_RPCINFO_H

#include <string>
#include <tuple>
#include <BeastConfig.h>
#include <services/server/Role.h>
#include <services/rpc/RPCHandler.h>

namespace skywell {
namespace RPC {

/*
 Statistical RPC command
 Number of requests,
 Error type and number
*/
class RPCInfo
{
public:
  static bool isprint;  //
  static std::string cmdstr;  // 
  	 	
private:
  RPCInfo(){};
  ~RPCInfo(){};
  using valueType = std::map<std::string, unsigned int >;
  typedef std::map<std::string,valueType> info_map_type;
  	
  static info_map_type mRPCError;  /* rpc error statistics  */
  static info_map_type mAPIError;  /* api error statistics  */
  static info_map_type mRPCInfo;   /* RPC command statistics  */ 
  static info_map_type mAPIInfo;   /* api command statistics  */ 
  static unsigned int rpc_num,api_num;

public:
  static void updateCmd(Json::Value const& inCommand ,bool );
  static void updateError(Json::Value const& inCommand,Json::Value const& ,bool); 	
  static Json::Value show();  /* Generate JSON format results */
  static void reset();  /*Reset result  */

private:	
  static void update(info_map_type& ,std::string const& ,std::string const&);
  static void printInfo(Json::Value const& ,Json::Value const& ,bool); 	
  static Json::Value dataTojson(info_map_type&,info_map_type &);
  static Json::Value errToJson(std::string const&,info_map_type &,unsigned int&);
  static std::tuple<std::string,std::string> getCmd(Json::Value const& inCommand);
};
}
} // skywell
#endif
