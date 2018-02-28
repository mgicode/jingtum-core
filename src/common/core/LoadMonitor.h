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

#ifndef SKYWELL_CORE_LOADMONITOR_H_INCLUDED
#define SKYWELL_CORE_LOADMONITOR_H_INCLUDED

#include <chrono>
#include <mutex>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <common/core/LoadEvent.h>
#include <common/json/json_value.h>

namespace skywell {

// Monitors load levels and response times

//  TODO Rename this. Having both LoadManager and LoadMonitor is confusing.
//
class LoadMonitor
{
public:
    LoadMonitor ();

    void addCount ();

    void addLatency (int latency);

    void addLoadSample (LoadEvent const& sample);

    void addSamples (int count, std::chrono::milliseconds latency);

    void setTargetLatency (std::uint64_t avg, std::uint64_t pk);

    bool isOverTarget (std::uint64_t avg, std::uint64_t peak);

    //  TODO make this return the values in a struct.
    struct Stats
    {
        Stats();

        std::uint64_t count;
        std::uint64_t latencyAvg;
        std::uint64_t latencyPeak;
        bool isOverloaded;
    };

    Stats getStats ();

    bool isOver ();

private:
   typedef std::tuple<unsigned int,double,double> tuple_type;
   typedef std::pair<std::string,tuple_type> pair_type;
   typedef std::map<std::string,tuple_type> map_type;
   static map_type mThreadInfo;//Save task execution information 
   static double mExecTime;  //execution time
   static boost::posix_time::ptime mStartTime ; //Statistical start time

public:
   static void updateData(std::string const&,double const&); //
   static void reset(std::string const&);
   static Json::Value getThreadInfo(std::string const&); 

private:
    static std::string printElapsed (double seconds);

    void update ();

    typedef std::mutex LockType;
    typedef std::lock_guard <LockType> ScopedLockType;
    LockType mLock;

    std::uint64_t mCounts;
    int           mLatencyEvents;
    std::uint64_t mLatencyMSAvg;
    std::uint64_t mLatencyMSPeak;
    std::uint64_t mTargetLatencyAvg;
    std::uint64_t mTargetLatencyPk;
    int           mLastUpdate;
};

} // skywell

#endif
