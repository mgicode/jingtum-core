#include <services/rpc/handlers/RPCInfo.h>
#include <services/rpc/handlers/ServerInfo.h>
#include <boost/algorithm/string.hpp>
#include <common/base/Log.h>
#include <common/json/to_string.h>

namespace skywell {
namespace RPC {


RPCInfo::info_map_type RPCInfo::mAPIInfo = info_map_type();
RPCInfo::info_map_type RPCInfo::mRPCInfo = info_map_type();
RPCInfo::info_map_type RPCInfo::mRPCError = info_map_type();
RPCInfo::info_map_type RPCInfo::mAPIError = info_map_type();
bool RPCInfo::isprint = false;
unsigned int RPCInfo::api_num =0;
unsigned int RPCInfo::rpc_num =0;
std::string RPCInfo::cmdstr = "";

void skywell::RPC::RPCInfo::reset()
{
   mAPIInfo.clear();
   mRPCInfo.clear();
   mRPCError.clear();
   mAPIError.clear();
   api_num = 0;
   rpc_num = 0;
   cmdstr.clear();
   isprint = false;
}
//
void skywell::RPC::RPCInfo::updateError(Json::Value const& inCommand ,Json::Value const& inError,bool isrpc)
{
  std::string firstCmd = "",secondCmd="null",err ="";
  auto const& cmd = getCmd(inCommand);
  firstCmd = get<0> (cmd);
  secondCmd = get<1> (cmd);
  if (firstCmd.length() == 0 || firstCmd == "rpc_info")
  {
     return ;
  }	
  if(isprint || (firstCmd == "submit" && secondCmd == "null"))  {  //isprint = true or secondCmd = NULL
    if (cmdstr.length() > 0) {
      if( cmdstr == firstCmd || cmdstr == secondCmd) {
         printInfo(inCommand,inError,isrpc);
      }
    }
    else {
      printInfo(inCommand,inError,isrpc);
    }
  }

  if (inError.isMember(jss::error) ) {  //error
     err = boost::lexical_cast<std::string>( inError[jss::error_message]);
     if (inError.isMember(jss::error_exception)) {
       err = boost::lexical_cast<std::string>( inError[jss::error_exception]); //except error
     }
  }
  else if (inError.isMember(jss::result) //reult
           && inError[jss::result].isMember(jss::engine_result_code)
           && inError[jss::result][jss::engine_result_code] != 0)   {
     err = boost::lexical_cast<std::string>( inError[jss::result][jss::engine_result_message]);
  }
  else if (inError.isMember(jss::engine_result_code)  //no result
           && inError[jss::engine_result_code] != 0)   {
     err = boost::lexical_cast<std::string>( inError[jss::engine_result_message]);
  }
  else {  //no error
     return ;
  }
  err = boost::algorithm::trim_copy_if(err, boost::algorithm::is_any_of(" \"\n")) ;
  
  if(err == "null" || err == "Internal error." || err =="" )  {  //isprint = true or secondCmd = NULL
      printInfo(inCommand,inError,isrpc);
  }  

  if(isrpc) {
    update(mRPCError,firstCmd+"|"+secondCmd,err);
  }
  else  {
    update(mAPIError,firstCmd+"|"+secondCmd,err);
  }
}

void skywell::RPC::RPCInfo::updateCmd(Json::Value const& inCommand ,bool isrpc)
{
  auto const& cmd = getCmd(inCommand);
  if ((get<0> (cmd)).length() == 0 || get<0> (cmd) == "rpc_info")
  {
     return ;
  }	
  
  if(isrpc) {
    update(mRPCInfo,get<0> (cmd),get<1> (cmd)); //first cmd ,second cmd
  }
  else  {
    update(mAPIInfo,get<0> (cmd),get<1> (cmd));
  }
}

void skywell::RPC::RPCInfo::update(info_map_type& mInfo,std::string const& cmdkey,std::string const& tycmd)
{	
  if (mInfo.find(cmdkey) == mInfo.end() )
  {  //cmd not exist
    valueType vt;
    vt.insert(valueType::value_type(tycmd,1));
    mInfo.insert(info_map_type::value_type(cmdkey,vt));
  }
  else //cmd exist
  {
     if (mInfo[cmdkey].find(tycmd) == mInfo[cmdkey].end()) {
        mInfo[cmdkey].insert(valueType::value_type(tycmd,1));
     }
     else {
       mInfo[cmdkey][tycmd] +=1;
    }
  }
}

std::tuple<std::string,std::string> skywell::RPC::RPCInfo::getCmd(Json::Value const& inCommand)
{
  std::string firstCmd = "",secondCmd="null";
  firstCmd =  boost::lexical_cast<std::string>(inCommand[jss::command]);
  firstCmd =  boost::algorithm::trim_copy_if(firstCmd, boost::algorithm::is_any_of(" \"\n")) ; //
  if (inCommand.isMember(jss::tx_json))  {
    if (inCommand[jss::tx_json].isMember(jss::TransactionType) ) {
       secondCmd = boost::lexical_cast<std::string>(inCommand[jss::tx_json][jss::TransactionType]);
       secondCmd = boost::algorithm::trim_copy_if(secondCmd, boost::algorithm::is_any_of(" \"\n")) ; //
    }
    else  {
    	 WriteLog (lsWARNING,RPCErr) <<"TransactionType does not exist in tx_json\n"
    	                             <<"cmd:"<< inCommand ;
    }
  }
  else if (inCommand.isMember(jss::tx_blob)) {
    auto const& jsonRes = parseTxBlob(inCommand[jss::tx_blob]);
    if (jsonRes.isMember(jss::info))  {  //blob data ok
    	if (jsonRes[jss::info].isMember(jss::TransactionType))  {
          secondCmd = boost::lexical_cast<std::string>(jsonRes[jss::info][jss::TransactionType]);
          secondCmd = boost::algorithm::trim_copy_if(secondCmd, boost::algorithm::is_any_of(" \"\n")) ; //
        }
        else  {
    	    WriteLog (lsWARNING,RPCErr) <<"TransactionType does not exist in tx_blob\n"
    	                                <<"cmd:\n" <<inCommand 
                                        <<"tx_blob:\n"<<jsonRes;
        }
        if (firstCmd == "show_blob" ) {
            secondCmd = "null";
        }
    }
    else  {   //blob data error
        WriteLog (lsWARNING,RPCErr) <<"tx_blob is invalid\n"
    	                              <<"cmd:"<< inCommand ;
    }
  }
  return std::make_tuple(firstCmd ,secondCmd);
}
void skywell::RPC::RPCInfo::printInfo(Json::Value const& inCommand,Json::Value const& inError,bool isrpc)
{
  Json::Value jvResult (Json::objectValue);
  if (inCommand.isMember(jss::tx_blob) &&inCommand[jss::command] != "show_blob") {
     jvResult = parseTxBlob(inCommand[jss::tx_blob]);
  }
  if (isrpc) {
    WriteLog (lsWARNING,RPCErr) <<"--rpc--NO:"<<++rpc_num<<"   \n cmd: " <<inCommand<<"\ntx_blob:"<<jvResult<<"\n result:"<<inError;
  }
  else {
    WriteLog (lsWARNING,RPCErr) <<"--api--NO:"<<++api_num<<"   \n cmd: " <<inCommand<<"\ntx_blob:"<<jvResult<<"\n result:"<<inError;
  }
}

Json::Value skywell::RPC::RPCInfo::errToJson(std::string const&cmd,info_map_type &mErr,unsigned int& count)
{
  Json::Value jvResult = Json::arrayValue;
  if (mErr.find(cmd)== mErr.end()) {
    return jvResult; 
  }
  valueType mmap = mErr[cmd];
  valueType::iterator ib = mmap.begin();
  for(;ib!=mmap.end();++ib)
  {
    Json::Value jsonTmp = Json::objectValue;     
    jsonTmp[jss::error] = ib->first;
    jsonTmp[jss::failed_total] = ib->second;
    jvResult.append(jsonTmp);
    count += ib->second;
  } 
  return jvResult;
}

Json::Value skywell::RPC::RPCInfo::dataTojson(info_map_type& mCmd,info_map_type& mErr)
{
  Json::Value jsonRes = Json::objectValue;
  Json::Value jvResult = Json::arrayValue;   
  Json::Value jsonTotal = Json::objectValue;
  info_map_type::iterator cib = mCmd.begin();
  unsigned int rec_count = 0,err_count = 0;
  for (;cib!=mCmd.end();++cib)
  {
    unsigned int err_num =0;
    Json::Value jsonTmp = Json::objectValue;
    jsonTmp[jss::command] = cib->first; 
    valueType vmap =cib->second;  //
    if (vmap.find("null")!= vmap.end() && vmap.size() == 1 && cib->first != "submit") {
      jsonTmp[jss::received_total] = vmap["null"];
      rec_count += vmap["null"]; 
      jsonTmp[jss::error] = errToJson(cib->first+"|"+"null",mErr,err_num); 
      err_count += err_num;
    }
    else {
      valueType::iterator ib = vmap.begin();
      Json::Value tyjson = Json::arrayValue;
      unsigned int count =0;
      for (;ib!=vmap.end();++ib)
      {
        err_num = 0; 
        Json::Value jTmp = Json::objectValue;
        jTmp[jss::TransactionType] = ib->first;
        jTmp[jss::error] = errToJson(cib->first+"|"+ib->first,mErr,err_num);
        jTmp[jss::received_total]  = ib->second;
        count += ib->second;
        err_count += err_num;
        tyjson.append(jTmp);
      } 
      jsonTmp[jss::received_total] = count;  
      jsonTmp[jss::transactionType] = tyjson;
      rec_count += count;
    }
    jvResult.append(jsonTmp);
  }	
  jsonTotal[jss::received_total] = rec_count;
  jsonTotal[jss::failed_total] =err_count;
  jsonRes[jss::info] = jvResult;
  jsonRes[jss::total] = jsonTotal;
  return jsonRes;
}

Json::Value skywell::RPC::RPCInfo::show()
{
   Json::Value jvResult (Json::objectValue);
   jvResult[jss::api_command]=dataTojson(mAPIInfo,mAPIError);
   jvResult[jss::rpc_command]=dataTojson(mRPCInfo,mRPCError);
   return jvResult;
}


}
}

