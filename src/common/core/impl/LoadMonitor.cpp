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
#include <boost/format.hpp> 
#include <algorithm>

#include <common/base/Log.h>
#include <common/base/UptimeTimer.h>
#include <common/core/LoadMonitor.h>
#include <protocol/JsonFields.h>

using namespace std;


namespace skywell {

/*

TODO
----

- Use Journal for logging

*/

//------------------------------------------------------------------------------

LoadMonitor::map_type LoadMonitor::mThreadInfo = map_type();//Save thread task information 
boost::posix_time::ptime LoadMonitor::mStartTime =   //start time
                            boost::posix_time::second_clock::local_time();
double LoadMonitor::mExecTime = 0; //The sum of task execution time 

LoadMonitor::Stats::Stats()
    : count (0)
    , latencyAvg (0)
    , latencyPeak (0)
    , isOverloaded (false)
{
}

//------------------------------------------------------------------------------

LoadMonitor::LoadMonitor ()
    : mCounts (0)
    , mLatencyEvents (0)
    , mLatencyMSAvg (0)
    , mLatencyMSPeak (0)
    , mTargetLatencyAvg (0)
    , mTargetLatencyPk (0)
    , mLastUpdate (UptimeTimer::getInstance ().getElapsedSeconds ())
{
}

//  NOTE WHY do we need "the mutex?" This dependence on
//         a hidden global, especially a synchronization primitive,
//         is a flawed design.
//         It's not clear exactly which data needs to be protected.
//
// call with the mutex
void LoadMonitor::update ()
{
    int now = UptimeTimer::getInstance ().getElapsedSeconds ();

    //  TODO stop returning from the middle of functions.

    if (now == mLastUpdate) // current
        return;

    //  TODO Why 8?
    if ((now < mLastUpdate) || (now > (mLastUpdate + 8)))
    {
        // way out of date
        mCounts = 0;
        mLatencyEvents = 0;
        mLatencyMSAvg = 0;
        mLatencyMSPeak = 0;
        mLastUpdate = now;
        //  TODO don't return from the middle...
        return;
    }

    // do exponential decay
    /*
        David:

        "Imagine if you add 10 to something every second. And you
         also reduce it by 1/4 every second. It will "idle" at 40,
         correponding to 10 counts per second."
    */
    do
    {
        ++mLastUpdate;
        mCounts -= ((mCounts + 3) / 4);
        mLatencyEvents -= ((mLatencyEvents + 3) / 4);
        mLatencyMSAvg -= (mLatencyMSAvg / 4);
        mLatencyMSPeak -= (mLatencyMSPeak / 4);
    }
    while (mLastUpdate < now);
}

void LoadMonitor::addCount ()
{
    ScopedLockType sl (mLock);

    update ();
    ++mCounts;
}

void LoadMonitor::addLatency (int latency)
{
    //  NOTE Why does 1 become 0?
    if (latency == 1)
        latency = 0;

    ScopedLockType sl (mLock);

    update ();

    ++mLatencyEvents;
    mLatencyMSAvg += latency;
    mLatencyMSPeak += latency;

    // Units are quarters of a millisecond
    int const latencyPeak = mLatencyEvents * latency * 4;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

std::string LoadMonitor::printElapsed (double seconds)
{
    std::stringstream ss;
    ss << (std::size_t (seconds * 1000 + 0.5)) << " ms";
    return ss.str();
}

void LoadMonitor::addLoadSample (LoadEvent const& sample)
{
    std::string const& name (sample.name());
    beast::RelativeTime const latency (sample.getSecondsTotal());

    if (latency.inSeconds() > 0.5)
    {
        WriteLog ((latency.inSeconds() > 1.0) ? lsWARNING : lsINFO, LoadMonitor)
            << "Job: " << name << " ExecutionTime: " << printElapsed (sample.getSecondsRunning()) <<
            " WaitingTime: " << printElapsed (sample.getSecondsWaiting());
    }
    double const& runTime = sample.getSecondsRunning();
    updateData(name,runTime); 

    //  NOTE Why does 1 become 0?
    std::size_t latencyMilliseconds (latency.inMilliseconds());
    if (latencyMilliseconds == 1)
        latencyMilliseconds = 0;

    ScopedLockType sl (mLock);

    update ();
    ++mCounts;
    ++mLatencyEvents;
    mLatencyMSAvg += latencyMilliseconds;
    mLatencyMSPeak += latencyMilliseconds;

    //  NOTE Why are we multiplying by 4?
    int const latencyPeak = mLatencyEvents * latencyMilliseconds * 4;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

/* Add multiple samples
   @param count The number of samples to add
   @param latencyMS The total number of milliseconds
*/
void LoadMonitor::addSamples (int count, std::chrono::milliseconds latency)
{
    ScopedLockType sl (mLock);

    update ();
    mCounts += count;
    mLatencyEvents += count;
    mLatencyMSAvg += latency.count();
    mLatencyMSPeak += latency.count();

    int const latencyPeak = mLatencyEvents * latency.count() * 4 / count;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

void LoadMonitor::setTargetLatency (std::uint64_t avg, std::uint64_t pk)
{
    mTargetLatencyAvg  = avg;
    mTargetLatencyPk = pk;
}

bool LoadMonitor::isOverTarget (std::uint64_t avg, std::uint64_t peak)
{
    return (mTargetLatencyPk && (peak > mTargetLatencyPk)) ||
           (mTargetLatencyAvg && (avg > mTargetLatencyAvg));
}

bool LoadMonitor::isOver ()
{
    ScopedLockType sl (mLock);

    update ();

    if (mLatencyEvents == 0)
        return 0;

    return isOverTarget (mLatencyMSAvg / (mLatencyEvents * 4), mLatencyMSPeak / (mLatencyEvents * 4));
}

LoadMonitor::Stats LoadMonitor::getStats ()
{
    Stats stats;

    ScopedLockType sl (mLock);

    update ();

    stats.count = mCounts / 4;

    if (mLatencyEvents == 0)
    {
        stats.latencyAvg = 0;
        stats.latencyPeak = 0;
    }
    else
    {
        stats.latencyAvg = mLatencyMSAvg / (mLatencyEvents * 4);
        stats.latencyPeak = mLatencyMSPeak / (mLatencyEvents * 4);
    }

    stats.isOverloaded = isOverTarget (stats.latencyAvg, stats.latencyPeak);

    return stats;
}

void LoadMonitor::updateData(std::string const& jobName,double const& runTime)
{
  if (jobName.length() <= 0) 
    return;
  double const& mSeconds = runTime;
  map_type::iterator ib = mThreadInfo.find(jobName); 
  if (ib == mThreadInfo.end() ) 
  {
    tuple_type value = std::make_tuple(1,mSeconds,mSeconds); 
    mThreadInfo.emplace(jobName,value);
  }  
  else
  {
    tuple_type value = ib->second;
	std::get<0>(value) += 1;
	double maxRunTime = std::get<1>(value);
	std::get<1>(value) = (maxRunTime > mSeconds) ? maxRunTime : mSeconds;
	std::get<2>(value) += mSeconds;
    mThreadInfo[jobName] = value; 
  }
  mExecTime += runTime;
}

void LoadMonitor::reset(std::string const& jobName)
{
  if (jobName.length() != 0 )  {
    map_type::iterator ib = mThreadInfo.find(jobName); 
    if ( ib != mThreadInfo.end() ) 
    {
      tuple_type value = ib->second;
      std::get<0>(value) = 0; 
      std::get<1>(value) = 0;
      std::get<2>(value) = 0;
      ib->second = value;
    }
  }
  else   {
    mThreadInfo.clear();
    mExecTime = 0;
    mStartTime = boost::posix_time::second_clock::local_time();
  }
}

Json::Value LoadMonitor::getThreadInfo(std::string const& showJobName)
{
  Json::Value jvResult (Json::objectValue);
  Json::Value& jvArray (jvResult[jss::info] = Json::arrayValue);
  auto mSecondsFunc = [](double const& seconds)
         {
           std::string value (str(boost::format( "%.2f ms" ) % ((std::size_t)(seconds * 100000 + 0.5) * 0.01)));
           if (seconds > -0.0000001 && seconds < 0.000001) {
	      value = "0.0";
	   } 
           return value;
         };
  auto func = [&jvArray,&mSecondsFunc](pair_type const& ib)  
        {
          Json::Value jTmp = (Json::objectValue);
          jTmp[jss::job_name] = ib.first;
		  jTmp["job_times"] = std::get<0>(ib.second);
		  jTmp["max_runTime"] = mSecondsFunc(std::get<1>(ib.second));
		  jTmp["total_runTime"] = mSecondsFunc(std::get<2>(ib.second));
		  jTmp["avg_runTime"] = (std::get<0>(ib.second) == 0) ? "0.0" : mSecondsFunc(std::get<2>(ib.second) / std::get<0>(ib.second));
          jvArray.append(jTmp);
        };

  auto cmpFunc = [](const pair_type& a,const pair_type& b) 
      {
      	tuple_type first = a.second;
      	tuple_type second = b.second;
		return std::get<2>(first) > std::get<2>(second);
      };
  map_type::iterator ib = mThreadInfo.begin(); 
  if (showJobName.length() != 0 )
  {
    ib = mThreadInfo.find(showJobName);
    if ( ib != mThreadInfo.end() ) 
    {
      pair_type pt = std::make_pair(ib->first,ib->second);
      func(pt);
    }
  }
  else
  {
    std::vector<pair_type> vec_data;
    for (;ib!= mThreadInfo.end();++ib) 
    {
      vec_data.push_back(std::make_pair(ib->first,ib->second)); //add in vectot
    }
    sort(vec_data.begin(),vec_data.end(),cmpFunc);  //sort
    for (auto const& item : vec_data)
    {
      func(item);
    }
  }
 
  auto convertTime = [](double const& pTime)
      {
         boost::posix_time::ptime time_5 = boost::posix_time::from_time_t(pTime);
         return boost::posix_time::to_simple_string(time_5.time_of_day());
      }; 
  double runTime = (boost::posix_time::second_clock::local_time() - mStartTime).total_seconds(); //seconds 
  Json::Value jsInfo (Json::objectValue); 
  jsInfo["run_time"] = convertTime(runTime);
  jsInfo["exec_time"] = convertTime(mExecTime);
  double freeTime = runTime - mExecTime;
  if (freeTime >= 0) {
     jsInfo["free_time"]= convertTime(freeTime );
  }
  jvResult["time_info"] = jsInfo; 
  return jvResult;
}

} // skywell
